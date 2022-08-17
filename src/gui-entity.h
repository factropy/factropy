#pragma once

// Entities visible on screen are not exposed directly to the rendering thread,
// but loaded into a GuiEntity when a frame starts. This allows the rendering
// thread to mostly proceed without locking the Sim, decoupling UPS and FPS.

struct GuiEntity;
struct GuiFakeEntity;

#include "entity.h"
#include "scene.h"

// These are a bit bloated and should  eventually possibly be rebuilt into a
// shadow entity-component-system.

struct GuiEntity {
	static const uint32_t ELECTRICITY = 1<<16;
	static const uint32_t BLOCKED = 1<<17;

	uint id;
	Spec* spec;
	struct { int x; int y; int z; } _pos;
	struct { float x; float y; float z; } _dir;
	uint state;
	Health health;
	uint flags;

	bool ghost;
	bool connected; // networker
	Point aim;

	union {
		uint iid; // drone, arm, cart
		uint fid; // pipe
	};

	union {
		float radius; // explosion
		float level; // fluid
	};

	int cartWaypointLine = CartWaypoint::Red;
	int cartLine = CartWaypoint::Red;

	int monorailLine = Monorail::Red;
	int monocarLine = Monorail::Red;

	enum class Status {
		None = 0,
		Ok,
		Alert,
		Warning,
	};

	Status status = Status::None;

	Color color;

	struct {
		Point target;
	} turret;

	struct {
		Recipe* recipe = nullptr;
	} crafter;

	struct {
		ConveyorSlot left[3];
		ConveyorSlot right[3];
	} conveyor;

	struct {
		Point relative[3];
	} cartWaypoint;

	struct tubeState {
		uint length = 0;
		minivec<Tube::Thing> stuff;
		Point origin = Point::Zero;
		Point target = Point::Zero;
	};

	tubeState* tube = nullptr;

	struct monorailState {
		std::vector<Monorail::RailLine> railsOut;
	};

	monorailState* monorail = nullptr;

	struct powerpoleState {
		struct Wire {
			Point target = Point::Zero;
			bool render = false;
		};
		minivec<Wire> wires;
		Point point;
	};

	powerpoleState* powerpole = nullptr;

	struct shipyardState {
		Shipyard::Stage stage;
		Point pos;
		Spec* spec;
		bool ghost;
	};

	shipyardState* shipyard;

	static void prepareCaches();

	GuiEntity();
	GuiEntity(uint id);
	GuiEntity(Entity* en);
	GuiEntity(const GuiEntity& other);
	virtual ~GuiEntity();

	Point pos() const;
	Point pos(Point p);
	Point dir() const;
	Point dir(Point p);

	void load(const Entity& en);
	void loadArm();
	void loadLoader();
	void loadCrafter();
	void loadConveyor(const Entity& en);
	void loadNetworker();
	void loadDrone();
	void loadCart();
	void loadCartWaypoint();
	void loadExplosion();
	void loadTurret();
	void loadTube();
	void loadMonorail();
	void loadMonocar();
	void loadPipe();
	void loadComputer();
	void loadRouter();
	void loadPowerPole();
	void loadShipyard();
	Box box() const;
	Box selectionBox() const;
	Box southBox() const;
	Box miningBox() const;
	Box drillingBox() const;
	Box supportBox() const;
	Point ground() const;
	Sphere sphere() const;
	Cuboid cuboid() const;
	Cuboid selectionCuboid() const;
	void updateTransform();

	bool connectable(GuiEntity* other) const;

	void instance();
	void instanceItems();
	void instanceCables();
	void overlayHovering(bool full = true);
	void overlayDirecting();
	void overlayRouting();
	void overlayAlignment();
	void icon();
	Color cartRouteColor(int line);
	Color monorailRouteColor(int line);
	void waypointLine(int line, Point a, Point b, bool wide = false);
	void monorailLink(int line, Rail rail, bool routeOnly = false, bool check = true);

	virtual bool isEnabled() const;
};

struct GuiFakeEntity : GuiEntity {
	Entity::Settings* settings = nullptr;

	GuiFakeEntity(Spec* spec);
	~GuiFakeEntity();

	GuiFakeEntity* getConfig(Entity& en, bool plan = false);
	GuiFakeEntity* setConfig(Entity& en, bool plan = false);
	GuiFakeEntity* move(Point p);
	GuiFakeEntity* move(Point p, Point d);
	GuiFakeEntity* move(float x, float y, float z);
	GuiFakeEntity* floor(float level);
	GuiFakeEntity* rotate();
	GuiFakeEntity* update();

	bool isEnabled() const override;
	GuiFakeEntity& setEnabled(bool state);
};
