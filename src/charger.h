#pragma once

// Charger components buffer electricity.

struct Charger;

#include "slabmap.h"
#include "store.h"
#include "electricity.h"

struct Charger {
	uint id;
	Entity* en;
	static void tick();
	static void reset();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Charger,&Charger::id> all;
	static Charger& create(uint id);
	static Charger& get(uint id);

	Energy energy;
	Energy buffer;
	bool connected;

	void destroy();
	Energy consume(Energy e);
	Energy chargeRate();
	void charge();
	float level();
	bool powered();
};
