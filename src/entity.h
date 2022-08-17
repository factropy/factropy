#pragma once

// The core of the Entity Component System. All entities have an Entity struct
// with a unique uint ID. Component structs store the ID to associate with an
// entity.

struct Entity;

#include <set>
#include <unordered_set>
#include <map>
#include <vector>
#include "slabmap.h"
#include "gridmap.h"
#include "gridagg.h"
#include "spec.h"
#include "recipe.h"
#include "world.h"
#include "part.h"
#include "mat4.h"
#include "point.h"
#include "box.h"
#include "sphere.h"
#include "cylinder.h"
#include "cuboid.h"
#include "curve.h"
#include "rail.h"
#include "ghost.h"

// Components

#include "pile.h"
#include "explosive.h"
#include "store.h"
#include "arm.h"
#include "conveyor.h"
#include "unveyor.h"
#include "loader.h"
#include "balancer.h"
#include "tube.h"
#include "pipe.h"
#include "crafter.h"
#include "venter.h"
#include "effector.h"
#include "launcher.h"
#include "drone.h"
#include "missile.h"
#include "explosion.h"
#include "depot.h"
#include "vehicle.h"
#include "cart.h"
#include "cart-stop.h"
#include "cart-waypoint.h"
#include "burner.h"
#include "charger.h"
#include "generator.h"
#include "turret.h"
#include "computer.h"
#include "router.h"
#include "networker.h"
#include "zeppelin.h"
#include "flight-pad.h"
#include "flight-path.h"
#include "flight-logistic.h"
#include "teleporter.h"
#include "monorail.h"
#include "monocar.h"
#include "source.h"
#include "powerpole.h"
#include "electricity.h"
#include "shipyard.h"
#include "ship.h"

struct Entity {
	Spec* spec;     // Specification (prototype/class/behaviour)
	uint id;        // Unique id during an entity's lifetime. May be reused
	uint16_t state; // Animation state, an index into spec->states
	uint16_t flags; // Flags below
	//Point _pos;     // Absolute position in the world
	struct { int x; int y; int z; } _pos;
	//Point _dir;     // Normalized direction vector relative to position
	struct { float x; float y; float z; } _dir;
	Health health;

	union {
		Conveyor* conveyor;
	} cache;

	// Flags
	static const uint32_t GHOST = 1<<0;
	static const uint32_t CONSTRUCTION = 1<<1;
	static const uint32_t DECONSTRUCTION = 1<<2;
	static const uint32_t ENABLED = 1<<3;
	static const uint32_t GENERATING = 1<<4;
	static const uint32_t MARKED1 = 1<<5;
	static const uint32_t MARKED2 = 1<<6;
	static const uint32_t BLOCKED = 1<<7;
	static const uint32_t RULED = 1<<8;
	static const uint32_t PERMANENT = 1<<9;

	// Spatial indexes of entity axis-aligned bounding boxes
	static const uint32_t GRID = 16;
	static inline gridmap<GRID,Entity*> grid;
	static inline gridmap<GRID,Entity*> gridStores;
	static inline gridmap<GRID,Entity*> gridStoresFuel;
	static inline gridmap<GRID,Entity*> gridConveyors;
	static inline gridmap<GRID,Entity*> gridPipes;
	static inline gridagg<GRID,uint64_t> gridPipesChange;
	static inline gridmap<GRID,Entity*> gridEnemyTargets;
	static inline gridmap<GRID,Entity*> gridCartWaypoints;
	static inline gridmap<GRID,Entity*> gridFlightPaths;
	static inline gridmap<GRID,Entity*> gridTubes;
	static inline gridmap<GRID,Entity*> gridSlabs;

	static const uint32_t RENDER = 128;
	static inline gridmap<RENDER,Entity*> gridRender;
	static inline gridmap<RENDER,Entity*> gridRenderTubes;
	static inline gridmap<RENDER,Entity*> gridRenderMonorails;

	static const uint32_t DEPOT = 64;
	static inline gridmap<DEPOT,Entity*> gridGhosts;
	static inline gridmap<DEPOT,Entity*> gridDamaged;
	static inline gridmap<DEPOT,Entity*> gridStoresLogistic;

	static inline std::set<uint> enemyTargets;
	static inline std::map<uint,uint> repairActions;

	// Entities may only be destroyed between ticks, never during. See ::preTick()
	static inline miniset<uint> removing;
	static inline miniset<uint> exploding;
	static inline miniset<uint> damaged;

