#pragma once

// Venters are rate-limited fluid sinks.

struct Venter;

#include "slabmap.h"
#include "entity.h"

struct Venter {
	uint id;
	Entity *en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Venter,&Venter::id> all;
	static Venter& create(uint id);
	static Venter& get(uint id);

	void destroy();
	void update();
};
