#pragma once

// Turret components scan for enemies and fire Missiles at them.

struct Turret;

#include "sim.h"
#include "slabmap.h"

struct Turret {
	uint id;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Turret,&Turret::id> all;
	static Turret& create(uint id);
	static Turret& get(uint id);

	uint tid; // target
	Point aim;
	bool fire;
	uint cool;
	uint ammoId;
	uint ammoRounds;
	uint64_t pause;
	Health attack;

	void destroy();
	void update();

	bool aimAt(Point o);
	uint ammo();
	Health damage();
};
