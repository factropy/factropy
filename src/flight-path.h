#pragma once

// Cooperative flight component

struct FlightPath;

#include "entity.h"
#include "sky.h"

struct FlightPath {
	uint id;
	Entity* en;

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<FlightPath,&FlightPath::id> all;
	static FlightPath& create(uint id);
	static FlightPath& get(uint id);

	void destroy();
	void update();

	uint64_t pause;

	bool moving;
	Sky::Path path;
	int step;
	bool request;

	float fueled;
	float speed;

	Point destination;
	uint64_t arrived;

	Point origin;
	uint64_t departed;

	Point orient;

	void depart(Point to, Point at);
	bool move(Point dir, float targetSpeed);
	void cancel();

	bool departing();
	bool flying();
	bool done();
};
