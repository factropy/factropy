#include "common.h"
#include "depot.h"
#include "sim.h"

// Depot components deploy Drones to construct and deconstruct Ghosts, and supply
// Stores within their vicinity. Depots don't form networks or send drones over
// long distances, but they do co-operate within shared areas to load-balance
// drone activity.

void Depot::reset() {
	hot.clear();
	cold.clear();
	all.clear();
}

void Depot::tick() {
	networkedStores.clear();

	for (auto network: Networker::networks) {
		auto& stores = networkedStores[network];
		for (auto nid: network->logisticStores) {
			auto& ne = Entity::get(nid);
			ensuref(!ne.isGhost() && ne.spec->store && ne.spec->logistic, "invalid network logistic store");
			stores.push_back({.id = ne.id, .en = &ne, .store = &ne.store(), .pos = ne.pos()});
		}
	}

	hot.tick();
	for (auto depot: hot) depot->update();

	cold.tick();
	for (auto depot: cold) depot->update();
}

Depot& Depot::create(uint id) {
	ensure(!all.has(id));
	Depot& depot = all[id];
	depot.id = id;
	depot.en = &Entity::get(id);
	depot.nextDispatch = 0;
	depot.construction = true;
	depot.deconstruction = true;
	depot.recall = false;
	depot.network = true;
	depot.nextStoreRefresh = 0;
	depot.nextGhostRefresh = 0;
	cold.insert(&depot);
	return depot;
}

Depot& Depot::get(uint id) {
	return all.refer(id);
}

void Depot::destroy() {
	cold.erase(this);
	hot.erase(this);
	all.erase(id);
}

