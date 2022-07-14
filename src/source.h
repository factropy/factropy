#pragma once

// Source components generate items or fluids

struct Source;

#include "slabmap.h"
#include "entity.h"

struct Source {
	uint id;
	Entity *en;
	Pipe* pipe;
	Store* store;

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Source,&Source::id> all;
	static Source& create(uint id);
	static Source& get(uint id);

	void destroy();
	void update();
};

