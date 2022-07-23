#pragma once

// PowerPole components create an electricity grid

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

	miniset<uint> links;

	void destroy();

	bool managed;
	void manage();
	void unmanage();

	Sphere range();
	Box coverage();

	minivec<uint> siblings();
	Point point();

	void connect();
	void disconnect();
};
