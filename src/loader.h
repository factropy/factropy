#pragma once

// Loader components link up with Conveyors to load/unload Stores. They
// are essentially a subset of Arm functionality and so correspondingly
// less flexible but also faster and more efficient.

struct Loader;
struct LoaderSettings;

#include "entity.h"
#include "conveyor.h"
#include "slabmap.h"
#include "signal.h"
#include "activeset.h"

struct Loader {
	uint id = 0;
	Entity* en = nullptr;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);
	static std::size_t memory();

	static inline slabmap<Loader,&Loader::id> all;
	static Loader& create(uint id);
	static Loader& get(uint id);

	uint storeId = 0;
	uint64_t pause = 0;
	miniset<uint> filter;
	bool loading = false;
	bool ignore = false;

	struct {
		Point point = Point::Zero;
		uint64_t refresh = 0;
	} cache;

	enum class Monitor {
		Store = 0,
		Network,
	};

	Monitor monitor;
	Signal::Condition condition;

	void destroy();
	void update();

	LoaderSettings* settings();
	void setup(LoaderSettings*);

	Point point();
	bool checkCondition();

	Stack transferBeltToStore(Store& dst, Stack stack);
	Stack transferStoreToBelt(Store& src);
};

struct LoaderSettings {
	miniset<uint> filter;
	Loader::Monitor monitor;
	Signal::Condition condition;
	LoaderSettings(Loader& arm);
};
