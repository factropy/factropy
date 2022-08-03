#pragma once

// Spec(ifications) describe everything about a type of entity. They are
// complex and huge so that entity components can be simple and small.

struct Spec;
typedef int Health;

#include "glm-ex.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include "point.h"
#include "mat4.h"
#include "item.h"
#include "fluid.h"
#include "part.h"
#include "volume.h"
#include "mass.h"
#include "energy.h"
#include "time-series.h"
#include "recipe.h"
#include "rail.h"

struct Spec {

	enum Place {
		Footings = 0,
		Land = 1<<1,
		Water = 1<<2,
		Hill = 1<<3,
		Monorail = 1<<4,
	};

	struct Footing {
		Place place;
		Point point;
	};

	static inline std::map<std::string,Spec*> all;
	static Spec* byName(std::string name);

	static void saveAll(const char *path);
	static void loadAll(const char *path);
	static void reset();

	struct {
		int ghosts = 0;
		int extant = 0;
		int render = 0;
	} count;

	std::string name;
	std::string title;
	std::string wiki;
	std::vector<Part*> parts;
	std::vector<std::vector<Mat4>> states;
	std::vector<std::vector<bool>> statesShow;
	float iconD;
	float iconV;

	std::string toolbar = "z";

	Health health;
	bool coloredAuto;
	bool coloredCustom;
	Color color;

	// Auto generated
	int highLOD;

	bool tipStorage;

	bool licensed;
	bool align;
	bool shunt;
	bool enable;
	bool junk;
	bool named;
	bool block;
	bool pile;
	bool underground;
	bool deconstructable;
	bool raise;
	bool slab;

	bool direct;
	bool directRevert;

	bool forceDelete;
	bool forceDeleteStore;
	bool collideBuild;

	bool rotateGhost;
	bool rotateExtant;
	std::vector<Point> rotations;

	Box collision;
	Box selection;

	int place;
	bool placeOnHill;
	float placeOnHillPlatform;
	std::vector<Footing> footings;

	float costGreedy;
	float clearance;

	// the entity exploding
	bool explodes;
	Spec* explosionSpec;

	// the resulting explosion
	bool explosion;
	float explosionRate;
	float explosionRadius;
	Health explosionDamage;

	// hill explosive
	bool explosive;

	// store
	bool store;
	Mass capacity;
	bool storeSetLower;
	bool storeSetUpper;
	bool storeMagic;
	bool storeAnything;
	bool storeAnythingDefault;
	bool storeUpgradePreserve;
	std::vector<Stack> supplies;
	// store drones
	bool logistic;
	bool construction;
	bool overflow;
	// store arms
	int priority;

	bool showItem;
	Point showItemPos;

	// tanks
	bool tank;
	Mass tankCapacity;

	bool crafter;
	bool crafterMiner;
	bool crafterSmelter;
	bool crafterChemistry;
	bool crafterProgress;
	bool crafterOnOff;
	bool crafterState;
	bool crafterManageStore;
	bool crafterShowTab;
	bool crafterTransmitResources;
	float crafterRate;
	Energy crafterEnergyConsume;
	std::set<std::string> recipeTags;
	Recipe* crafterRecipe;
	bool crafterOutput;
	Point crafterOutputPos;

	bool venter;
	Mass venterRate;

	bool launcher;
	uint launcherInitialState;
	std::vector<Amount> launcherFuel;

	bool arm;
	Point armInput;
	Point armOutput;
	float armSpeed;

	bool depot;
	bool depotAssist;
	bool depotFixed;
	float depotRange;
	uint depotDrones;
	Spec* depotDroneSpec;
	Energy depotDispatchEnergy;

	bool drone;
	float droneSpeed;
	Point dronePoint;
	float dronePointRadius;
	uint droneCargoSize;

	// Construction materials to be delivered by drone
	std::vector<Stack> materials;
	float materialsMultiplyHill;

	bool consumeFuel;
	bool burnerState;
	std::string consumeFuelType;
	bool consumeElectricity;
	bool consumeElectricityAnywhere;
	bool consumeThermalFluid;
	bool consumeCharge;
	Energy consumeChargeBuffer;
	Energy consumeChargeRate;
	bool consumeChargeEffect;
	bool consumeMagic;
	Energy energyConsume;
	Energy energyDrain;
	bool generateElectricity;
	bool bufferElectricity;
	bool bufferElectricityState;
	Energy bufferDischargeRate;
	Energy energyGenerate;
	bool generatorState;

	bool powerpole;
	bool powerpoleRoot;
	float powerpoleRange;
	float powerpoleCoverage;
	Point powerpolePoint;

	bool effector;
	float effectorElectricity;
	float effectorFuel;
	float effectorCharge;
	Energy effectorEnergyDrain;

	bool vehicle;
	bool vehicleStop;
	bool vehicleWaitActivity;

	bool pipe;
	bool pipeHints;
	std::vector<Point> pipeConnections;
	std::vector<Point> pipeInputConnections;
	std::vector<Point> pipeOutputConnections;
	Liquid pipeCapacity;
	bool pipeUnderground;
	float pipeUndergroundRange;
	bool pipeUnderwater;
	float pipeUnderwaterRange;
	bool pipeValve;

	struct PipeLevel {
		Box box;
		Mat4 trx;
	};

	std::vector<PipeLevel> pipeLevels;

	bool turret;
	bool turretLaser;
	float turretRange;
	float turretPivotSpeed;
	uint turretCooldown;
	float turretDamage;
	Point turretPivotPoint;
	Point turretPivotFirePoint;
	Color turretTrail;
	bool turretStateAlternate;
	bool turretStateRevert;

	bool missile;
	float missileSpeed;
	bool missileBallistic;

