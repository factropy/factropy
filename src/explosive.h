#pragma once

// Explosive components turn hills into flat ground after construction,
// and auto-remove when detonated.

struct Explosive;

#include "entity.h"
#include "slabmap.h"
#include "activeset.h"

struct Explosive {
	uint id;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Explosive,&Explosive::id> all;
	static Explosive& create(uint id);
	static Explosive& get(uint id);

	static inline activeset<uint,180> active;

	void destroy();
	void update();
	float radius();
};
