#include "common.h"
#include "store.h"
#include "entity.h"
#include "sim.h"

// Store components are general-purpose configurable item inventories. They
// allow setting lower and upper limits per item and can function as dumb
// containers, logistic requester/suppliers, burner fuel tanks, crafter
// input/output buffers etc. Arms and Drones work together with Stores to
// move items around co-operatively.

//                lower                upper                    capacity
// |----------------|--------------------|-------------------------|
//     requester           provider            active provider
//     overflow            overflow              no overflow

std::size_t Store::memory() {
	std::size_t size = all.memory();
	for (auto& store: all) {
		size += store.stacks.memory();
		size += store.levels.memory();
		size += store.drones.memory();
		size += store.arms.memory();
	}
	return size;
}

void Store::reset() {
	all.clear();
}

void Store::tick() {
	for (Store& store: all) {
		store.update();
	}
	for (auto& burner: Burner::all) {
		burner.store.update();
	}
	for (auto& ghost: Ghost::all) {
		ghost.store.update();
	}
}

void Store::update() {
	for (Level& lvl: levels) {
		lvl.upper = std::max(lvl.lower, lvl.upper);
	}

	deliveries.clear();

	minivec<uint> drop;

	for (uint did: drones) {
		if (!Entity::exists(did)) {
			drop.push_back(did);
			continue;
		}

		Drone& drone = Drone::get(did);

		if (drone.src == id) {
			reserve(drone.stack);
		}

		if (drone.dst == id) {
			promise(drone.stack);
		}
	}

	for (uint did: drop) {
		drones.erase(did);
	}

	drop.clear();

	for (uint aid: arms) {
		if (!Entity::exists(aid)) {
			drop.push_back(aid);
			continue;
		}

		Arm& arm = Arm::get(aid);

		if (arm.inputId != id && arm.outputId != id) {
			drop.push_back(aid);
			continue;
		}

		if (arm.iid && arm.inputId == id) {
			reserve({arm.iid,1});
		}

		if (arm.iid && arm.outputId == id) {
			promise({arm.iid,1});
		}
	}

	for (uint aid: drop) {
		arms.erase(aid);
	}

	calcUsage();

	if (magic) {
		for (auto& level: levels) {
			uint net = countNet(level.iid);
			uint mid = level.lower + (level.upper - level.lower)/2;
			if (net < mid) insert({level.iid, mid - net});
			if (net > mid) remove({level.iid, net - mid});
		}
		calcUsage();
	}

	if (transmit) {
		if (!en->isGhost() && en->spec->networker) {
			auto& networker = en->networker();
			for (auto& stack: stacks) {
				networker.output().write(stack);
			}
		}
	}

	hint.requesting = false;
	hint.accepting = overflow;
	hint.providing = overflow;
	hint.activeproviding = false;
	hint.repairing = false;

	for (Level& lvl: levels) {
		uint n = count(lvl.iid);
		hint.requesting = hint.requesting || (lvl.lower > 0 && n < lvl.lower);
		hint.accepting = hint.accepting || (n < lvl.upper);
		hint.providing = hint.providing || (n > lvl.lower);
		hint.activeproviding = hint.activeproviding || (n > lvl.upper);
	}

	for (Stack& stk: stacks) {
		hint.repairing = hint.repairing || Item::get(stk.iid)->repair;
		if (level(stk.iid)) continue;
		hint.providing = true;
		hint.activeproviding = true;
	}

	if (Sim::tick > 60 && hint.checked < Sim::tick-60) {
		hint.iid = 0;
		hint.checked = Sim::tick;
		if (!hint.iid) for (auto& lvl: levels) {
			hint.iid = lvl.iid;
			break;
		}
		if (!hint.iid) for (auto& stk: stacks) {
			hint.iid = stk.iid;
			break;
		}
	}
}

Store& Store::create(uint id, uint sid, Mass cap) {
	ensure(!all.has(id));
	Store& store = all[id];
	store.id = id;
	store.en = &Entity::get(id);
	store.sid = sid;
	store.activity = 0;
	store.contents = 0;
	store.contentsPredict = 0;
	store.capacity = cap;
	store.fuel = false;
	store.ghost = false;
	auto spec = store.en->spec;
	store.magic = spec->storeMagic;
	store.overflow = spec->overflow;
	store.transmit = false;
	store.purge = spec->zeppelin;
	store.hint.checked = 0;
	store.hint.iid = 0;
	store.hint.requesting = false;
	store.hint.accepting = false;
	store.hint.providing = false;
	store.hint.activeproviding = false;
	return store;
}

