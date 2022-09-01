#include "common.h"
#include "config.h"

#include "json.hpp"
#include <fstream>
using json = nlohmann::json;

#include <filesystem>
namespace fs = std::filesystem;

namespace Config {
	Version version;
	ImGuiStyle styleDefault;

	Window window;
	Engine engine;
	SideBar sidebar;
	HUD hud;
	ToolBar toolbar;
	Mono mono;
	Popup popup;
	Mode mode;

	std::map<Action,KeyMouseCombo> controls;

	std::string dataPath(const std::string& name) {
		return fmt("%s%s", mode.dataPath, name);
	}

	std::string savePath(const std::string& name) {
		auto path = std::filesystem::path(fmt("%ssaves/%s", mode.dataPath, name));
		return path.make_preferred().string();
	}

	std::string plansPath() {
		auto path = std::filesystem::path(fmt("%splans", mode.dataPath));
		return path.make_preferred().string();
	}

	bool saveDelete(const std::string& name) {
		std::filesystem::remove_all(savePath(name));
		return true;
	}

	bool saveRename(const std::string& from, const std::string& name) {
		try {
			std::filesystem::rename(savePath(from), savePath(name));
		}
		catch (std::filesystem::filesystem_error& e) {
			notef("rename failed: %s", e.what());
			return false;
		}
		return true;
	}

