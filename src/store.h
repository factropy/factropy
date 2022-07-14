#pragma once

// Store components are general-purpose configurable item inventories. They
// allow setting lower and upper limits per item and can function as dumb
// containers, logistic requester/suppliers, burner fuel tanks, crafter
// input/output buffers etc. Arms and Drones work together with Stores to
// move items around co-operatively.

//                lower                upper                    capacity
// |----------------|--------------------|-------------------------|
//     requester           provider            active provider
//     overflow            overflow              no overflow

struct Store;
struct StoreSettings;
struct Entity;

#include "slabmap.h"
#include "minivec.h"
#include "miniset.h"
#include "item.h"
#include "mass.h"
#include "signal.h"
#include <vector>
#include <set>

struct Store {
	uint id;
	Entity* en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);
	static std::size_t memory();

	static inline slabmap<Store,&Store::id> all;
	static Store& create(uint id, uint sid, Mass cap);
	static Store& get(uint id);

	struct Level {
		uint iid = 0;
		uint lower = 0;
		uint upper = 0;
	};

	struct Delivery {
		uint iid = 0;
		uint promised = 0;
		uint reserved = 0;
	};

	uint sid;
	uint64_t activity;
	Mass contents;
	Mass contentsPredict;
	Mass capacity;
	minivec<Stack> stacks;
	minivec<Level> levels;
	minivec<Delivery> deliveries;
	miniset<uint> drones;
	miniset<uint> arms;
	std::string fuelCategory;
	bool fuel;
	bool ghost;
	bool magic;
	bool transmit;
	bool anything;
	bool overflow;
	bool input;
	bool output;

	struct {
		uint64_t checked;
		uint iid;
		bool requesting;
		bool accepting;
		bool providing;
		bool activeproviding;
	} hint;

	void destroy();
	void update();
	StoreSettings* settings();
	void setup(StoreSettings*);
	void ghostInit(uint id, uint sid);
	void ghostDestroy();
	void burnerInit(uint id, uint sid, Mass cap, std::string category);
	void burnerDestroy();
	Stack insert(Stack stack);
	Stack remove(Stack stack);
	Stack removeAny(uint size);
	uint wouldRemoveAny();
	uint wouldRemoveAny(miniset<uint>& filter);
	Stack removeFuel(std::string chemical, uint size);
	Stack overflowAny(uint size);
	void promise(Stack stack);
	void reserve(Stack stack);
	void levelSet(uint iid, uint lower, uint upper);
	void levelClear(uint iid);
	Level* level(uint iid);
	Delivery* delivery(uint iid);
	void sortAlpha();
	bool isEmpty();
	bool isFull();
	Mass limit();
	Mass usage();
	Mass usagePredict();
	void calcUsage();
	uint count(uint iid);
	uint countNet(uint iid);
	uint countSpace(uint iid);
	uint countAvailable(uint iid);
	uint countExpected(uint iid);
	uint countProvidable(uint iid, Level* lvl = nullptr);
	uint countActiveProvidable(uint iid, Level* lvl = nullptr);
	uint countAcceptable(uint iid, Level* lvl = nullptr);
	uint countRequired(uint iid, Level* lvl = nullptr);
	bool isRequesterSatisfied();
	bool isRequesting(uint iid, Level* lvl = nullptr);
	bool isProviding(uint iid, Level* lvl = nullptr);
	bool isActiveProviding(uint iid, Level* lvl = nullptr);
	bool isAccepting(uint iid, Level* lvl = nullptr);
	bool isOverflowDefault(uint iid, Level* lvl = nullptr);
	bool relevant(uint iid);
	bool intersection(Store& other);
	Stack forceSupplyFrom(Store& src, uint size = 1);
	Stack forceSupplyFrom(Store& src, miniset<uint>& filter, uint size = 1);
	Stack supplyFrom(Store& src, uint size = 1);
	Stack supplyFrom(Store& src, miniset<uint>& filter, uint size = 1);
	Stack forceOverflowTo(Store& dst, uint size = 1);
	Stack overflowTo(Store& dst, uint size = 1);
	Stack overflowTo(Store& dst, miniset<uint>& filter, uint size = 1);
	Stack overflowDefaultTo(Store& dst, uint size = 1);
	minimap<Signal,&Signal::key> signals();
};

struct StoreSettings {
	minivec<Store::Level> levels;
	minivec<Stack> stacks;
	bool transmit;
	bool anything;
	StoreSettings(Store& store);
};
