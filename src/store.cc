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

	std::vector<uint> drop;

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
	hint.providing = false;
	hint.activeproviding = false;

	for (Level& lvl: levels) {
		uint n = count(lvl.iid);
		hint.requesting = hint.requesting || (lvl.lower > 0 && n < lvl.lower);
		hint.accepting = hint.accepting || (n < lvl.upper);
		hint.providing = hint.providing || (n > lvl.lower);
		hint.activeproviding = hint.activeproviding || (n > lvl.upper);
	}

	if (!overflow) for (Stack& stk: stacks) {
		if (!level(stk.iid)) {
			hint.providing = true;
			hint.activeproviding = true;
		}
	}

	if (overflow) for (Stack& stk: stacks) {
		if (!level(stk.iid)) hint.providing = true;
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
	store.anything = spec->storeAnything ? spec->storeAnythingDefault: false;
	store.magic = spec->storeMagic;
	store.overflow = spec->overflow;
	store.transmit = false;
	store.input = true;
	store.output = true;
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
	anything = false;
	magic = false;
	overflow = false;
	transmit = false;
	input = true;
	output = true;
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
	anything = false;
	magic = false;
	overflow = false;
	transmit = false;
	input = true;
	output = false;
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
	anything = store.anything;
}

void Store::setup(StoreSettings* settings) {
	levels.clear();
	for (auto level: settings->levels) {
		levelSet(level.iid, level.lower, level.upper);
	}
	transmit = settings->transmit;
	anything = Entity::get(id).spec->storeAnything ? settings->anything: false;
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

uint Store::wouldRemoveAny() {
	for (auto& stack: stacks) {
		if (isActiveProviding(stack.iid)) {
			return stack.iid;
		}
	}
	for (auto& stack: stacks) {
		if (isProviding(stack.iid)) {
			return stack.iid;
		}
	}

	if (!output) return 0;

	for (auto& stack: stacks) {
		if (fuel && Item::get(stack.iid)->fuel.category == fuelCategory) {
			continue;
		}
		Level *lvl = level(stack.iid);
		if (!lvl) {
			return stack.iid;
		}
	}
	return 0;
}

uint Store::wouldRemoveAny(miniset<uint>& filter) {
	for (auto& stack: stacks) {
		if (filter.count(stack.iid) && isActiveProviding(stack.iid)) {
			return stack.iid;
		}
	}
	for (auto& stack: stacks) {
		if (filter.count(stack.iid) && isProviding(stack.iid)) {
			return stack.iid;
		}
	}

	if (!output) return 0;

	for (auto& stack: stacks) {
		if (!filter.count(stack.iid)) {
			continue;
		}
		if (fuel && Item::get(stack.iid)->fuel.category == fuelCategory) {
			continue;
		}
		Level *lvl = level(stack.iid);
		if (!lvl) {
			return stack.iid;
		}
	}
	return 0;
}

Stack Store::removeAny(uint size) {
	uint iid = wouldRemoveAny();
	if (iid) {
		return remove({iid,size});
	}
	return {0,0};
}

Stack Store::removeFuel(std::string category, uint size) {
//	if (!output) return {0,0};

	for (auto& stack: stacks) {
		if (Item::get(stack.iid)->fuel.category == category) {
			return remove({stack.iid, std::min(size, stack.size)});
		}
	}
	return {0,0};
}

Stack Store::overflowAny(uint size) {
	for (auto& stack: stacks) {
		if (isActiveProviding(stack.iid)) {
			return remove({stack.iid, size});
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
uint Store::countAvailable(uint iid) {
	uint n = count(iid);
	Delivery* del = delivery(iid);
	return del ? n - std::min(n, del->reserved): n;
}

// number we can fit in right now
uint Store::countSpace(uint iid) {
	return (limit() - usagePredict()).items(iid);
}

// number we have right now over lower limit that have not been reserved to export
uint Store::countProvidable(uint iid, Level* lvl) {
	uint n = count(iid);
	if (!lvl) lvl = level(iid);
	Delivery* del = delivery(iid);
	if (del) n -= std::min(n, del->reserved);
	if (!lvl) return n;
	if (lvl->lower > n) return 0;
	return n - lvl->lower;
}

// number we have right now over upper limit that have not been reserved to export
uint Store::countActiveProvidable(uint iid, Level* lvl) {
	uint n = count(iid);
	if (!lvl) lvl = level(iid);
	Delivery* del = delivery(iid);
	if (del) n -= std::min(n, del->reserved);
	if (!lvl) return n;
	if (lvl->upper > n) return 0;
	return n - lvl->upper;
}

// number we will to have after current imports are complete.
// does not account for interrim exports
uint Store::countExpected(uint iid) {
	uint n = count(iid);
	Delivery* del = delivery(iid);
	return del ? n + del->promised: n;
}

// number we can fit in under an upper limit
uint Store::countAcceptable(uint iid, Level* lvl) {
	if (!input) return 0;
	if (!lvl) lvl = level(iid);
	uint net = countNet(iid);
	if (lvl && net >= lvl->upper) return 0;
	uint max = (limit()-usagePredict()).items(iid);
	return std::min(max, lvl ? lvl->upper - net: max);
}

// number we need to meet a lower limit
uint Store::countRequired(uint iid, Level* lvl) {
	if (!input) return 0;
	if (!lvl) lvl = level(iid);
	uint net = countNet(iid);
	if (lvl && net >= lvl->lower) return 0;
	uint max = (limit()-usage()).items(iid);
	return std::min(max, lvl ? lvl->lower - net: 0);
}

// store has all lower limits met (ghost construction)
bool Store::isRequesterSatisfied() {
	if (!input) return true;
	for (Level& lvl: levels) {
		if (lvl.lower > 0 && count(lvl.iid) < lvl.lower) {
			return false;
		}
	}
	return true;
}

// store has a lower limit for item not met, and has space
bool Store::isRequesting(uint iid, Level* lvl) {
	if (!input) return false;
	if (!lvl) lvl = level(iid);
	if (!countAcceptable(iid, lvl)) return false;
	return lvl != NULL && lvl->lower > 0 && countExpected(iid) < lvl->lower;
}

// store has a lower limit for item exceeded
bool Store::isProviding(uint iid, Level* lvl) {
	if (!output) return false;
	if (!lvl) lvl = level(iid);
	uint available = countAvailable(iid);
	if (!lvl && !fuel && !ghost && available) return true;
	return lvl != NULL && lvl->upper >= lvl->lower && available > lvl->lower;
}

// store has an upper limit for item exceeded
bool Store::isActiveProviding(uint iid, Level* lvl) {
	if (!output) return false;
	if (!lvl) lvl = level(iid);
	uint available = countAvailable(iid);
	return lvl != NULL && lvl->upper >= lvl->lower && available > lvl->upper;
}

// store has an upper limit for an item not exceeded, and has space
bool Store::isAccepting(uint iid, Level* lvl) {
	if (!input) return false;
	if (!lvl) lvl = level(iid);

	if (isFull() || !countAcceptable(iid, lvl)) {
		return false;
	}
	if (isRequesting(iid, lvl)) {
		return true;
	}
	if (!lvl && anything) {
		return true;
	}
	if (lvl != NULL && countExpected(iid) < lvl->upper) {
		return true;
	}
	if (fuel && Item::get(iid)->fuel.category == fuelCategory) {
		return true;
	}
	return false;
}

bool Store::relevant(uint iid) {
	for (auto& lvl: levels) if (lvl.iid == iid) return true;
	for (auto& stk: stacks) if (stk.iid == iid) return true;
	return false;
}

bool Store::intersection(Store& other) {
	for (auto& lvl: levels) if (other.relevant(lvl.iid)) return true;
	for (auto& stk: stacks) if (other.relevant(stk.iid)) return true;
	return false;
}

// requester to another requester
Stack Store::forceSupplyFrom(Store& src, uint size) {
	if (!isFull() && !src.isEmpty()) {
		for (Level& dl: levels) {
			if (!src.relevant(dl.iid)) continue;
			uint have = src.countAvailable(dl.iid);
			uint need = countRequired(dl.iid, &dl);
			if (need && have && isAccepting(dl.iid, &dl) && isRequesting(dl.iid, &dl)) {
				return {dl.iid, std::min(size, std::min(need, have))};
			}
		}
		if (fuel) {
			for (Stack& ss: src.stacks) {
				if (Item::get(ss.iid)->fuel.category == fuelCategory) {
					return {ss.iid, 1};
				}
			}
		}
	}
	return {0,0};
}

Stack Store::forceSupplyFrom(Store& src, miniset<uint>& filter, uint size) {
	if (!isFull() && !src.isEmpty()) {
		for (Level& dl: levels) {
			if (!filter.count(dl.iid)) continue;
			if (!src.relevant(dl.iid)) continue;
			uint have = src.countAvailable(dl.iid);
			uint need = countRequired(dl.iid, &dl);
			if (need && have && isAccepting(dl.iid, &dl) && isRequesting(dl.iid, &dl)) {
				return {dl.iid, std::min(size, std::min(need, have))};
			}
		}
		if (fuel) {
			for (Stack& ss: src.stacks) {
				if (filter.count(ss.iid) && Item::get(ss.iid)->fuel.category == fuelCategory) {
					return {ss.iid, 1};
				}
			}
		}
	}
	return {0,0};
}

// any provider or overflow to requester
Stack Store::supplyFrom(Store& src, uint size) {
	if (!src.output || !input) return {0,0};

	if (!isFull() && !src.isEmpty()) {
		for (Level& dl: levels) {
			if (!src.relevant(dl.iid)) continue;
			uint have = src.countProvidable(dl.iid);
			uint need = countRequired(dl.iid, &dl);
			if (need && have && isAccepting(dl.iid, &dl) && isRequesting(dl.iid, &dl) && src.isProviding(dl.iid)) {
				return {dl.iid, std::min(size, std::min(have, need))};
			}
		}
		if (fuel) {
			for (Stack& ss: src.stacks) {
				if (Item::get(ss.iid)->fuel.category == fuelCategory && isAccepting(ss.iid) && src.isProviding(ss.iid)) {
					return {ss.iid, 1};
				}
			}
		}
	}
	return {0,0};
}

Stack Store::supplyFrom(Store& src, miniset<uint>& filter, uint size) {
	if (!src.output || !input) return {0,0};

	if (!isFull() && !src.isEmpty()) {
		for (Level& dl: levels) {
			if (!filter.count(dl.iid)) continue;
			if (!src.relevant(dl.iid)) continue;
			uint have = src.countProvidable(dl.iid);
			uint need = countRequired(dl.iid, &dl);
			if (need && have && isAccepting(dl.iid, &dl) && isRequesting(dl.iid, &dl) && src.isProviding(dl.iid)) {
				return {dl.iid, std::min(size, std::min(have, need))};
			}
		}
		if (fuel) {
			for (Stack& ss: src.stacks) {
				if (filter.count(ss.iid) && Item::get(ss.iid)->fuel.category == fuelCategory && isAccepting(ss.iid) && src.isProviding(ss.iid)) {
					return {ss.iid, 1};
				}
			}
		}
	}
	return {0,0};
}

// active provider to anything
Stack Store::forceOverflowTo(Store& dst, uint size) {
	if (!dst.isFull()) {
		for (Level& sl: levels) {
//			if (!dst.relevant(sl.iid)) continue;
			uint excess = countActiveProvidable(sl.iid, &sl);
			uint accept = dst.countSpace(sl.iid);
			if (excess && accept && isActiveProviding(sl.iid, &sl)) {
				return {sl.iid, std::min(size, std::min(accept, excess))};
			}
		}
	}
	return {0,0};
}

// active provider to overflow with space
Stack Store::overflowTo(Store& dst, uint size) {
	if (!dst.input || !output) return {0,0};

	for (Level& sl: levels) {
		if (!dst.relevant(sl.iid)) continue;
		uint excess = countActiveProvidable(sl.iid, &sl);
		uint accept = dst.countAcceptable(sl.iid);
		if (excess && accept && dst.isAccepting(sl.iid) && isActiveProviding(sl.iid, &sl)) {
			return {sl.iid, std::min(size, std::min(excess, accept))};
		}
	}

	if (!anything) {
		// stacks without levels are implicitly active for overlow
		for (Stack& stack: stacks) {
			if (!dst.relevant(stack.iid)) continue;
			if (level(stack.iid)) continue;
			uint excess = stack.size;
			uint accept = dst.countAcceptable(stack.iid);
			if (excess && accept && dst.isAccepting(stack.iid)) {
				return {stack.iid, std::min(size, std::min(excess, accept))};
			}
		}
	}
	return {0,0};
}

Stack Store::overflowTo(Store& dst, miniset<uint>& filter, uint size) {
	if (!dst.input || !output) return {0,0};

	for (Level& sl: levels) {
		if (!filter.count(sl.iid)) continue;
		if (!dst.relevant(sl.iid)) continue;
		uint excess = countActiveProvidable(sl.iid, &sl);
		uint accept = dst.countAcceptable(sl.iid);
		if (excess && accept && dst.isAccepting(sl.iid) && isActiveProviding(sl.iid, &sl)) {
			return {sl.iid, std::min(size, std::min(excess, accept))};
		}
	}

	if (!anything) {
		// stacks without levels are implicitly active for overlow
		for (Stack& stack: stacks) {
			if (!dst.relevant(stack.iid)) continue;
			if (level(stack.iid)) continue;
			if (!filter.count(stack.iid)) continue;
			uint excess = stack.size;
			uint accept = dst.countAcceptable(stack.iid);
			if (excess && accept && dst.isAccepting(stack.iid)) {
				return {stack.iid, std::min(size, std::min(excess, accept))};
			}
		}
	}
	return {0,0};
}

// store is a logistic overflow accepting an item
bool Store::isOverflowDefault(uint iid, Level* lvl) {
	if (ghost || fuel || !overflow || isFull()) return false;
	if (!lvl) lvl = level(iid);
	return lvl ? isAccepting(iid, lvl): true;
}

// active provider to default overflow
Stack Store::overflowDefaultTo(Store& dst, uint size) {
	if (!dst.input || !output) return {0,0};

	if (overflow) {
		for (Stack& stack: stacks) {
			uint excess = countActiveProvidable(stack.iid);
			uint accept = dst.countAcceptable(stack.iid);
			if (excess && accept && dst.isAccepting(stack.iid) && !dst.isOverflowDefault(stack.iid) && isActiveProviding(stack.iid)) {
				return {stack.iid, std::min(size, std::min(excess, accept))};
			}
		}
		return {0,0};
	}

	for (Level& sl: levels) {
		uint excess = countActiveProvidable(sl.iid);
		uint accept = dst.countSpace(sl.iid);
		if (excess && accept && dst.isOverflowDefault(sl.iid) && isActiveProviding(sl.iid, &sl)) {
			return {sl.iid, std::min(size, std::min(excess, accept))};
		}
	}

	if (!anything) {
		// stacks without levels are implicitly active for overlow
		for (Stack& stack: stacks) {
			if (level(stack.iid)) continue;
			uint excess = stack.size;
			uint accept = dst.countSpace(stack.iid);
			if (excess && accept && dst.isOverflowDefault(stack.iid)) {
				return {stack.iid, std::min(size, std::min(excess, accept))};
			}
		}
	}
	return {0,0};
}

// special case: rebalance overflow back to provider with space
Stack Store::overflowBalanceTo(Store& dst, uint size) {
	if (!dst.input || !output || !overflow || dst.overflow) return {0,0};

	for (Level& sl: levels) {
		if (!dst.relevant(sl.iid)) continue;
		uint excess = countProvidable(sl.iid, &sl);
		uint accept = dst.countAcceptable(sl.iid);
		if (excess && accept && dst.isAccepting(sl.iid) && isProviding(sl.iid, &sl)) {
			return {sl.iid, std::min(size, std::min(excess, accept))};
		}
	}

	if (!anything) {
		// stacks without levels are implicitly active for overlow
		for (auto& stack: stacks) {
			if (level(stack.iid)) continue;
			if (!dst.relevant(stack.iid)) continue;
			uint excess = stack.size;
			uint accept = dst.countAcceptable(stack.iid);
			if (excess && accept && dst.isAccepting(stack.iid) && isProviding(stack.iid)) {
				return {stack.iid, std::min(size, std::min(excess, accept))};
			}
		}
	}

	return {0,0};
}

minimap<Signal,&Signal::key> Store::signals() {
	minimap<Signal,&Signal::key> sigs;
	for (auto& stack: stacks) {
		sigs.insert(Signal(stack));
	}
	return sigs;
}
