#pragma once

// Depot components deploy Drones to construct and deconstruct Ghosts, and supply
// Stores within their vicinity. Depots don't form networks or send drones over
// long distances, but they do co-operate within shared areas to load-balance
// drone activity.

struct Depot;

#include "slabmap.h"
#include "miniset.h"
#include "entity.h"
#include "networker.h"
#include "activeset.h"

struct Depot {
	uint id;
	bool construction;
	bool deconstruction;
	bool recall;
	bool network;
	Entity* en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Depot,&Depot::id> all;
	static Depot& create(uint id);
	static Depot& get(uint id);

	static inline activeset<Depot*,5> hot;
	static inline activeset<Depot*,60> cold;

	struct EntityStore {
		uint id = 0;
		Entity* en = nullptr;
		Store* store = nullptr;
		Point pos = Point::Zero;
	};

	static inline std::map<Networker::Network*,miniset<EntityStore>> networkedStores;

	miniset<uint> drones;
	miniset<uint> batteries;
	uint64_t nextDispatch;

	miniset<uint> storesCache;
	uint64_t nextStoreRefresh;

	miniset<uint> ghostsCache;
	uint64_t nextGhostRefresh;

	enum {
		FlightRepair = 1<<0,
	};

	void destroy();
	void update();
	void dispatch(uint dep, uint src, uint dst, Stack stack, uint flags = 0);
};