void Store::ghostInit(uint gid, uint gsid) {
	id = gid;
	en = &Entity::get(id);
	sid = gsid;
	activity = 0;
	contents = 0;
	contentsPredict = 0;
	capacity = Mass::Inf;
	fuel = false;
	ghost = true;
	magic = false;
	overflow = false;
	transmit = false;
	purge = false;
	hint.checked = 0;
	hint.iid = 0;
	hint.requesting = false;
	hint.accepting = false;
	hint.providing = false;
	hint.activeproviding = false;
}

void Store::burnerInit(uint bid, uint bsid, Mass cap, std::string category) {
	id = bid;
	en = &Entity::get(id);
	sid = bsid;
	activity = 0;
	contents = 0;
	contentsPredict = 0;
	capacity = cap;
	fuel = true;
	fuelCategory = category;
	ghost = false;
	magic = false;
	overflow = false;
	transmit = false;
	purge = false;
	hint.checked = 0;
	hint.iid = 0;
	hint.requesting = false;
	hint.accepting = false;
	hint.providing = false;
	hint.activeproviding = false;
}

Store& Store::get(uint id) {
	return all.refer(id);
}

void Store::destroy() {
	stacks.clear();
	levels.clear();
	all.erase(id);
}

void Store::ghostDestroy() {
	stacks.clear();
	levels.clear();
}

void Store::burnerDestroy() {
	stacks.clear();
	levels.clear();
}

StoreSettings* Store::settings() {
	return new StoreSettings(*this);
}

StoreSettings::StoreSettings(Store& store) {
	for (auto level: store.levels) {
		levels.push_back(level);
	}
	transmit = store.transmit;
}

void Store::setup(StoreSettings* settings) {
	levels.clear();
	for (auto level: settings->levels) {
		levelSet(level.iid, level.lower, level.upper);
	}
	transmit = settings->transmit;
}

bool Store::strict() {
	return ghost || fuel || en->spec->crafter || en->spec->turret || en->spec->launcher;
}

Stack Store::insert(Stack istack) {
	Mass space = limit() - usage();
	uint count = std::min(istack.size, space.items(istack.iid));

	if (count > 0) {
		activity = Sim::tick;
	}

	for (auto& stack: stacks) {
		if (stack.iid == istack.iid) {
			stack.size += count;
			istack.size -= count;
			count = 0;
			break;
		}
	}

	if (count > 0) {
		stacks.push_back({istack.iid, count});
		istack.size -= count;
	}
	calcUsage();
	return istack;
}

Stack Store::remove(Stack rstack) {
	for (auto it = stacks.begin(); it != stacks.end(); it++) {
		if (it->iid == rstack.iid) {
			if (it->size <= rstack.size) {
				rstack.size = it->size;
				stacks.erase(it);
			} else {
				it->size -= rstack.size;
			}
			activity = Sim::tick;
			calcUsage();
			return rstack;
		}
	}
	rstack.size = 0;
	return rstack;
}

Stack Store::removeFuel(std::string category, uint size) {
	for (auto& stack: stacks) {
		if (Item::get(stack.iid)->fuel.category == category) {
			return remove({stack.iid, std::min(size, stack.size)});
		}
	}
	return {0,0};
}

void Store::promise(Stack stack) {
	Delivery *del = delivery(stack.iid);
	if (!del) {
		deliveries.push_back({stack.iid,0,0});
		del = &deliveries.back();
	}
	del->promised += stack.size;
}

void Store::reserve(Stack stack) {
	Delivery *del = delivery(stack.iid);
	if (!del) {
		deliveries.push_back({stack.iid,0,0});
		del = &deliveries.back();
	}
	del->reserved += stack.size;
}

void Store::levelSet(uint iid, uint lower, uint upper) {
	Level *lvl = level(iid);
	if (lvl) {
		lvl->lower = lower;
		lvl->upper = upper;
		return;
	}
	levels.push_back({
		.iid = iid,
		.lower = lower,
		.upper = upper,
	});
}

void Store::levelClear(uint iid) {
	for (auto it = levels.begin(); it != levels.end(); it++) {
		if (it->iid == iid) {
			levels.erase(it);
			break;
		}
	}
}

Store::Level* Store::level(uint iid) {
	for (auto& lvl: levels) {
		if (lvl.iid == iid) return &lvl;
	}
	return nullptr;
}

Store::Delivery* Store::delivery(uint iid) {
	for (auto& del: deliveries) {
		if (del.iid == iid) return &del;
	}
	return nullptr;
}