void Depot::update() {
	if (nextDispatch > Sim::tick) {
		hot.pause(this);
		return;
	}

	if (en->isGhost()) {
		cold.pause(this);
		return;
	}

	minivec<uint> drop;
	for (auto did: drones) {
		if (!Entity::exists(did)) drop.push_back(did);
	}
	for (auto did: drop) {
		drones.erase(did);
	}

	if (en->spec->depotFixed) {
		for (auto did: drones)
			Drone::get(did).rangeCheck(en->pos() + en->spec->dronePoint);
	}

	if (drones.size() >= en->spec->depotDrones || recall) {
		cold.pause(this);
		return;
	}

	if (en->spec->consumeCharge && en->spec->depotDispatchEnergy > en->charger().energy) {
		cold.pause(this);
		return;
	}

	auto range = Sphere(en->pos().floor(0.0), en->spec->depotRange);

	if (Sim::tick >= nextGhostRefresh && (construction || deconstruction)) {
		nextGhostRefresh = Sim::tick + (en->spec->zeppelin ? 60: 60*3);

		auto pos = en->pos();
		minivec<Entity*> ghosts;

		for (auto eg: Entity::intersecting(range, Entity::gridGhosts)) {
			ensure(eg->isGhost());
			if (eg->isConstruction() && !eg->spec->licensed) continue;
			ghosts.push(eg);
		}

		std::sort(ghosts.begin(), ghosts.end(), [&](const auto& a, const auto& b) {
			return a->pos().distanceSquared(pos) < b->pos().distanceSquared(pos);
		});

		ghostsCache.clear();
		for (auto eg: ghosts) ghostsCache.push(eg->id);
	}

	minivec<EntityStore> ghosts;

	for (auto gid: ghostsCache) {
		auto eg = Entity::find(gid);
		if (!eg || !eg->isGhost()) continue;
		ghosts.push_back({.id = eg->id, .en = eg, .store = &eg->ghost().store, .pos = eg->pos()});
	}

	auto damaged = Entity::intersecting(range, Entity::gridDamaged);
	discard_if(damaged, [&](Entity* ed) { return ed->spec->junk || Entity::repairActions.count(ed->id); });

	minivec<EntityStore> stores;

	if (nextStoreRefresh < Sim::tick) {
		nextStoreRefresh = Sim::tick+60*3;
		for (auto es: Entity::intersecting(range, Entity::gridStoresLogistic)) {
			if (es->isGhost()) continue;
			if (es->spec->zeppelin && es->zeppelin().moving) continue;
			ensure(es->spec->store && es->spec->logistic);
			stores.push({.id = es->id, .en = es, .store = &es->store(), .pos = es->pos()});
		}
		auto pos = en->pos();
		std::sort(stores.begin(), stores.end(), [&](const auto& a, const auto& b) {
			return a.pos.distanceSquared(pos) < b.pos.distanceSquared(pos);
		});
		storesCache.clear();
		for (auto& item: stores) storesCache.push(item.id);
	}

	if (!stores.size()) for (auto sid: storesCache) {
		auto se = Entity::find(sid);
		if (!se) continue;
		if (se->isGhost()) continue;
		if (se->spec->zeppelin && se->zeppelin().moving) continue;
		stores.push_back({.id = se->id, .en = se, .store = &se->store(), .pos = se->pos()});
	}

	uint cargo = en->spec->depotDroneSpec->droneCargoSize;

	auto supply = [&](auto& src, auto& dst) {
		if (src.en == dst.en) return false;
		if (!src.hint.providing) return false;
		if (!dst.hint.requesting) return false;
		for (auto& stack: src.stacks) {
			uint need = dst.countRequesting(stack.iid);
			if (!need) continue;
			uint have = src.countProviding(stack.iid);
			if (!have) continue;
			uint move = std::min(cargo,std::min(have,need));
			dispatch(id, src.en->id, dst.en->id, {stack.iid,move});
			return true;
		}
		return false;
	};

	auto forceSupply = [&](auto& src, auto& dst) {
		if (src.en == dst.en) return false;
		if (!dst.hint.requesting) return false;
		for (auto& stack: src.stacks) {
			uint need = dst.countRequesting(stack.iid);
			if (!need) continue;
			uint have = src.countLessReserved(stack.iid);
			if (!have) continue;
			uint move = std::min(cargo,std::min(have,need));
			dispatch(id, src.en->id, dst.en->id, {stack.iid,move});
			return true;
		}
		return false;
	};

	auto constructGhostLocal = [&]() {
		for (auto& ghost: ghosts) {
			if (!ghost.en->isConstruction()) continue;
			if (forceSupply(en->store(), *ghost.store)) return true;
		}
		return false;
	};

	auto collect = [&](auto& src, auto& dst) {
		if (src.en == dst.en) return false;
		if (!dst.hint.accepting) return false;
		if (!src.hint.activeproviding) return false;
		for (auto& stack: src.stacks) {
			uint have = src.countActiveProviding(stack.iid);
			if (!have) continue;
			uint space = dst.countAccepting(stack.iid);
			if (!space) continue;
			uint move = std::min(cargo,std::min(space,have));
			dispatch(id, src.en->id, dst.en->id, {stack.iid,move});
			return true;
		}
		return false;
	};

	auto forceCollect = [&](auto& src, auto& dst) {
		if (src.en == dst.en) return false;
		if (!src.hint.activeproviding) return false;
		for (auto& stack: src.stacks) {
			uint have = src.countActiveProviding(stack.iid);
			if (!have) continue;
			uint space = dst.countSpace(stack.iid);
			if (!space) continue;
			uint move = std::min(cargo,std::min(space,have));
			dispatch(id, src.en->id, dst.en->id, {stack.iid,move});
			return true;
		}
		return false;
	};

	auto overflow = [&](auto& src, auto& dst) {
		if (!dst.overflow) return false;
		return forceCollect(src, dst);
	};

	auto deconstructGhostLocal = [&]() {
		for (auto& ghost: ghosts) {
			if (!ghost.en->isDeconstruction()) continue;
			if (forceCollect(*ghost.store, en->store())) return true;
		}
		return false;
	};

	auto repairDamageLocal = [&]() {
		auto& store = en->store();
		for (auto& stack: store.stacks) {
			if (!Item::get(stack.iid)->repair) continue;
			for (auto de: damaged) {
				dispatch(id, id, de->id, {stack.iid,1}, FlightRepair);
				return true;
			}
		}
		return false;
	};

	auto constructGhost = [&](const minivec<EntityStore>& src) {
		if (!src.size()) return false;
		if (!ghosts.size()) return false;
		// first buffers or overflows
		for (auto se: src) {
			if (!se.en->spec->construction && !se.en->spec->overflow) continue;
			for (auto de: ghosts) {
				if (!de.en->isConstruction()) continue;
				if (forceSupply(*se.store, *de.store)) return true;
			}
		}
		// then other providers
		for (auto se: src) {
			if (se.en->spec->construction || se.en->spec->overflow) continue;
			for (auto de: ghosts) {
				if (!de.en->isConstruction()) continue;
				if (supply(*se.store, *de.store)) return true;
			}
		}
		return false;
	};

	auto deconstructGhost = [&](const minivec<EntityStore>& dst) {
		if (!dst.size()) return false;
		for (auto se: ghosts) {
			if (!se.en->isDeconstruction()) continue;
			for (auto de: dst) {
				if (collect(*se.store, *de.store)) return true;
			}
			for (auto de: dst) {
				if (overflow(*se.store, *de.store)) return true;
			}
		}
		return false;
	};

	auto repairDamage = [&](const minivec<EntityStore>& src) {
		for (auto se: src) {
			auto& store = *se.store;
			if (!store.hint.repairing) continue;
			for (auto& stack: store.stacks) {
				if (!Item::get(stack.iid)->repair) continue;
				if (se.en->spec->construction || store.countProviding(stack.iid)) {
					for (auto de: damaged) {
						dispatch(id, se.en->id, de->id, {stack.iid,1}, FlightRepair);
						return true;
					}
				}
			}
		}
		return false;
	};

	auto providerToRequester = [&](const minivec<EntityStore>& src, const minivec<EntityStore>& dst) {
		if (!src.size()) return false;
		if (!dst.size()) return false;
		for (auto se: src) for (auto de: dst) {
			if (supply(*se.store, *de.store)) return true;
		}
		return false;
	};

	auto providerBalance = [&](const minivec<EntityStore>& src, const minivec<EntityStore>& dst) {
		if (!src.size()) return false;
		if (!dst.size()) return false;
		for (auto se: src) for (auto de: dst) {
			if (supply(*se.store, *de.store)) return true;
		}
		for (auto se: src) for (auto de: dst) {
			if (overflow(*se.store, *de.store)) return true;
		}
		return false;
	};

	if (en->spec->store) {
		minivec<EntityStore> self = {{.id = id, .en = en, .store = &en->store(), .pos = en->pos()}};

		// repair damaged entities
		if (damaged.size() && repairDamageLocal()) return;

		// local store to ghost construction
		if (construction && constructGhostLocal()) return;

		// ghost deconstruction to local store
		if (deconstruction && deconstructGhostLocal()) return;

		// local store to provider
		if (providerBalance(self, stores)) return;

		// provider to local store
		if (providerBalance(stores, self)) return;
		if (providerToRequester(stores, self)) return;

		// buffer to local store
		for (auto se: stores) {
			if (se.en->isGhost()) continue;
			if (!se.en->spec->construction) continue;
			if (forceSupply(*se.store, en->store())) return;
		}
	}

	if (en->spec->depotAssist) {

		// repair local damaged entities
		if (damaged.size() && repairDamage(stores)) return;

		// any local store to ghost construction
		if (construction && constructGhost(stores)) return;

		// ghost deconstruction to any local provider
		if (deconstruction && deconstructGhost(stores)) return;

		// any local provider to any local requester
		if (providerToRequester(stores, stores)) return;

		// any local provider to any local provider
		if (providerBalance(stores, stores)) return;

		if (network && en->spec->networker) {
			minivec<EntityStore> networkStores;

			auto& networker = en->networker();
			for (auto& interface: networker.interfaces) {
				if (!interface.network) continue;
				networkStores.append(networkedStores[interface.network]);
			}

			// prioritise by proximity
			auto pos = en->pos();
			std::sort(networkStores.begin(), networkStores.end(), [&](const auto& a, const auto& b) {
				return pos.distanceSquared(a.pos) < pos.distanceSquared(b.pos);
			});

			// repair damaged entities
			if (damaged.size() && repairDamage(networkStores)) return;

			// any networked store to local ghost construction
			if (construction && constructGhost(networkStores)) return;

			// local ghost deconstruction to any networked provider
			if (deconstruction && deconstructGhost(networkStores)) return;

			// any local provider to any network requester
			if (providerToRequester(stores, networkStores)) return;

			// any network provider to any local requester
			if (providerToRequester(networkStores, stores)) return;

			// any network provider to any local provider
			if (providerBalance(networkStores, stores)) return;

			// any local provider to any network provider
			if (providerBalance(stores, networkStores)) return;
		}
	}

	cold.pause(this);
}

