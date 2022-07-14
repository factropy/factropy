#pragma once

struct FlightPad;

#include "entity.h"
#include "miniset.h"

struct FlightPad {
	uint id;
	Entity* en;
	Store* store;

	static void reset();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<FlightPad,&FlightPad::id> all;
	static FlightPad& create(uint id);
	static FlightPad& get(uint id);
	void destroy();

	uint claimId;

	bool reserve(uint cid);
	void release(uint cid);
	bool reserved(uint cid = 0);
	Point home();
};