	// Some Entity fields cannot change during a tick
	static inline std::atomic<bool> mutating = {true};

	// Every extant entity is tracked here. See ::get() and ::exists()
	static inline slabmap<Entity,&Entity::id> all;
	static inline uint sequence = 0;
	static uint next(); // allocate an unused id

	// Player-controlled names. spec->named
	static inline std::map<uint,std::string> names;

	// Player-controlled names. Colored parts
	static inline std::map<uint,Color> colors;

	// debug
	static std::size_t memory();

	// Create an entity and all components defined by its Spec. New entity will
	// not be a ghost unless you explicitly ::construct()
	static Entity& create(uint id, Spec* spec);
	static bool exists(uint id);
	static Entity& get(uint id);
	static Entity* find(uint id);

	static void saveAll(const char* name, channel<bool,3>* tickets);
	static void loadAll(const char* name);
	static void reset();
	static void preTick();
	static void postTick();

	// Would an entity of spec, at pos, facing dir, fit on the map?
	static bool fits(Spec *spec, Point pos, Point dir);

	// Spatial queries to find entities
	static std::vector<Entity*> intersecting(const Cuboid& cuboid);

	template <class G>
	static std::vector<Entity*> intersecting(const Cuboid& cuboid, const G& gm) {
		std::vector<Entity*> hits;
		for (Entity* en: gm.search(cuboid.box.sphere())) {
			if (en->cuboid().intersects(cuboid)) {
				hits.push_back(en);
			}
		}
		return hits;
	}

	static std::vector<Entity*> intersecting(const Box& box);

	template <class G>
	static std::vector<Entity*> intersecting(const Box& box, const G& gm) {
		std::vector<Entity*> hits;
		for (Entity* en: gm.search(box)) {
			if (en->box().intersects(box)) {
				hits.push_back(en);
			}
		}
		return hits;
	}

	static std::vector<Entity*> intersecting(const Sphere& sphere);

	template <class G>
	static std::vector<Entity*> intersecting(const Sphere& sphere, const G& gm) {
		std::vector<Entity*> hits;
		for (Entity* en: gm.search(sphere)) {
			if (en->sphere().intersects(sphere)) {
				hits.push_back(en);
			}
		}
		return hits;
	}

	static std::vector<Entity*> intersecting(const Cylinder& cylinder);

	template <class G>
	static std::vector<Entity*> intersecting(const Cylinder& cylinder, const G& gm) {
		std::vector<Entity*> hits;
		for (Entity* en: gm.search(cylinder.box())) {
			if (en->box().intersects(cylinder)) {
				hits.push_back(en);
			}
		}
		return hits;
	}

	static std::vector<Entity*> intersecting(Point pos, float radius);
	static std::vector<Entity*> enemiesInRange(Point pos, float radius);
	static Entity* at(Point p); // intersecting[0]
	static Entity* at(Point p, gridmap<GRID,Entity*>& gm); // intersecting[0]

	static bool isLand(Point p);
	static bool isLand(Box b);

	Point pos() const;
	Point pos(Point p);
	Point dir() const;
	Point dir(Point p);

	// Bit flags setters/getters
	bool isGhost() const;
	Entity& setGhost(bool state);
	bool isConstruction() const;
	Entity& setConstruction(bool state);
	bool isDeconstruction() const;
	Entity& setDeconstruction(bool state);
	bool isEnabled() const;
	Entity& setEnabled(bool state);
	bool isBlocked() const;
	Entity& setBlocked(bool state);
	bool isRuled() const;
	Entity& setRuled(bool state);
	bool isPermanent() const;
	Entity& setPermanent(bool state);
	bool isGenerating() const;
	Entity& setGenerating(bool state);
	bool isMarked1() const;
	Entity& setMarked1(bool state);
	bool isMarked2() const;
	Entity& setMarked2(bool state);
	Entity& clearMarks();

	// If spec->named
	std::string name() const;
	bool rename(std::string n);
	std::string title() const;

	Color color() const;
	Color color(Color c);

	// AABB axis aligned bounding box
	Box box() const;

	// Bounding sphere of the AABB
	Sphere sphere() const;

	// Usually == AABB so far, might go away
	Box miningBox() const;
	Box drillingBox() const;

	// Centroid of the AABB on the ground plane
	Point ground() const;
	Sphere groundSphere() const;

