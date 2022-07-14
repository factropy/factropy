#pragma once

struct Zeppelin;

#include "entity.h"
#include "path.h"

struct Zeppelin {
	uint id;
	Point target;
	float speed;
	float altitude;
	bool moving;

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Zeppelin,&Zeppelin::id> all;
	static Zeppelin& create(uint id);
	static Zeppelin& get(uint id);

	void destroy();
	void update();
	void flyOver(Point p);
	float flightPathAltitude(Point a, Point b);
};
