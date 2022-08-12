#include "common.h"
#include "sim.h"
#include "loader.h"

// Loader components link up with Conveyors to load/unload Stores. They
// are essentially a subset of Arm functionality and so correspondingly
// less flexible but also faster and more efficient.

std::size_t Loader::memory() {
	std::size_t size = all.memory();
	for (auto& loader: all) size += loader.filter.memory();
	return size;
}

void Loader::reset() {
	all.clear();
}

void Loader::tick() {
	for (auto& loader: all) loader.update();
}

Loader& Loader::create(uint id) {
	ensure(!all.has(id));
	Entity& en = Entity::get(id);
	Loader& loader = all[id];
	loader.id = id;
	loader.en = &en;
	loader.loading = !en.spec->loaderUnload;
	loader.monitor = Monitor::Store;
	loader.pause = 0;
	loader.cache.point = Point::Zero;
	loader.cache.refresh = 0;
	return loader;
}

Loader& Loader::get(uint id) {
	return all.refer(id);
}

void Loader::destroy() {
	all.erase(id);
}

LoaderSettings* Loader::settings() {
	return new LoaderSettings(*this);
}

LoaderSettings::LoaderSettings(Loader& loader) {
	filter = loader.filter;
	loading = loader.loading;
	monitor = loader.monitor;
	condition = loader.condition;
}

void Loader::setup(LoaderSettings* settings) {
	filter = settings->filter;
	loading = settings->loading;
	monitor = settings->monitor;
	condition = settings->condition;
}

Point Loader::point() {
	return en->spec->loaderPoint.transform(en->dir().rotation()) + en->pos();
}

Stack Loader::transferBeltToStore(Store& dst, Stack stack) {
	Entity& de = *dst.en; // Entity::get(dst.id);

	if (de.isGhost()) {
		return {0,0};
	}

	if (dst.strict()) {
		if (filter.size()) {
			if (filter.count(stack.iid) && dst.isAccepting(stack.iid)) {
				return {stack.iid, std::min(stack.size, dst.countAcceptable(stack.iid))};
			}
			return {0,0};
		}

		if (dst.isAccepting(stack.iid)) {
			return {stack.iid, std::min(stack.size, dst.countAcceptable(stack.iid))};
		}
		return {0,0};
	}

	if (dst.countSpace(stack.iid) && (!filter.size() || filter.count(stack.iid))) {
		return {stack.iid, std::min(stack.size, dst.countSpace(stack.iid))};
	}

	return {0,0};
}

Stack Loader::transferStoreToBelt(Store& src) {
	Entity& se = *src.en; //Entity::get(src.id);

	if (se.isGhost()) {
		return {0,0};
	}

	if (src.strict()) {
		if (filter.size()) {
			Stack stack = {src.wouldRemoveAny(filter),1};
			if (!stack.iid) {
				for (Stack& ss: src.stacks) {
					if (filter.count(ss.iid) && src.isActiveProviding(ss.iid)) return {ss.iid, 1};
				}
				for (Stack& ss: src.stacks) {
					if (filter.count(ss.iid) && src.isProviding(ss.iid)) return {ss.iid, 1};
				}
			}
			return stack;
		}

		Stack stack = {src.wouldRemoveAny(),1};
		if (!stack.iid) {
			for (Stack& ss: src.stacks) {
				if (src.isActiveProviding(ss.iid)) return {ss.iid, 1};
			}
			for (Stack& ss: src.stacks) {
				if (src.isProviding(ss.iid)) return {ss.iid, 1};
			}
		}
		return stack;
	}

	for (auto& ss: src.stacks) {
		if (src.countNet(ss.iid) && (!filter.size() || filter.count(ss.iid))) return {ss.iid, 1};
	}

	return {0,0};
}

// check network
bool Loader::checkCondition() {
	bool rule = condition.valid();
	en->setRuled(rule);

	bool permit = true;

	if (rule && monitor == Monitor::Store && storeId) {
		auto& store = Store::get(storeId);
		permit = condition.evaluate(store.signals());
	}

	if (rule && en->spec->networker && monitor == Monitor::Network) {
		auto network = en->networker().input().network;
		permit = condition.evaluate(network ? network->signals: Signal::NoSignals);
	}

	en->setBlocked(!permit);
	return permit;
}

void Loader::update() {
	if (pause > Sim::tick) return;

	// half the speed of the fastest belt
	auto delay = [&]() { pause = Sim::tick+10; };

	if (en->isGhost()) { delay(); return; }
	if (!en->isEnabled()) { delay(); return; }

	auto& conveyor = en->conveyor();

	if (!conveyor.empty()) {
		if (en->consume(en->spec->energyConsume) == Energy(0)) return;
	}

	if (loading && !conveyor.left.offloading() && !conveyor.right.offloading()) {
		return;
	}

	if (!loading && !conveyor.left.deliverable() && !conveyor.right.deliverable()) {
		return;
	}

	if (cache.refresh < Sim::tick) {
		cache.point = point();
		cache.refresh = Sim::tick+60;
	}

	Box targetArea = cache.point.box().grow(0.1f);
	Entity* es = storeId ? Entity::find(storeId): nullptr;

	if (!es || !es->box().intersects(targetArea)) {
		storeId = 0;
		for (auto es: Entity::intersecting(targetArea, Entity::gridStores)) {
			storeId = es->id;
			break;
		}
	}

	if (!checkCondition()) {
		return;
	}

	if (!storeId) {
		delay();
		en->setBlocked(true);
		return;
	}

	auto& store = Store::get(storeId);

	if (loading) {
		auto loadLeft = [&]() {
			uint iid = conveyor.left.offloading();
			if (iid) {
				Stack stack = transferBeltToStore(store, {iid,1});
				if (stack.iid == iid && stack.size && store.insert({stack.iid,1}).size == 0) {
					conveyor.offloadLeft(iid);
				}
			}
		};

		auto loadRight = [&]() {
			uint iid = conveyor.right.offloading();
			if (iid) {
				Stack stack = transferBeltToStore(store, {iid,1});
				if (stack.iid == iid && stack.size && store.insert({stack.iid,1}).size == 0) {
					conveyor.offloadRight(iid);
				}
			}
		};

		if (Sim::tick%2) {
			loadLeft();
			loadRight();
		} else {
			loadRight();
			loadLeft();
		}
	}

	if (!loading) {
		auto unloadLeft = [&]() {
			Stack stack = transferStoreToBelt(store);
			if (stack.iid && stack.size && conveyor.deliverLeft(stack.iid)) {
				store.remove(stack);
			}
		};

		auto unloadRight = [&]() {
			Stack stack = transferStoreToBelt(store);
			if (stack.iid && stack.size && conveyor.deliverRight(stack.iid)) {
				store.remove(stack);
			}
		};

		if (Sim::tick%2) {
			unloadLeft();
			unloadRight();
		} else {
			unloadRight();
			unloadLeft();
		}
	}
}
