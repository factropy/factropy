#pragma once

// The Enemy system uses the scenario to spawn enemy entities like missiles.

struct Enemy;

#include "entity.h"

struct Enemy {
	static inline bool enable = true;
	static inline bool trigger = false;
	static void tick();
	static void spawn(localvec<uint>& targets, Point dir);
};