void Store::sortAlpha() {
	std::map<std::string,Level> currentLevels;
	std::map<std::string,Stack> currentStacks;
	for (auto& level: levels) {
		currentLevels[Item::get(level.iid)->name] = level;
	}
	for (auto& stack: stacks) {
		currentStacks[Item::get(stack.iid)->name] = stack;
	}
	levels.clear();
	stacks.clear();
	for (auto& [_,level]: currentLevels) {
		levels.push_back(level);
	}
	for (auto& [_,stack]: currentStacks) {
		stacks.push_back(stack);
	}
}

bool Store::isEmpty() {
	return stacks.size() == 0;
}

bool Store::isFull() {
	return !ghost && usagePredict() >= limit();
}

Mass Store::limit() {
	return capacity;
}

Mass Store::usage() {
	if (!contents) calcUsage();
	return contents;
}

Mass Store::usagePredict() {
	if (!contentsPredict) calcUsage();
	return contents;
}

void Store::calcUsage() {
	contents = 0;
	for (auto& stack: stacks) {
		contents += Item::get(stack.iid)->mass * stack.size;
	}
	contentsPredict = contents;
	for (auto& del: deliveries) {
		if (del.promised) contentsPredict += Item::get(del.iid)->mass * del.promised;
	}
}

// number we have right now
uint Store::count(uint iid) {
	for (auto& stack: stacks) {
		if (stack.iid == iid) return stack.size;
	}
	return 0;
}

// number we will have, after current imports and exports are complete
uint Store::countNet(uint iid) {
	uint n = count(iid);
	Delivery* del = delivery(iid);
	return del ? n - std::min(n, del->reserved) + del->promised: n;
}

// number we have right now that have not been reserved to export
uint Store::countLessReserved(uint iid) {
	uint n = count(iid);
	Delivery* del = delivery(iid);
	return del ? n - std::min(n, del->reserved): n;
}

// number we will to have after current imports are complete
uint Store::countPlusPromised(uint iid) {
	uint n = count(iid);
	Delivery* del = delivery(iid);
	return del ? n + del->promised: n;
}

// number we can fit in after current imports are complete
uint Store::countSpace(uint iid) {
	return (limit() - usagePredict()).items(iid);
}

// store has all lower limits met (ghost construction)
bool Store::isRequesterSatisfied() {
	for (Level& lvl: levels) {
		if (lvl.lower > 0 && count(lvl.iid) < lvl.lower) return false;
	}
	return true;
}

// store has a lower limit for item not met, and has space
bool Store::isRequesting(uint iid) {
	return countRequesting(iid) > 0;
}

// number under a lower limit that have not been promised to import
uint Store::countRequesting(uint iid) {
	auto lvl = level(iid);
	int n = countPlusPromised(iid);
	int spaceA = (limit()-usage()).items(iid);
	int spaceB = (limit()-usagePredict()).items(iid);
	int space = std::max(0, std::min(spaceA, spaceB));
	if (fuel && Item::get(iid)->fuel.category == fuelCategory) return space;
	if (!lvl) return 0;
	return std::max(0, std::min(space, (int)lvl->lower - n));
}

// store has a lower limit for item exceeded
bool Store::isProviding(uint iid) {
	return countProviding(iid) > 0;
}

// number right now over lower limit that have not been reserved to export
uint Store::countProviding(uint iid) {
	if (fuel) return 0;
	uint n = countLessReserved(iid);
	auto lvl = level(iid);
	if (!lvl) return n;
	return (n > lvl->lower) ? lvl->lower-n: 0;
}

// store has an upper limit for item exceeded
bool Store::isActiveProviding(uint iid) {
	return countActiveProviding(iid) > 0;
}

// number right now over upper limit that have not been reserved to export
uint Store::countActiveProviding(uint iid) {
	if (fuel) return 0;
	uint n = countLessReserved(iid);
	auto lvl = level(iid);
	if (!lvl && purge) return n;
	if (!lvl) return 0;
	return (n > lvl->upper) ? lvl->upper-n: 0;
}

// store has an upper limit for an item not exceeded, and has space
bool Store::isAccepting(uint iid) {
	return countAccepting(iid) > 0;
}

// number we can fit in under an upper limit
uint Store::countAccepting(uint iid) {
	auto lvl = level(iid);
	int n = countPlusPromised(iid);
	int spaceA = (limit()-usage()).items(iid);
	int spaceB = (limit()-usagePredict()).items(iid);
	int space = std::max(0, std::min(spaceA, spaceB));
	if (fuel && Item::get(iid)->fuel.category == fuelCategory) return space;
	if (!lvl) return 0;
	return std::max(0, std::min(space, (int)lvl->upper - n));
}

minimap<Signal,&Signal::key> Store::signals() {
	minimap<Signal,&Signal::key> sigs;
	for (auto& stack: stacks) {
		sigs.insert(Signal(stack));
	}
	return sigs;
}
