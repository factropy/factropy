#pragma once

// Game config
#include "../imgui/setup.h"
#include <SDL.h>

#include "glm-ex.h"
#include "scene.h"
#include <set>
#include <map>

extern SDL_Window* sdlWindow();

namespace Config {
	struct Version {
		int major = 0;
		int minor = 0;
		int patch = 0;
		char text[32];
	};

	extern Version version;

	extern ImGuiStyle styleDefault;

	typedef uint MouseButton;
	typedef uint KeyboardKey;

	// used as a scaling baseline; actual sizes are determined at runtime
	#define BASELINE_WINDOW_WIDTH 1920.0f
	#define BASELINE_WINDOW_HEIGHT 1080.0f
	#define BASELINE_FONT_SIZE 20.0f

	struct Window {
		int width = BASELINE_WINDOW_WIDTH;
		int height = BASELINE_WINDOW_HEIGHT;
		bool fullscreen = true;
		bool resizable = true;
		bool antialias = true;
		bool vsync = true;
		int fps = 60;
		int horizon = 1500;
		int fog = 1000;
		int levelsOfDetail[4] = {150,600,1000,1500};
		float fogDensity = 0.004f;
		Color fogColor = {0.4,0.74,1.0,1.0};
		Color ambient = 0x666666ff;
		Color ground = 0xc2b280ff;
		Color groundSand = 0xc2b280ff;
		Color groundGrass = 0x8ca66eff;
		Color grid = 0xaa9966ff;
		Color gridSand = 0xaa9966ff;
		Color gridGrass = 0x778877ff;
		uint sdlFlags = 0;
		float speedH = 0.0025;
		float speedV = 0.0025;
		float speedW = 0.25;
		int zoomLowerLimit = 5.0f;
		int zoomUpperLimit = 500.0f;
		float ddpi = 0.0f;
		float hdpi = 0.0f;
		float vdpi = 0.0f;
		int fov = 75;
	};

	extern Window window;

	struct Engine {
		int ups = 60;
		bool pulse = false;
		// Size of the thread pool must be large enough to avoid deadlock.
		// It's not just about CPU core count, but IO blocking, save-game
		// compression, producer/consumer counts etc.
		// https://en.wikipedia.org/wiki/Communicating_sequential_processes
		// Defaults are for 4-cores only -- see profile()
		int cores = 4;
		int threads = 4;
		int sceneLoadingThreads = 2;
		int sceneInstancingThreads = 2;
		int sceneInstancingItemsThreads = 2;
	};

	extern Engine engine;

	struct Font {
		float size = 1.0f;
		float icon = 0.8f;
		std::string ttf = "font/Rubik-Regular.ttf";
		ImFont* imgui = nullptr;
	};

	struct SideBar {
		float width = 0.12f;
		Font font = {
			.size = 0.7f,
			.icon = 0.55f,
		};
	};

	extern SideBar sidebar;

	struct HUD {
		float width = 0.6f;
		Font font = {
			.size = 0.8f,
			.icon = 0.55f,
		};
	};

	extern HUD hud;

	struct Mono {
		Font font = {
			.size = 0.7f,
			.ttf = "font/RobotoMono-Regular.ttf",
		};
	};

	extern Mono mono;

	struct ToolBar {
		float width = 0.6f;
		Font font = {
			.size = 1.0f,
			.icon = 0.55f,
		};
		struct {
			int size = 1;
			float sizes[3] = {32,64,96};
		} icon;
	};

	extern ToolBar toolbar;

	struct Popup {
		float width = 0.6f;
		float height = 0.75f;
		Font font = {
			.size = 1.0f,
			.icon = 0.8f,
		};
	};

	extern Popup popup;

	struct Mode {
		bool game1 = false;
		minivec<int64_t> seeds = {60464830,60555982,60670951,60841977,60900622,52778587};

		bool pause = false;
		bool restart = false;
		bool restartNew = false;
		bool grid = true;
		bool alignment = false;
		bool cardinalSnap = false;
		bool autotip = true;

		std::string dataPath;
		std::string saveName;

		bool fresh = false;
		uint freshSeed = 0;
		bool load = false;

		int autosave = 15;

		bool shadowmap = true;
		bool itemShadows = true;
		bool meshMerging = true;
		bool waterwaves = true;
		bool treebreeze = true;
		bool filters = true;
		bool junk = true;
		bool pathfinder = true;
		bool overlayUPS = true;
		bool overlayFPS = true;
		bool overlayDebug = false;
		bool skyBlocks = false;

		int world = 0;
	};

	extern Mode mode;

	void load();
	void save();
	std::string dataPath(const std::string& name);
	std::string savePath(const std::string& name);
	bool saveDelete(const std::string& name);
	bool saveRename(const std::string& from, const std::string& name);
	bool saveBackup(const std::string& from, const std::string& name);
	bool saveNameOk(const std::string& name);
	bool saveNameFree(const std::string& name);

	void args(int argc, char *argv[]);
	void sdl();
	void profile();
	void imgui();
	float scale();
	void autoscale(SDL_Window*);

	float width(float r);
	float height(float r);

	struct KeyMouseCombo {
		std::set<MouseButton> buttonsReleased;
		std::set<MouseButton> buttonsDown;
		std::set<MouseButton> buttonsDragged;
		std::set<KeyboardKey> keysReleased;
		std::set<KeyboardKey> keysDown;
		std::set<KeyboardKey> mods;
		KeyMouseCombo() = default;
		bool triggered() const;
		bool operator==(const KeyMouseCombo& o) const;
		bool operator!=(const KeyMouseCombo& o) const;
		bool operator<(const KeyMouseCombo& o) const;
	};

	enum class Action {
		Copy = 1,
		Cut,
		Paste,
		Pipette,
		CopyConfig,
		PasteConfig,
		Upgrade,
		UpgradeCascade,
		Rotate,
		Cycle,
		Open,
		Direct,
		Move,
		Link,
		Connect,
		Disconnect,
		RouteRed,
		RouteBlue,
		RouteGreen,
		RouteSetNext,
		RouteClrNext,
		Flush,
		Construct,
		ConstructForce,
		Deconstruct,
		DeconstructForce,
		ToggleConstruct,
		ToggleGrid,
		ToggleEnable,
		ToggleAlignment,
		ToggleCardinalSnap,
		SpecUp,
		SpecDown,
		SelectJunk,
		SelectUnder,
		Plan,
		Paint,
		Zeppelins,
		Escape,
		Save,
		Build,
		Stats,
		Log,
		Map,
		Attack,
		Pause,
		Debug,
		Debug2,
	};

	extern std::map<Action,KeyMouseCombo> controls;
};
