#pragma once

// Conveyor components move items around, automatically linking together to form belts.

// Conveyors forming a belt are moved from `unmanaged` into vectors for iteration locality.
// A single belt is a single vector, no matter how long. It's practically diffcult to place
// a long enough unbroken belt for this to be a memory allocation problem.
//
// Observation: Factories are rarely perfectly balanced and belts spend a lot of time either
// mostly full (front of belt is backed up) or mostly empty (front of belt is starved).
// Optimizing for those states by tracking the first "active" conveyor on each belt reduces
// load significantly.
//
// Observation: Belts that don't side-load to another can be updated in parallel, so those
// can be handed off to a worker thread pool.

struct Conveyor;
struct ConveyorSlot;
struct ConveyorSegment;
struct ConveyorBelt;

struct ConveyorSlot {
	uint16_t iid = 0;
	uint16_t offset = 0;
};

struct ConveyorSegment {
	ConveyorSlot items[3];
	uint16_t slots = 0;
	uint16_t steps = 0;
	bool update(ConveyorSegment* snext, ConveyorSegment* sprev, ConveyorSegment* sside, uint slot, bool* blocked);
	bool insert(uint slot, uint iid);
	bool deliver(uint iid);
	bool deliverable();
	bool remove(uint iid);
	bool removeBack(uint iid);
	uint removeAny();
	uint removeAnyBack();
	uint count();
	uint countBack();
	uint countFront();
	uint offloading();
	bool offload(uint iid);
	void flush();
};

#include "entity.h"
#include "slabmap.h"
#include "hashset.h"

struct Conveyor {
	Entity* en = nullptr;
	ConveyorBelt* belt = nullptr;
	uint id = 0;
	uint prev = 0;
	uint next = 0;
	uint side = 0;
	ConveyorSegment left;
	ConveyorSegment right;
	bool marked = false;
	bool blockedLeft = false;
	bool blockedRight = false;

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);
	static std::size_t memory();

	struct ConveyorLink {
		uint id = 0;
		uint offset = 0;
		ConveyorBelt* belt = nullptr;
	};

	static inline slabmap<ConveyorLink,&ConveyorLink::id> managed;
	static inline slabmap<Conveyor,&Conveyor::id> unmanaged;

	static Conveyor& create(uint id);
	static Conveyor& get(uint id);

	static std::vector<uint> upgradableGroup(uint cid);

	static inline hashset<uint> leadersStraight;
	static inline minivec<uint> leadersStraightSide;
	static inline minivec<uint> leadersStraightNoSide;
	static inline hashset<uint> leadersCircular;
	static inline hashset<uint> changed;

	static inline std::map<Spec*,int> extant;

	enum {
		SideLoadLeft = 0,
		SideLoadRight,
	};

	Conveyor() = default;
	Conveyor(const Conveyor& other);
	Conveyor& operator=(const Conveyor& other);

	void destroy();
	Conveyor& manage();
	Conveyor& unmanage();

	void updateLeft();
	void updateRight();
	bool insert(uint load, uint iid);
	bool insertLeft(uint slot, uint iid);
	bool insertRight(uint slot, uint iid);
	bool insertFrom(Point pos, uint iid);
	bool insertNear(Point pos, uint iid);
	bool insertFar(Point pos, uint iid);
	bool insertAny(uint iid);
	bool insertAnyFront(uint iid);
	bool insertAnyBack(uint iid);
	bool offloadLeft(uint iid);
	bool offloadRight(uint iid);
	bool deliverLeft(uint iid);
	bool deliverRight(uint iid);
	bool remove(uint iid);
	bool remove(uint load, uint iid);
	bool removeLeft(uint iid);
	bool removeLeftBack(uint iid);
	uint removeLeftAny();
	bool removeRight(uint iid);
	bool removeRightBack(uint iid);
	uint removeRightAny();
	bool removeNear(Point pos, uint iid);
	bool removeFar(Point pos, uint iid);
	uint removeAny();
	uint removeAnyBack();
	uint countFront();
	uint countBack();
	bool blocked();
	bool empty();
	bool full();
	void hold();
	std::vector<uint> items();

	Point input();
	Point output();

	uint offset();
	bool last();
	bool deadEnd();
	void flush();

	void upgrade(uint uid);
};

struct ConveyorBelt {
	static inline hashset<ConveyorBelt*> all;
	static inline hashset<ConveyorBelt*> changed;
	std::vector<Conveyor> conveyors;
	Conveyor* cside = nullptr;
	int sideLoad = Conveyor::SideLoadLeft;
	uint firstActiveLeft = 0;
	uint firstActiveRight = 0;
	Spec* bulkConsumeSpec = nullptr;
	void activeLeft(uint offset);
	void activeRight(uint offset);
	void flush();
};
