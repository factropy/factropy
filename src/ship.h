#pragma once

struct Ship;

#include "entity.h"
#include "slabmap.h"

struct Ship {
	uint id;
	Entity* en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Ship,&Ship::id> all;
	static Ship& create(uint id);
	static Ship& get(uint id);

	enum class Stage {
		Start,
		Lift,
		Fly,
	};

	Stage stage = Stage::Start;
	minivec<Point> flight;
	float speed;
	float elevate;

	void destroy();
	void update();
};


