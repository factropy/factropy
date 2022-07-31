#pragma once

// PowerPole components create the electricity grid

struct PowerPole;

#include "slabmap.h"

struct PowerPole {
	uint id;
	Entity* en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<PowerPole,&PowerPole::id> all;
	static PowerPole& create(uint id);
	static PowerPole& get(uint id);

	static inline gridmap<64,PowerPole*> gridCoverage;
	static bool covered(Box box);
	static bool powered(Box box);

	static inline miniset<PowerPole*> roots;
	static inline activeset<PowerPole*,60> queue;

	uint root = 0;
	miniset<uint> links;

	void destroy();

	bool managed;
	void manage();
	void unmanage();

	Cylinder range();
	Box coverage();

	minivec<uint> siblings();
	Point point();

	void connect();
	void disconnect();
};
