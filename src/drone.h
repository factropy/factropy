#pragma once

// Drone components are deployed by Depots to move items between Stores.

struct Drone;

#include "item.h"
#include "slabmap.h"
#include "minivec.h"

struct Drone {
	uint id;
	Entity* en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Drone,&Drone::id> all;
	static Drone& create(uint id);
	static Drone& get(uint id);

	static float flightPathAltitude(Point a, Point b);

	static inline float speedFactor = 1.0f;

	struct flightPath {
		Point origin;
		Point target;
		float altitude;
		uint64_t until;
	};

	static inline std::map<Point,std::map<Point,flightPath>> cache;
	static inline minivec<flightPath*> queue;

	enum Stage {
		ToSrc = 1,
		ToDst,
		ToDep,
		Stranded,
	};

	uint iid;
	uint dep;
	uint src;
	uint dst;
	Stack stack;
	enum Stage stage;
	float altitude;
	float range;
	bool srcGhost;
	bool dstGhost;
	bool repairing;

	void destroy();
	void update();
	void rangeCheck(Point home);
	bool travel(Entity* te);
};
