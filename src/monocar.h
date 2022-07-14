#pragma once

// Monorail car components navigate Monorail towers carying shipping containers.

struct Monocar;

#include "entity.h"
#include "slabmap.h"
#include "gridmap.h"

struct Monocar {
	uint id = 0;
	Entity* en = nullptr;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Monocar,&Monocar::id> all;
	static Monocar& create(uint id);
	static Monocar& get(uint id);

	void destroy();
	void update();

	Monorail* getTower(Point pos);

	enum class State {
		Start = 0,
		Acquire,
		Travel,
		Stop,
		Unload,
		Unloading,
		Load,
		Loading,
	};

	State state = State::Stop;

	void start();
	void acquire();
	void travel();
	void stop();
	void unload();
	void unloading();
	void load();
	void loading();

	Point positionCargo();

	uint tower = 0;
	uint container = 0;
	float speed = 0;
	uint iid = 0;
	bool blocked = false;

	Point dirArrive = Point::Zero;
	Point dirDepart = Point::Zero;

	struct Bogie {
		Point pos = Point::Zero;
		Point dir = Point::Zero;
		Point arrive = Point::Zero;
		uint step = 0;
	};

	Bogie bogieA;
	Bogie bogieB;

	Rail rail;
	minivec<Point> steps;

	int line;
	minivec<Signal> constants;
	minimap<Signal,&Signal::key> signals();

	struct {
		bool stop = false;
		bool move = false;
	} flags;
};

