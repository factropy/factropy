#pragma once

struct Scene;

#include "frustum.h"
#include "chunk.h"
#include "sim.h"
#include "entity.h"
#include "gui-entity.h"
#include "config.h"
#include "plan.h"
#include "sky.h"
#include "popup.h"

struct Scene {
	uint current = 0;
	uint future = 1;
	trigger advanceDone;
	std::vector<slabpool<GuiEntity>> entityPools[2];
	std::vector<Box> visibleCells;

	GLuint shadowMapFrameBuffer = 0;
	GLuint shadowMapDepthTexture = 0;
	struct {
		Shader part;
		Shader ghost;
		Shader water;
		Shader shadow;
		Shader terrain;
		Shader glow;
		Shader flame;
		Shader tree;
	} shader;
	double near = 1.0;
	double far = 2500.0;

	Mesh* water = nullptr;

	// world
	Point position = {10.0,10.0,10.0};
	Point offset = glm::origin;
	Point target = glm::origin;

	struct {
		Point position = Point::Zero;
		Point target = Point::Zero;
	} jump;

	// relative
	Point direction = glm::south;
	Point camera = glm::origin;

	glm::mat4 perspective = glm::mat4(1.0f);
	glm::mat4 viewCamera = glm::mat4(1.0f);
	glm::dmat4 viewWorld = glm::dmat4(1.0f);
	double aspect = 0.0f;
	double fovy = 0.0f;
	double fps = 60.0f;

	Frustum frustum;
	Box region;

	float altitudeMax = 500.0f;
	float altitudeMin = -50.0f;

	struct MouseXY {
		int x = 0;
		int y = 0;
	};

	struct MouseButton {
		bool down = false;
		bool changed = false;
		MouseXY downAt, upAt, drag;
		bool dragged = false;
		bool pressed = false;
		bool released = false;
		bool clicked = false;
	};

	struct MouseState {
		int x = 0;
		int y = 0;
		int dx = 0;
		int dy = 0;
		int wheel = 0;
		float rH = 0.0f;
		float rV = 0.0f;
		float zW = 0.0f;
		uint64_t moved = 0;
		Ray ray = {Point::Zero, Point::South};

		MouseButton left;
		MouseButton right;
		MouseButton middle;

		MouseState() {};
	};

	MouseState mouse;

	bool focused = true;
	bool showGrid = true;
	bool selecting = false;

	static const uint SelectAll = 0;
	static const uint SelectJunk = 1;
	static const uint SelectUnder = 2;
	uint selectingTypes = SelectAll;

	// world
	struct {
		Point a = glm::origin;
		Point b = glm::origin;
	} selection;

	Box selectionBox;
	Box selectionBoxFuture;
	std::vector<GuiEntity*> selected;
	std::vector<GuiEntity*> selectedFuture;

	std::vector<Chunk*> chunks;
	std::vector<Chunk*> chunksFuture;

	float range = 0.0f;
	bool snapNow = false;

	GuiEntity* hovering = nullptr;
	GuiEntity* directing = nullptr;
	GuiEntity* connecting = nullptr;
	GuiEntity* routing = nullptr;
	minivec<uint> routingHistory;

	uint hoveringFuture = 0;
	uint directRevert = 0;

	minivec<Plan*> plans;
	Plan* placing = nullptr;
	bool placingFits = false;

	GuiFakeEntity* settings = nullptr;

	uint64_t frame = 0;
	int width = 0;
	int height = 0;
	int wheel = 0;
	std::map<int,bool> keys;
	std::map<int,bool> keysLast;

//	uint32_t objects = 0;
//	uint32_t chunks = 0;

	struct outputLine {
		std::string text;
		uint64_t tick = 0;
	};

	struct {
		std::list<outputLine> visible;
		channel<outputLine,-1> queue;
	} output;

	struct {
		struct {
			Mesh* cube;
			Mesh* sphere;
			Mesh* sphere2;
			Mesh* sphere3;
			Mesh* line;
			Mesh* plane;
		} mesh;
	} unit;

	struct {
		Mesh* wifi;
		Mesh* beaconFrame;
		Mesh* beaconGlass;
		Mesh* pipeConnector;
		Mesh* chevron;
		Mesh* flame;
		Mesh* droplet;
	} bits;

	struct {
		Mesh* triangle;
		Mesh* tick;
		Mesh* exclaim;
		Mesh* electricity;
	} icon;

	std::array<float,1024> packets;

	std::map<Spec*,std::vector<uint64_t>> specIconTextures;
	std::map<uint,std::vector<uint64_t>> itemIconTextures;
	std::map<uint,std::vector<uint64_t>> fluidIconTextures;

	struct {
		TimeSeries update;
		TimeSeries updateTerrain;
		TimeSeries updateEntities;
		TimeSeries updateEntitiesParts;
		TimeSeries updateEntitiesFind;
		TimeSeries updateEntitiesLoad;
		TimeSeries updateEntitiesHover;
		TimeSeries updateEntitiesInstances;
		TimeSeries updateEntitiesItems;
		TimeSeries updateCurrent;
		TimeSeries updatePlacing;
		TimeSeries render;
	} stats;

	struct Riser {
		Point start = Point::Zero;
		Mesh* icon = nullptr;
		Color color = 0x00ff00ff;
		uint64_t tick = 0;
	};

	std::vector<Riser> risers;

	void tick(Point pos);
	void exclaim(Point pos);

	Scene() = default;
	void init();
	void prepare();
	void destroy();
	float pen();
	void cube(const Box& box, const Color& color);
	void sphere(const Sphere& sphere, const Color& color);
	void circle(const Point& centroid, float radius, const Color& color, float pen = 0.0f, int step = 5);
	void square(const Point& centroid, float half, const Color& color, float pen = 0.0f);
	void line(const Point& a, const Point& b, const Color& color, float penW = 0.0f, float penH = 0.0f);
	void cuboid(const Cuboid& cuboid, const Color& color, float pen = 0.0f);
	void warning(Mesh* symbol, Point pos);
	void alert(Mesh* symbol, Point pos);
	bool keyDown(int key);
	bool keyReleased(int key);
	bool buttonDown(int button);
	bool buttonReleased(int button);
	bool anyButtonReleased();
	bool buttonDragged(int button);
	Ray mouseRay(int mx, int my);
	std::pair<bool,Point> collisionRayGround(const Ray& ray, float level = 0.0f);
	Point groundTarget(float level = 0.0f);
	Point mouseGroundTarget(float level = 0.0f);
	Point mouseTerrainTarget();
	Point mouseWaterSurfaceTarget();
	void updateMouse();
	void updateCamera();
	void updateVisibleCells();
	void updateEntities();
	void updateCurrent();
	void updateTerrain();
	void updatePlacing();
	void tipBegin(float w = 1.0f);
	void tipEnd();
	void tipResources(Box, Box);
	void tipStorage(Store& store);
	void tipDebug(GuiEntity* ge);
	void update(uint w, uint h, float fps);
	void updateLoading(uint w, uint h);
	void advance();
	void render();
	void renderLoading();
	bool renderSpecIcon();
	bool renderItemIcon();
	bool renderFluidIcon();
	void build(Spec* spec, Point dir = Point::South);
	void saveFramebuffer();
	void print(std::string m);

	void planPush(Plan* plan);
	void planDrop();
	void planPaste();

	void view(Point pos);

	typedef Popup::Texture Texture;
	Texture loadTexture(const char* path);
};

extern Scene scene;