	// Bounding cuboid
	Cuboid cuboid() const;
	Cuboid selectionCuboid() const;

	// Adjust direction entity is facing
	Entity& look(Point); // turn to face a direction relative to position
	Entity& lookAt(Point); // turn to face an absolute point on the map
	bool lookAtPivot(Point, float speed = 0.01f); // smooth pivoting, see turret
	bool lookingAt(Point);

	// Movement. These update the spatial grid index and trigger manage/unmanage
	// on components where relevant
	Entity& move(Point p);
	Entity& move(Point p, Point d);
	Entity& move(float x, float y, float z);
	Entity& bump(Point p, Point d);
	Entity& place(Point p, Point d);

	// Player-controlled N/E/S/W entity rotation
	Entity& rotate();

	// Destroy an entity now. You probably don't generally want this, use ::remove
	// to destroy atomically on a tick boundary
	void destroy();
	void remove();
	void explode();

	// Spatial grid index
	bool specialIndexable();
	Entity& index();
	Entity& unindex();

	// Per-component registration/deregistratin of non-ghost entities
	Entity& manage();
	Entity& unmanage();

	// Ghost/Extant handling
	Entity& construct();   // Become a ghost requesting construction materials
	Entity& upgrade();     // If upgradeable, become a ghost requesting additional materials
	static void upgradeCascade(uint from); // Upgrade connected entities of the same type
	Entity& deconstruct(bool items = false); // Become a ghost exporting construction materials
	Entity& materialize(); // Stop being a ghost and become an extant entity
	Entity& complete();    // Consume construction materials

	// Energy consumption and generation. How an entity accesses energy -- burner,
	// thermal fluid, electricity etc -- is defined by the spec. Other components
	// only care that they *can* consume energy not how they do it.
	Energy consume(Energy e);
	float consumeRate(Energy e);

	// Apply damage. May result in ::unmanage() and ::remove()
	void damage(Health hits);
	void repair(Health hits);

	struct Settings {
		StoreSettings* store = nullptr;
		CrafterSettings* crafter = nullptr;
		ArmSettings* arm = nullptr;
		LoaderSettings* loader = nullptr;
		BalancerSettings* balancer = nullptr;
		PipeSettings* pipe = nullptr;
		CartSettings* cart = nullptr;
		CartStopSettings* cartStop = nullptr;
		CartWaypointSettings* cartWaypoint = nullptr;
		NetworkerSettings* networker = nullptr;
		TubeSettings* tube = nullptr;
		MonorailSettings* monorail = nullptr;
		RouterSettings* router = nullptr;
		bool enabled = true;
		bool applicable = false;
		Color color = 0xffffffff;
		Settings();
		Settings(Entity& en);
		~Settings();
	};

	Settings* settings();
	bool setup(Settings* settings);

	// Access entity components
	Ghost& ghost() const;
	Store& store() const;
	std::vector<Store*> stores() const;
	Crafter& crafter() const;
	Venter& venter() const;
	Effector& effector() const;
	Launcher& launcher() const;
	Vehicle& vehicle() const;
	Cart& cart() const;
	CartStop& cartStop() const;
	CartWaypoint& cartWaypoint() const;
	Arm& arm() const;
	Conveyor& conveyor() const;
	Unveyor& unveyor() const;
	Loader& loader() const;
	Balancer& balancer() const;
	Pipe& pipe() const;
	Drone& drone() const;
	Missile& missile() const;
	Explosion& explosion() const;
	Depot& depot() const;
	Burner& burner() const;
	Charger& charger() const;
	Generator& generator() const;
	Turret& turret() const;
	Computer& computer() const;
	Router& router() const;
	Pile& pile() const;
	Explosive& explosive() const;
	Networker& networker() const;
	Zeppelin& zeppelin() const;
	FlightPad& flightPad() const;
	FlightPath& flightPath() const;
	FlightLogistic& flightLogistic() const;
	Teleporter& teleporter() const;
	Monorail& monorail() const;
	Monocar& monocar() const;
	Tube& tube() const;
	Source& source() const;
	PowerPole& powerpole() const;
	ElectricityProducer& electricityProducer() const;
	ElectricityConsumer& electricityConsumer() const;
	ElectricityBuffer& electricityBuffer() const;
	Shipyard& shipyard() const;
	Ship& ship() const;
};
