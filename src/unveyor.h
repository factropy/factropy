#pragma once

// Unveyor (underground conveyor) components link up with Conveyors to allow
// short distance underground belts.

struct Unveyor;

#include "entity.h"
#include "conveyor.h"
#include "slabmap.h"

struct Unveyor {
	uint id;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Unveyor,&Unveyor::id> all;
	static Unveyor& create(uint id);
	static Unveyor& get(uint id);

	uint left;
	uint right;

	uint partner;
	bool entry;

	void destroy();
	void update();
	void manage();
	void unmanage();
	void link();
	void unlink();

	Box range();
};
