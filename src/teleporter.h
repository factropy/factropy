#pragma once

// Teleporter components move shipments of items between points.

struct Teleporter;

#include "entity.h"

struct Teleporter {
	uint id;
	Entity* en;
	Store* store;

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Teleporter,&Teleporter::id> all;
	static Teleporter& create(uint id);
	static Teleporter& get(uint id);

	void destroy();
	void update();

	bool trigger;
	bool working;
	float progress;
	Energy energyUsed;
	uint completed;
	minivec<Stack> shipment;
	uint partner;
};