	bool saveBackup(const std::string& from, const std::string& name) {
		try {
			std::filesystem::copy(savePath(from), savePath(name),
				std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
		}
		catch (std::filesystem::filesystem_error& e) {
			notef("backup failed: %s", e.what());
			return false;
		}
		return true;
	}

	bool saveNameOk(const std::string& name) {
		if (!name.size()) return false;
		for (auto c: name) if (!(std::isalpha(c) || std::isdigit(c) || c == '_' || c == '-')) return false;
		return true;
	}

	bool saveNameFree(const std::string& name) {
		auto path = savePath(name);
		return !std::filesystem::exists(path);
	}

	void load() {
		controls[Action::Copy]
			= KeyMouseCombo::keyUp({SDLK_c}, {SDLK_LCTRL});
		controls[Action::Cut]
			= KeyMouseCombo::keyUp({SDLK_x}, {SDLK_LCTRL});
		controls[Action::Paste]
			= KeyMouseCombo::keyUp({SDLK_v}, {SDLK_LCTRL});
		controls[Action::CopyConfig]
			= KeyMouseCombo::keyUp({SDLK_c}, {SDLK_LALT});
		controls[Action::PasteConfig]
			= KeyMouseCombo::keyUp({SDLK_v}, {SDLK_LALT});
		controls[Action::Pipette]
			= KeyMouseCombo::keyUp({SDLK_q});
		controls[Action::Upgrade]
			= KeyMouseCombo::keyUp({SDLK_u});
		controls[Action::UpgradeCascade]
			= KeyMouseCombo::keyUp({SDLK_u}, {SDLK_LSHIFT});
		controls[Action::Rotate]
			= KeyMouseCombo::keyUp({SDLK_r});
		controls[Action::Cycle]
			= KeyMouseCombo::keyUp({SDLK_c});
		controls[Action::Open]
			= KeyMouseCombo::buttonUp({SDL_BUTTON_LEFT});
		controls[Action::Direct]
			= KeyMouseCombo::buttonUp({SDL_BUTTON_LEFT}, {SDLK_LCTRL});
		controls[Action::Move]
			= KeyMouseCombo::buttonUp({SDL_BUTTON_RIGHT}, {SDLK_LCTRL});
		controls[Action::Link]
			= KeyMouseCombo::keyUp({SDLK_l});
		controls[Action::Connect]
			= KeyMouseCombo::buttonUp({SDL_BUTTON_LEFT});
		controls[Action::Disconnect]
			= KeyMouseCombo::buttonUp({SDL_BUTTON_RIGHT});
		controls[Action::Link]
			= KeyMouseCombo::keyUp({SDLK_l});
		controls[Action::RouteRed]
			= KeyMouseCombo::keyUp({SDLK_1});
		controls[Action::RouteBlue]
			= KeyMouseCombo::keyUp({SDLK_2});
		controls[Action::RouteGreen]
			= KeyMouseCombo::keyUp({SDLK_3});
		controls[Action::RouteSetNext]
			= KeyMouseCombo::buttonUp({SDL_BUTTON_LEFT});
		controls[Action::RouteClrNext]
			= KeyMouseCombo::buttonUp({SDL_BUTTON_RIGHT});
		controls[Action::Flush]
			= KeyMouseCombo::keyUp({SDLK_f}, {SDLK_LSHIFT});
		controls[Action::Construct]
			= KeyMouseCombo::buttonDrag({SDL_BUTTON_LEFT});
		controls[Action::ConstructForce]
			= KeyMouseCombo::buttonDown({SDL_BUTTON_LEFT}, {SDLK_LSHIFT});
		controls[Action::Deconstruct]
			= KeyMouseCombo::keyUp({SDLK_DELETE});
		controls[Action::DeconstructForce]
			= KeyMouseCombo::keyUp({SDLK_DELETE}, {SDLK_LSHIFT});
		controls[Action::ToggleConstruct]
			= KeyMouseCombo::buttonUp({SDL_BUTTON_RIGHT});
		controls[Action::ToggleGrid]
			= KeyMouseCombo::keyUp({SDLK_g});
		controls[Action::ToggleAlignment]
			= KeyMouseCombo::keyUp({SDLK_h});
		controls[Action::ToggleEnable]
			= KeyMouseCombo::keyUp({SDLK_o});
		controls[Action::ToggleCardinalSnap]
			= KeyMouseCombo::keyUp({SDLK_TAB});
		controls[Action::SpecUp]
			= KeyMouseCombo::keyUp({SDLK_PAGEUP});
		controls[Action::SpecDown]
			= KeyMouseCombo::keyUp({SDLK_PAGEDOWN});
		controls[Action::SelectJunk]
			= KeyMouseCombo::keyUp({SDLK_j});
		controls[Action::SelectUnder]
			= KeyMouseCombo::keyUp({SDLK_k});
		controls[Action::Plan]
			= KeyMouseCombo::keyUp({SDLK_b});
		controls[Action::Vehicles]
			= KeyMouseCombo::keyUp({SDLK_v});
		controls[Action::Paint]
			= KeyMouseCombo::keyUp({SDLK_p});
		controls[Action::Escape]
			= KeyMouseCombo::keyUp({SDLK_ESCAPE});
		controls[Action::Save]
			= KeyMouseCombo::keyUp({SDLK_F5});
		controls[Action::Build]
			= KeyMouseCombo::keyUp({SDLK_e});
		controls[Action::Stats]
			= KeyMouseCombo::keyUp({SDLK_F1});
		controls[Action::Log]
			= KeyMouseCombo::keyUp({SDLK_BACKQUOTE});
		controls[Action::Map]
			= KeyMouseCombo::keyUp({SDLK_m});
		controls[Action::Attack]
			= KeyMouseCombo::keyUp({SDLK_F12});
		controls[Action::Pause]
			= KeyMouseCombo::keyUp({SDLK_PAUSE});
		controls[Action::Debug]
			= KeyMouseCombo::keyUp({SDLK_F9});
		controls[Action::Debug2]
			= KeyMouseCombo::keyUp({SDLK_F10});

		std::string folder = (version.minor > 1)
			? fmt("factropy.%d.%d", version.major, version.minor)
			: "factropy";

		char* path = SDL_GetPrefPath("factropy.com", folder.c_str());
		mode.dataPath = std::string(path ? path: "./");
		if (path) SDL_free(path);

		notef("Data path: %s", mode.dataPath);

		if (!std::filesystem::exists(dataPath("saves"))) {
			std::filesystem::create_directory(dataPath("saves"));
		}

		mode.saveName = "game1";

		std::ifstream script(dataPath("state.json"));

		if (!script.good()) {
			save();
			return;
		}

		std::string content((std::istreambuf_iterator<char>(script)), (std::istreambuf_iterator<char>()));
		script.close();

		auto state = json::parse(content);
		mode.saveName = state["game"];

		if (state.contains("/window/vsync"_json_pointer)) {
			Config::window.vsync = state["window"]["vsync"];
		}

		if (state.contains("/window/fullscreen"_json_pointer)) {
			Config::window.fullscreen = state["window"]["fullscreen"];
		}

		if (state.contains("/window/width"_json_pointer) && state.contains("/window/height"_json_pointer)) {
			Config::window.width = state["window"]["width"];
			Config::window.height = state["window"]["height"];
		}

		if (state.contains("/window/antialias"_json_pointer)) {
			Config::window.antialias = state["window"]["antialias"];
		}

		if (state.contains("/window/fps"_json_pointer)) {
			Config::window.fps = state["window"]["fps"];
		}

		if (state.contains("/window/fov"_json_pointer)) {
			Config::window.fov = state["window"]["fov"];
		}

		if (state.contains("/window/horizon"_json_pointer)) {
			Config::window.horizon = state["window"]["horizon"];
		}

		if (state.contains("/window/fog"_json_pointer)) {
			Config::window.fog = state["window"]["fog"];
		}

		if (state.contains("/window/zoom"_json_pointer)) {
			Config::window.zoomUpperLimit = state["window"]["zoom"];
		}

		if (state.contains("/window/lods"_json_pointer)) {
			Config::window.levelsOfDetail[0] = state["window"]["lods"][0];
			Config::window.levelsOfDetail[1] = state["window"]["lods"][1];
			Config::window.levelsOfDetail[2] = state["window"]["lods"][2];
			Config::window.levelsOfDetail[3] = state["window"]["lods"][3];
		}

		if (state.contains("/window/ground"_json_pointer)) {
			Config::window.ground = state["window"]["ground"] == "sand" ? Config::window.groundSand: Config::window.groundGrass;
			Config::window.grid = state["window"]["ground"] == "sand" ? Config::window.gridSand: Config::window.gridGrass;
		}

		if (state.contains("/mode/autosave"_json_pointer)) {
			Config::mode.autosave = state["mode"]["autosave"];
		}

		if (state.contains("/mode/autosaveN"_json_pointer)) {
			Config::mode.autosaveN = state["mode"]["autosaveN"];
		}

		if (state.contains("/mode/autotip"_json_pointer)) {
			Config::mode.autotip = state["mode"]["autotip"];
		}

		if (state.contains("/mode/grid"_json_pointer)) {
			Config::mode.grid = state["mode"]["grid"];
		}

		if (state.contains("/mode/alignment"_json_pointer)) {
			Config::mode.alignment = state["mode"]["alignment"];
		}

		if (state.contains("/mode/cardinalSnap"_json_pointer)) {
			Config::mode.cardinalSnap = state["mode"]["cardinalSnap"];
		}

		if (state.contains("/mode/shadowmap"_json_pointer)) {
			Config::mode.shadowmap = state["mode"]["shadowmap"];
		}

		if (state.contains("/mode/meshmerging"_json_pointer)) {
			Config::mode.meshMerging = state["mode"]["meshmerging"];
		}

		if (state.contains("/mode/waterwaves"_json_pointer)) {
			Config::mode.waterwaves = state["mode"]["waterwaves"];
		}

		if (state.contains("/mode/treebreeze"_json_pointer)) {
			Config::mode.treebreeze = state["mode"]["treebreeze"];
		}

		if (state.contains("/mode/filters"_json_pointer)) {
			Config::mode.filters = state["mode"]["filters"];
		}

		if (state.contains("/mode/overlayUPS"_json_pointer)) {
			Config::mode.overlayUPS = state["mode"]["overlayUPS"];
		}

		if (state.contains("/mode/overlayFPS"_json_pointer)) {
			Config::mode.overlayFPS = state["mode"]["overlayFPS"];
		}

		if (state.contains("/mode/particles"_json_pointer)) {
			Config::mode.particles = state["mode"]["particles"];
		}
	}

	void save() {
		json state;
		state["game"] = Config::mode.saveName;
		state["window"]["width"] = Config::window.width;
		state["window"]["height"] = Config::window.height;
		state["window"]["vsync"] = Config::window.vsync;
		state["window"]["fullscreen"] = Config::window.fullscreen;

		state["window"]["antialias"] = Config::window.antialias;
		state["window"]["fps"] = Config::window.fps;
		state["window"]["fov"] = Config::window.fov;
		state["window"]["horizon"] = Config::window.horizon;
		state["window"]["fog"] = Config::window.fog;
		state["window"]["zoom"] = Config::window.zoomUpperLimit;

		state["window"]["ground"] = Config::window.ground == Config::window.groundSand ? "sand": "grass";

		state["window"]["lods"] = {
			Config::window.levelsOfDetail[0],
			Config::window.levelsOfDetail[1],
			Config::window.levelsOfDetail[2],
			Config::window.levelsOfDetail[3],
		};

		state["mode"]["autosave"] = Config::mode.autosave;
		state["mode"]["autosaveN"] = Config::mode.autosaveN;
		state["mode"]["autotip"] = Config::mode.autotip;
		state["mode"]["grid"] = Config::mode.grid;
		state["mode"]["alignment"] = Config::mode.alignment;
		state["mode"]["cardinalSnap"] = Config::mode.cardinalSnap;
		state["mode"]["shadowmap"] = Config::mode.shadowmap;
		state["mode"]["meshmerging"] = Config::mode.meshMerging;
		state["mode"]["waterwaves"] = Config::mode.waterwaves;
		state["mode"]["treebreeze"] = Config::mode.treebreeze;
		state["mode"]["filters"] = Config::mode.filters;
		state["mode"]["overlayUPS"] = Config::mode.overlayUPS;
		state["mode"]["overlayFPS"] = Config::mode.overlayFPS;
		state["mode"]["particles"] = Config::mode.particles;

		auto out = std::ofstream(dataPath("state.json"));
		out << state;
		out.close();
	}

	void args(int argc, char *argv[]) {
	}

	void sdl() {
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		if (window.antialias) {
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
		}

		window.sdlFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
		if (window.fullscreen) window.sdlFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		if (window.resizable) window.sdlFlags |= SDL_WINDOW_RESIZABLE;
	}

	void profile() {
		engine.cores = SDL_GetCPUCount();
		engine.sceneLoadingThreads = 2;
		engine.sceneInstancingThreads = 2;
		engine.sceneInstancingItemsThreads = 2;

		// scene threads + simulation + margin
		engine.threads = 16;

		if (engine.cores > 4) {
			engine.sceneLoadingThreads = 4;
			engine.sceneInstancingThreads = 4;
			engine.sceneInstancingItemsThreads = 4;
			engine.threads = 32;
		}
	}

	float scale() {
		return (float)window.height / BASELINE_WINDOW_HEIGHT;
	}

	void imgui() {
		auto& fonts = ImGui::GetIO().Fonts;
		fonts->Clear();

		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;

		float popupFontSize = popup.font.size * BASELINE_FONT_SIZE * scale();
		float popupFontSizeIcons = popup.font.icon * BASELINE_FONT_SIZE * scale();
		float sidebarFontSize = sidebar.font.size * BASELINE_FONT_SIZE * scale();
		float sidebarFontSizeIcons = sidebar.font.icon * BASELINE_FONT_SIZE * scale();
		float hudFontSize = hud.font.size * BASELINE_FONT_SIZE * scale();
		float hudFontSizeIcons = hud.font.icon * BASELINE_FONT_SIZE * scale();
		float toolbarFontSize = toolbar.font.size * BASELINE_FONT_SIZE * scale();
		float toolbarFontSizeIcons = toolbar.font.icon * BASELINE_FONT_SIZE * scale();
		float monoFontSize = mono.font.size * BASELINE_FONT_SIZE * scale();

		popup.font.imgui = fonts->AddFontFromFileTTF(popup.font.ttf.c_str(), popupFontSize);

		fonts->AddFontFromFileTTF("font/fontawesome-webfont.ttf", popupFontSizeIcons, &icons_config, icons_ranges);

		sidebar.font.imgui = fonts->AddFontFromFileTTF(sidebar.font.ttf.c_str(), sidebarFontSize);

		fonts->AddFontFromFileTTF("font/fontawesome-webfont.ttf", sidebarFontSizeIcons, &icons_config, icons_ranges);

		hud.font.imgui = fonts->AddFontFromFileTTF(hud.font.ttf.c_str(), hudFontSize);

		fonts->AddFontFromFileTTF("font/fontawesome-webfont.ttf", hudFontSizeIcons, &icons_config, icons_ranges);

		toolbar.font.imgui = fonts->AddFontFromFileTTF(toolbar.font.ttf.c_str(), toolbarFontSize);

		fonts->AddFontFromFileTTF("font/fontawesome-webfont.ttf", toolbarFontSizeIcons, &icons_config, icons_ranges);

		mono.font.imgui = fonts->AddFontFromFileTTF(mono.font.ttf.c_str(), monoFontSize);

		fonts->Build();

		ImGui_ImplOpenGL3_DestroyFontsTexture();
		ImGui_ImplOpenGL3_CreateFontsTexture();

		auto& style = ImGui::GetStyle();
		style = styleDefault;
		style.ScaleAllSizes(scale());

		Config::toolbar.icon.size = scale() < 1.1 ? 0: 1;

		style.FrameRounding = style.FramePadding.x;
		style.WindowRounding = style.FrameRounding;
		style.TabRounding = style.FrameRounding;
	}

	// ImGUI has font scaling, but it works on the initial glyph bitmaps and can look a bit crap
	// Regenerate the font atlas on window resize instead
	void autoscale() {
		int w = 0, h = 0;
		SDL_GL_GetDrawableSize(sdlWindow(), &w, &h);
		bool rescale = w != window.width;

		int hr = BASELINE_WINDOW_HEIGHT * ((float)w/BASELINE_WINDOW_WIDTH);
		if (!window.resizable && !window.fullscreen && h != hr) {
			h = hr;
			rescale = true;
			SDL_SetWindowSize(sdlWindow(), w, h);
		}

		if (rescale) {
			window.width = w;
			window.height = h;
			imgui();
		}
	}

	float width(float r) {
		return r * (float)window.width;
	}

	float height(float r) {
		return r * (float)window.height;
	}

	bool KeyMouseCombo::triggered() const {
		auto ignoreReleaseWhileDragging = [&](MouseButton button) {
			return scene.buttonReleased(button) && scene.buttonDragged(button) && !buttonsDragged.count(button);
		};
	    for (auto mod: mods) if (!scene.keyDown(mod)) return false;
		if (!mods.count(SDLK_LSHIFT) && scene.keyDown(SDLK_LSHIFT)) return false;
		if (!mods.count(SDLK_LCTRL) && scene.keyDown(SDLK_LCTRL)) return false;
		if (!mods.count(SDLK_LALT) && scene.keyDown(SDLK_LALT)) return false;
		if (!mods.count(SDLK_LGUI) && scene.keyDown(SDLK_LGUI)) return false;
		if (!mods.count(SDLK_RSHIFT) && scene.keyDown(SDLK_RSHIFT)) return false;
		if (!mods.count(SDLK_RCTRL) && scene.keyDown(SDLK_RCTRL)) return false;
		if (!mods.count(SDLK_RALT) && scene.keyDown(SDLK_RALT)) return false;
		if (!mods.count(SDLK_RGUI) && scene.keyDown(SDLK_RGUI)) return false;
	    for (auto button: buttonsReleased) if (!scene.buttonReleased(button) || ignoreReleaseWhileDragging(button)) return false;
	    for (auto button: buttonsDown) if (!scene.buttonDown(button) || ignoreReleaseWhileDragging(button)) return false;
	    for (auto key: keysReleased) if (!scene.keyReleased(key)) return false;
	    for (auto key: keysDown) if (!scene.keyDown(key)) return false;
	    return true;
	}

	bool KeyMouseCombo::operator==(const KeyMouseCombo& other) const {
		return mods == other.mods &&
			buttonsReleased == other.buttonsReleased &&
			buttonsDown == other.buttonsDown &&
			keysReleased == other.keysReleased &&
			keysDown == other.keysDown;
	}

	bool KeyMouseCombo::operator!=(const KeyMouseCombo& other) const {
		return mods != other.mods ||
			buttonsReleased != other.buttonsReleased ||
			buttonsDown != other.buttonsDown ||
			keysReleased != other.keysReleased ||
			keysDown != other.keysDown;
	}

	bool KeyMouseCombo::operator<(const KeyMouseCombo& other) const {
		if (mods.size() < other.mods.size()) return true;
		if (mods.size() > other.mods.size()) return false;
		if (buttonsReleased.size() < other.buttonsReleased.size()) return true;
		if (buttonsReleased.size() > other.buttonsReleased.size()) return false;
		if (buttonsDown.size() < other.buttonsDown.size()) return true;
		if (buttonsDown.size() > other.buttonsDown.size()) return false;
		if (keysReleased.size() < other.keysReleased.size()) return true;
		if (keysReleased.size() > other.keysReleased.size()) return false;
		if (keysDown.size() < other.keysDown.size()) return true;
		if (keysDown.size() > other.keysDown.size()) return false;
		ensure(mods.size() == other.mods.size());
		ensure(buttonsReleased.size() == other.buttonsReleased.size());
		ensure(buttonsDown.size() == other.buttonsDown.size());
		ensure(keysReleased.size() == other.keysReleased.size());
		ensure(keysDown.size() == other.keysDown.size());

		std::vector<KeyboardKey> aMods = {mods.begin(), mods.end()};
		std::vector<KeyboardKey> bMods = {other.mods.begin(), other.mods.end()};
		std::vector<MouseButton> aButtonsReleased = {buttonsReleased.begin(), buttonsReleased.end()};
		std::vector<MouseButton> bButtonsReleased = {other.buttonsReleased.begin(), other.buttonsReleased.end()};
		std::vector<MouseButton> aButtonsDown = {buttonsDown.begin(), buttonsDown.end()};
		std::vector<MouseButton> bButtonsDown = {other.buttonsDown.begin(), other.buttonsDown.end()};
		std::vector<KeyboardKey> aKeysReleased = {keysReleased.begin(), keysReleased.end()};
		std::vector<KeyboardKey> bKeysReleased = {other.keysReleased.begin(), other.keysReleased.end()};
		std::vector<KeyboardKey> aKeysDown = {keysDown.begin(), keysDown.end()};
		std::vector<KeyboardKey> bKeysDown = {other.keysDown.begin(), other.keysDown.end()};

		for (uint i = 0; i < mods.size(); i++) {
			if (aMods[i] < bMods[i]) return true;
			if (aMods[i] > bMods[i]) return false;
		}
		for (uint i = 0; i < buttonsReleased.size(); i++) {
			if (aButtonsReleased[i] < bButtonsReleased[i]) return true;
			if (aButtonsReleased[i] > bButtonsReleased[i]) return false;
		}
		for (uint i = 0; i < buttonsDown.size(); i++) {
			if (aButtonsDown[i] < bButtonsDown[i]) return true;
			if (aButtonsDown[i] > bButtonsDown[i]) return false;
		}
		for (uint i = 0; i < keysReleased.size(); i++) {
			if (aKeysReleased[i] < bKeysReleased[i]) return true;
			if (aKeysReleased[i] > bKeysReleased[i]) return false;
		}
		for (uint i = 0; i < keysDown.size(); i++) {
			if (aKeysDown[i] < bKeysDown[i]) return true;
			if (aKeysDown[i] > bKeysDown[i]) return false;
		}
		return false;
	}
}
