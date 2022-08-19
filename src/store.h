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
	bool overflow;
	bool purge;

	struct {
		uint64_t checked;
		uint iid;
		bool requesting;
		bool accepting;
		bool providing;
		bool activeproviding;
		bool repairing;
	} hint;

	void destroy();
	void update();
	StoreSettings* settings();
	void setup(StoreSettings*);
	void ghostInit(uint id, uint sid);
	void ghostDestroy();
	void burnerInit(uint id, uint sid, Mass cap, std::string category);
	void burnerDestroy();
	bool strict();
	Stack insert(Stack stack);
	Stack remove(Stack stack);
	Stack removeFuel(std::string chemical, uint size);
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
	uint countLessReserved(uint iid);
	uint countPlusPromised(uint iid);
	bool isRequesterSatisfied();
	uint countRequesting(uint iid);
	bool isRequesting(uint iid);
	bool isProviding(uint iid);
	uint countProviding(uint iid);
	bool isActiveProviding(uint iid);
	uint countActiveProviding(uint iid);
	bool isAccepting(uint iid);
	uint countAccepting(uint iid);
	minimap<Signal,&Signal::key> signals();
};

struct StoreSettings {
	minivec<Store::Level> levels;
	minivec<Stack> stacks;
	bool transmit;
	StoreSettings(Store& store);
};
