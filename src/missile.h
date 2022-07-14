#pragma once

// Missile components move between points and trigger an Explosion.

struct Missile;

#include "slabmap.h"

struct Missile {
	uint id = 0;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Missile,&Missile::id> all;
	static Missile& create(uint id);
	static Missile& get(uint id);

	uint tid = 0;
	Point aim = Point::Zero;
	bool attacking = false;

	void destroy();
	void update();
	void attack(uint tid);
};