void Depot::dispatch(uint dep, uint src, uint dst, Stack stack, uint flags) {
	Entity& ed = Entity::create(Entity::next(), en->spec->depotDroneSpec);

	Point pos = en->pos() + en->spec->dronePoint;
	if (en->spec->dronePointRadius > 0) {
		real angle = Sim::random()*360.0;
		auto rot = Mat4::rotateY(glm::radians(angle));
		auto dir = (Point::South * en->spec->dronePointRadius).transform(rot);
		pos += dir;
	}

	ed.move(pos);
	ed.materialize();
	drones.insert(ed.id);

	en->consume(en->spec->depotDispatchEnergy);

	Drone& drone = ed.drone();
	drone.dep = dep;
	drone.src = src;
	drone.dst = dst;
	drone.stack = stack;
	drone.srcGhost = Entity::get(src).isGhost();
	drone.dstGhost = Entity::get(dst).isGhost();
	drone.repairing = flags & FlightRepair;
	drone.stage = Drone::ToSrc;

	if (src == en->id) {
		drone.stage = Drone::ToDst;
		en->store().remove(stack);
		drone.iid = stack.iid;
	}

	Entity &se = Entity::get(src);
	Store& ss = drone.srcGhost ? se.ghost().store: se.store();
	ss.reserve(stack);
	ss.drones.insert(ed.id);

	Entity &de = Entity::get(dst);

	if (!drone.repairing) {
		Store& ds = drone.dstGhost ? de.ghost().store: de.store();
		ds.promise(stack);
		ds.drones.insert(ed.id);
	} else {
		Entity::repairActions[de.id] = drone.id;
	}

	drone.range = std::max(
		se.pos().distance(pos),
		de.pos().distance(pos)
	) + 10.0f;

	hot.pause(this);
	nextDispatch = Sim::tick + (en->spec->zeppelin ? 5: 15);
}
