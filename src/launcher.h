#pragma once

// Launcher components consume item shipments and fuel. Similar to a Crafter
// but without results and with a specific set of animation states.

struct Launcher;

#include "slabmap.h"
#include "item.h"
#include "fluid.h"
#include "entity.h"
#include "goal.h"
#include "activeset.h"
#include <map>
#include <vector>

struct Launcher {
	uint id;
	Entity* en;
	Store* store;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Launcher,&Launcher::id> all;
	static Launcher& create(uint id);
	static Launcher& get(uint id);

	bool working;
	bool activate;
	uint completed;
	float progress;
	miniset<uint> cargo;

	enum class Monitor {
		Network = 0,
	};

	Monitor monitor;
	Signal::Condition condition;
	bool checkCondition();

	void destroy();
	void update();
	bool fueled();
	bool ready();
	minimap<Amount,&Amount::fid> fuelRequired();
	minimap<Amount,&Amount::fid> fuelAccessable();
	minivec<uint> pipes();
};
