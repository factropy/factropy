#pragma once

// Tube components move items between points like elevated single-sided
// conveyor belts that do not need to interact with Arms

struct Tube;
struct TubeSettings;

#include "entity.h"
#include "slabmap.h"
#include "gridmap.h"

struct Tube {
	uint id = 0;
	Entity* en = nullptr;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);
	static std::size_t memory();

	static inline slabmap<Tube,&Tube::id> all;
	static Tube& create(uint id);
	static Tube& get(uint id);

	static std::vector<uint> upgradableGroup(uint cid);

	void destroy();
	void update();

	TubeSettings* settings();
	void setup(TubeSettings*);

	uint length = 0;
	uint next = 0;

	// No attempt is made to run tube segments in any
	// sort of hierarchical order, so a small buffer
	// helps smooth out traffic as items are pased
	// between towers.

	// Towers with conveyor components use the conveyor
	// itself as a buffer instead, as well as for
	// loading/unloading.

	uint accepted = 0;

	enum class Last {
		Tube = 0,
		Belt,
	};

	Last lastInput = Last::Tube;
	Last lastOutput = Last::Tube;

	bool connect(uint nid);
	bool disconnect(uint nid);

	struct Thing {
		uint iid;
		uint offset;
	};

	minivec<Thing> stuff;

	void flush();
	Point origin();
	Point target();
	bool accept(uint iid);

	enum class Mode {
		BeltOrTube,
		BeltOnly,
		TubeOnly,
		BeltPriority,
		TubePriority,
	};

	Mode input = Mode::BeltOrTube;
	Mode output = Mode::BeltOrTube;

	void upgrade(uint uid);

	bool canInputTube();
	bool canOutputTube();
	bool canInputBelt();
	bool canOutputBelt();

	uint inputTube();
	void outputTube(uint iid);
	uint inputBelt();
	void outputBelt(uint iid);
	bool isTubeBlocked();
	bool isBeltBlocked();
	bool tryTubeInTubeOut();
	bool tryTubeInBeltOut();
	bool tryBeltInBeltOut();
	bool tryBeltInTubeOut();
	bool tryBeltBackedTubeOut();
	bool tryTubeBackedBeltOut();
	bool tryAnyInAnyOut();
	bool tryAnyInBeltOut();
	bool tryBeltInAnyOut();
	bool tryAnyInTubeOut();
	bool tryTubeInAnyOut();
};

struct TubeSettings {
	Point target;
	Tube::Mode input;
	Tube::Mode output;
	TubeSettings(Tube& tube);
};