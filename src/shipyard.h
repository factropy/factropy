#pragma once

struct Shipyard;

#include "slabmap.h"
#include "item.h"
#include "fluid.h"
#include "entity.h"
#include "goal.h"
#include "activeset.h"
#include <map>
#include <vector>

struct Shipyard {
	uint id;
	Entity* en;
	Crafter* crafter;
	Spec* ship;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Shipyard,&Shipyard::id> all;
	static Shipyard& create(uint id);
	static Shipyard& get(uint id);

	enum class Stage {
		Start,
		Build,
		RollOut,
		Launch,
		RollIn,
	};

	Stage stage = Stage::Start;
	int delta = 1;
	uint completed = 0;

	bool ready(Spec* spec);
	void create(Spec* spec);
	void complete(Spec* spec);

	void start();
	void build();
	void rollOut();
	void launch();
	void rollIn();
	Point shipPos();
	bool shipGhost();

	void destroy();
	void update();
};

