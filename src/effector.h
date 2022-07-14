#pragma once

// Effector components transmit effects to adjacent entities, such as
// overall speed (energy consumption) and recharge rates

struct Effector;

#include "slabmap.h"
#include "entity.h"
#include "activeset.h"

struct Effector {
	uint id;
	Entity *en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Effector,&Effector::id> all;
	static Effector& create(uint id);
	static Effector& get(uint id);

	static inline activeset<Effector*,60> active;
	static inline std::map<uint,miniset<uint>> assist;
	static inline std::set<Spec*> specs;

	enum Type {
		SPEED = 1,
		CHARGE,
	};

	static float effect(uint eid, enum Type type);
	static float speed(uint eid);

	uint tid;

	void destroy();
	void update();
	Point output();
};

