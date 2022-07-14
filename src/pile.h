#pragma once

// Pile components turn lakes into flat ground after construction,
// and auto remove when fully under ground.

struct Pile;

#include "entity.h"
#include "slabmap.h"
#include "activeset.h"

struct Pile {
	uint id;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Pile,&Pile::id> all;
	static Pile& create(uint id);
	static Pile& get(uint id);

	static inline activeset<uint,180> active;

	void destroy();
	void update();
};