	bool enemy;
	bool enemyTarget;

	bool biter;
	float biterRange;
	float biterSpeed;
	Health biterDamage;

	// Conveyors
	bool conveyor;
	Point conveyorInput;
	Point conveyorOutput;
	Point conveyorCentroid;
	uint conveyorSlotsLeft;
	uint conveyorSlotsRight;
	std::vector<Mat4> conveyorTransformsLeft;
	std::vector<Mat4> conveyorTransformsRight;
	std::array<std::vector<Mat4>,4> conveyorTransformsLeftCache; // gui-entity
	std::array<std::vector<Mat4>,4> conveyorTransformsRightCache; // gui-entity
	std::array<std::vector<Mat4>,4> conveyorTransformsMultiCache; // gui-entity
	std::map<uint,std::vector<Part*>> conveyorPartMultiCache; // gui-entity
	Energy conveyorEnergyDrain;

	// Underground conveyors
	bool unveyor;
	bool unveyorEntry;
	float unveyorRange;
	Spec* unveyorPartner;

	bool loader;
	bool loaderUnload;
	Point loaderPoint;

	// Balancers
	bool balancer;
	Point balancerLeft;
	Point balancerRight;

	bool computer;
	uint computerDataStackSize;
	uint computerReturnStackSize;
	uint computerRAMSize;
	uint computerROMSize;
	uint computerCyclesPerTick;

	bool router;

	bool networker;
	bool networkHub;
	float networkRange;
	Point networkWifi;
	int networkInterfaces;

	bool cart;
	bool cartStop;
	bool cartWaypoint;
	float cartSpeed;
	uint cartWait;
	Point cartItem;
	Part* cartRoute;

	bool windTurbine;
	bool starship;

	bool zeppelin;
	float zeppelinAltitude;
	float zeppelinSpeed;

	bool flightPad;
	bool flightPadDepot;
	bool flightPadSend;
	bool flightPadRecv;
	Point flightPadHome;
	bool flightPath;
	float flightPathSpeed;
	float flightPathClearance;
	bool flightLogistic;
	uint flightLogisticRate;

	bool teleporter;
	bool teleporterSend;
	bool teleporterRecv;
	Energy teleporterEnergyCycle;

	bool ruled;
	bool status;
	Point beacon;

	bool tube;
	float tubeOrigin;
	uint tubeSpan;
	uint tubeSpeed;
	Part* tubeGlass;
	Part* tubeRing;
	Part* tubeChevron;

	bool monorail;
	bool monorailStop;
	Point monorailArrive;
	Point monorailDepart;
	uint monorailSpan;
	float monorailAngle;
	Point monorailStopUnload;
	Point monorailStopEmpty;
	Point monorailStopFill;
	Point monorailStopLoad;
	bool monorailContainer;

	bool monocar;
	Point monocarContainer;
	float monocarBogieA;
	float monocarBogieB;
	Part* monocarRoute;
	float monocarSpeed;

	bool source;
	Item* sourceItem;
	uint sourceItemRate;
	Fluid* sourceFluid;
	uint sourceFluidRate;

	bool once;
	bool done;

	bool field;

	// Spec is placed rapidly aligned in series, like straight belts.
	// Plans will prevent accidentally placing an identical entity
	// out of line for a short while after the last placement
	bool snapAlign;

	// Redirect to another spec and player uses pipette
	Spec* pipette;

	// Spec to switch to when player uses the cycle key
	// Example is conveyor -> conveyor-right -> conveyor-left -> conveyor
	Spec* cycle;

	// Cycling to this spec should trigger a automatic reversal of entity direction
	// Example is underground belt entry -> underground belt exit, which work intuitively as a toggling
	// of only the belt component's direction, not the whole entity
	bool cycleReverseDirection;

	// Spec to switch to after placed once
	// Example is underground belt entry -> underground belt exit with same
	// direction
	Spec* follow;
	Spec* followConveyor; // only single belt corner placements

	// Opposite of cycleReverseDirection
	bool followReverseDirection;

	Spec* downward;
	// Spec to switch to on pg-down
	// Example is belt to underground belt of same tier

	Spec* upward;
	// Spec to switch to on pg-up
	// Example is underground belt to belt of same tier

	// Spec appears in the build popup menu
	// Example is conveyor-right and conveyor-left which are effectively always
	// accessed via the normal straight conveyor and simply cycled thtough.
	bool build;

	// Spec can be selected in game via mouse-over or selection box
	bool select;

	// Spec can be part of a Plan (requires select). This is separate to select
	// because some entities are necessary to interact with but never placed
	bool plan;

	// Spec can be part of a single-entity Plan such as pipette.
	bool clone;

	// Redirect stats to another spec
	// Example is grouping all conveyor specs under one for tracking purposes
	Spec* statsGroup;
	TimeSeries energyConsumption;
	TimeSeries energyGeneration;

	Spec* upgrade;
	std::set<Spec*> upgradeCascade;

	Spec(std::string name);
	~Spec();
	Point aligned(Point p, Point dir) const;
	bool alignStrict(Point p, Point dir) const;

	Box box(Point pos, Point dir, Box vol) const;
	Box southBox(Point pos) const;
	Box southBox(Point pos, Box type) const;
	Box placeOnHillBox(Point pos) const;
	void setCornerSupports() const;

	bool operable() const;
	bool directable() const;

	Health damage(uint ammoId = 0) const;

	static std::vector<Point> relativePoints(const std::vector<Point> points, const Mat4 rotation, const Point position);
	std::vector<Stack> constructionMaterials(float height = 0);

	Rail railTo(Point posA, Point dirA, Spec* specB, Point posB, Point dirB);
	bool railOk(Rail& rail);
};

