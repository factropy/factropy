#include "common.h"
#include "sim.h"
#include "conveyor.h"
#include "loader.h"
#include "crew.h"

// Conveyor components move items around, automatically linking together to form belts.

std::size_t Conveyor::memory() {
	std::size_t size = 0U
		+ managed.memory()
		+ unmanaged.memory()
		+ ConveyorBelt::all.memory()
		+ ConveyorBelt::changed.memory()
	;
	for (auto belt: ConveyorBelt::all) {
		size += sizeof(ConveyorBelt);
		size += belt->conveyors.size() * sizeof(Conveyor);
	}
	return size;
}

void Conveyor::reset() {
	managed.clear();
	unmanaged.clear();
	leadersStraight.clear();
	leadersStraightSide.clear();
	leadersStraightNoSide.clear();
	leadersCircular.clear();
	changed.clear();
	extant.clear();
	for (auto belt: ConveyorBelt::all) delete belt;
	ConveyorBelt::all.clear();
	ConveyorBelt::changed.clear();
}

Conveyor& Conveyor::create(uint id) {
	ensure(!managed.has(id));
	ensure(!unmanaged.has(id));

	Entity& en = Entity::get(id);
	auto& conveyor = unmanaged[id];

	conveyor.id = id;
	conveyor.en = &en;
	conveyor.belt = nullptr;
	conveyor.left.slots = en.spec->conveyorSlotsLeft;
	conveyor.right.slots = en.spec->conveyorSlotsRight;
	ensure(en.spec->conveyorSlotsLeft <= 3);
	ensure(en.spec->conveyorSlotsRight <= 3);
	ensure(en.spec->conveyorTransformsLeft.size()/conveyor.left.slots < 65536);
	ensure(en.spec->conveyorTransformsRight.size()/conveyor.right.slots < 65536);
	conveyor.left.steps = en.spec->conveyorTransformsLeft.size()/conveyor.left.slots;
	conveyor.right.steps = en.spec->conveyorTransformsRight.size()/conveyor.right.slots;
	conveyor.prev = 0;
	conveyor.next = 0;
	conveyor.side = 0;
	conveyor.marked = false;

	en.cache.conveyor = &conveyor;

//	specs[en.spec]++;
	return conveyor;
}

Conveyor& Conveyor::get(uint id) {
	auto link = managed.point(id);
	if (link) {
		auto& conveyors = link->belt->conveyors;
		ensure(conveyors[link->offset].id == id);
		return conveyors[link->offset];
	}
	return unmanaged.refer(id);
}

void Conveyor::destroy() {
	ensure(!managed.has(id));
	ensure(unmanaged.has(id));
//	specs[en->spec]--;
	unmanaged.erase(id);
}

Conveyor::Conveyor(const Conveyor& other) {
	operator=(other);
}

Conveyor& Conveyor::operator=(const Conveyor& other) {
	id = other.id;
	en = other.en;
	belt = other.belt;
	prev = other.prev;
	next = other.next;
	side = other.side;
	left.slots = other.left.slots;
	left.steps = other.left.steps;
	left.items[0] = other.left.items[0];
	left.items[1] = other.left.items[1];
	left.items[2] = other.left.items[2];
	right.slots = other.right.slots;
	right.steps = other.right.steps;
	right.items[0] = other.right.items[0];
	right.items[1] = other.right.items[1];
	right.items[2] = other.right.items[2];
	marked = other.marked;
	return *this;
}

void Conveyor::tick() {
	if (changed.size()) {
		hashset<uint> affected;

		// When one or more conveyors have changed (placed, rotated, removed)
		// flood-fill from that position to find the affected belt lines, and
		// rebuild only as required.
		auto flood = [&](uint id) {
			if (affected.has(id)) return;
			if (!managed.has(id)) return;

			uint leader = id;

			while (id && get(id).next) {
				id = get(id).next;
				// circular belts
				if (id == leader) break;
			}

			leader = id;

			while (id) {
				affected.insert(id);
				id = get(id).prev;
				// circular belts
				if (id == leader) break;
			}
		};

		for (uint id: changed) {
			flood(id);
			leadersStraight.erase(id);
			leadersCircular.erase(id);
		}

		for (auto belt: ConveyorBelt::changed) {
			for (auto& conveyor: belt->conveyors) {
				// newly unmanaged
				if (!conveyor.belt) continue;
				flood(conveyor.id);
				leadersStraight.erase(conveyor.id);
				leadersCircular.erase(conveyor.id);
			}
		}

		hashset<uint> affectedLeadersStraight;
		hashset<uint> affectedLeadersCircular;

		for (auto& id: affected) {
			auto& conveyor = get(id);
			conveyor.marked = false;
			leadersStraight.erase(id);
			leadersCircular.erase(id);
		}

		// identify belt straight leaders
		for (auto& id: affected) {
			auto& leader = get(id);

			if (!leader.next) {
				affectedLeadersStraight.insert(leader.id);
				leader.marked = true;
				uint prev = leader.prev;
				while (prev) {
					Conveyor& before = get(prev);
					ensure(!before.marked);
					before.marked = true;
					prev = before.prev;
				}
			}
		}

		// identify belt circular leaders
		for (auto& id: affected) {
			auto& leader = get(id);

			if (!leader.marked) {
				ensure(leader.next && leader.prev);
				affectedLeadersCircular.insert(leader.id);
				leader.marked = true;
				uint prev = leader.prev;
				while (prev) {
					Conveyor& before = get(prev);
					ensure(before.next && before.prev);
					if (before.marked) break;
					before.marked = true;
					prev = before.prev;
				}
			}
		}

		hashset<ConveyorBelt*> oldBelts;
		hashset<ConveyorBelt*> newBelts;

		// identify old belts
		for (auto& id: affected) {
			auto& conveyor = get(id);
			if (conveyor.belt) {
				oldBelts.insert(conveyor.belt);
				conveyor.belt = nullptr;
			}
		}

		// identify old belts with only ghosts
		for (auto belt: ConveyorBelt::changed) {
			if (!oldBelts.has(belt)) {
				for (auto& conveyor: belt->conveyors) {
					ensure(!conveyor.belt);
				}
				oldBelts.insert(belt);
			}
		}

		// build new belts
		for (auto id: affectedLeadersStraight) {
			ConveyorBelt* belt = new ConveyorBelt;
			ConveyorBelt::all.insert(belt);
			newBelts.insert(belt);

			while (id) {
				auto& conveyor = get(id);
				belt->conveyors.push_back(conveyor);
				belt->conveyors.back().belt = belt;
				auto& link = managed[conveyor.id];
				link.belt = belt;
				link.offset = (uint)belt->conveyors.size()-1;
				id = conveyor.prev;
			}
		}

		// build new belts
		for (auto id: affectedLeadersCircular) {
			ConveyorBelt* belt = new ConveyorBelt;
			ConveyorBelt::all.insert(belt);
			newBelts.insert(belt);

			uint leader = id;
			while (id) {
				auto& conveyor = get(id);
				belt->conveyors.push_back(conveyor);
				belt->conveyors.back().belt = belt;
				auto& link = managed[conveyor.id];
				link.belt = belt;
				link.offset = (uint)belt->conveyors.size()-1;
				id = conveyor.prev;
				if (id == leader) break;
			}
		}

		// update related entities
		for (auto& id: affected) {
			auto& en = Entity::get(id);
			auto& conveyor = get(id);
			en.cache.conveyor = &conveyor;
//			if (conveyor.en->spec->loader)
//				conveyor.en->loader().conveyor = &conveyor;
//			if (conveyor.en->spec->balancer)
//				conveyor.en->balancer().conveyor = &conveyor;
		}

		// remove old belts
		for (auto belt: oldBelts) {
			ConveyorBelt::all.erase(belt);
			delete belt;
		}

//		for (auto belt: newBelts) {
//			for (auto& conveyor: belt->conveyors) {
//				managed[conveyor.id].belt = belt;
//				conveyor.belt = belt;
//			}
//		}

		for (auto id: affectedLeadersStraight) {
			leadersStraight.insert(id);
		}

		for (auto id: affectedLeadersCircular) {
			leadersCircular.insert(id);
		}

		leadersStraightSide.clear();
		leadersStraightNoSide.clear();

		// Have to check all leaders that side-load because any target conveyor
		// that has been removed won't be in the affected set
		for (auto id: leadersStraight) {
			Conveyor& leader = get(id);
			leader.belt->cside = leader.side ? &get(leader.side): nullptr;

			if (leader.belt->cside) {
				     if (leader.en->dir() == Point::South && leader.belt->cside->en->dir() == Point::East) leader.belt->sideLoad = SideLoadLeft;
				else if (leader.en->dir() == Point::South && leader.belt->cside->en->dir() == Point::West) leader.belt->sideLoad = SideLoadRight;
				else if (leader.en->dir() == Point::North && leader.belt->cside->en->dir() == Point::East) leader.belt->sideLoad = SideLoadRight;
				else if (leader.en->dir() == Point::North && leader.belt->cside->en->dir() == Point::West) leader.belt->sideLoad = SideLoadLeft;
				else if (leader.en->dir() == Point::East && leader.belt->cside->en->dir() == Point::South) leader.belt->sideLoad = SideLoadRight;
				else if (leader.en->dir() == Point::East && leader.belt->cside->en->dir() == Point::North) leader.belt->sideLoad = SideLoadLeft;
				else if (leader.en->dir() == Point::West && leader.belt->cside->en->dir() == Point::South) leader.belt->sideLoad = SideLoadLeft;
				else if (leader.en->dir() == Point::West && leader.belt->cside->en->dir() == Point::North) leader.belt->sideLoad = SideLoadRight;
				leadersStraightSide.push_back(id);
			}
			else {
				leadersStraightNoSide.push_back(id);
			}
		}

		for (auto belt: newBelts) {
			for (auto& conveyor: belt->conveyors) {
				if (conveyor.en->spec->conveyorEnergyDrain && !conveyor.en->spec->consumeElectricity) {
					belt->bulkConsumeSpec = conveyor.en->spec;
				}
			}
		}

		// SANITY check belts
//		uint checked = 0;
//		for (auto belt: ConveyorBelt::all) {
//			auto& leader = belt->conveyors[0];
//			if (leader.side) ensure(belt->cside == &get(leader.side));
//
//			for (auto& conveyor: belt->conveyors) {
//				ensure(conveyor.belt == belt && managed[conveyor.id].belt == belt);
//				checked++;
//			}
//		}
//		ensure(managed.size() == checked);

		notef("conveyor rebuild: changed: %llu, affected: %llu, changedBelts: %llu, oldBelts: %llu, newBelts: %llu, leadersStraight: %llu, leadersCircular: %llu",
			changed.size(), affected.size(), ConveyorBelt::changed.size(), oldBelts.size(), newBelts.size(), affectedLeadersStraight.size(), affectedLeadersCircular.size()
		);

		notef("conveyor totals: leadersStraight: %llu, leadersCircular: %llu, leadersStraightSide: %llu, leadersStraightNoSide: %llu",
			leadersStraight.size(), leadersCircular.size(), leadersStraightSide.size(), leadersStraightNoSide.size()
		);

		changed.clear();
		ConveyorBelt::changed.clear();
	}

	auto leaderUpdateLeft = [&](Conveyor& leader) {
		if (leader.side)
			leader.updateLeft();
		if (!leader.side && leader.belt->firstActiveLeft < leader.belt->conveyors.size())
			leader.belt->conveyors[leader.belt->firstActiveLeft].updateLeft();
	};

	auto leaderUpdateRight = [&](Conveyor& leader) {
		if (leader.side)
			leader.updateRight();
		if (!leader.side && leader.belt->firstActiveRight < leader.belt->conveyors.size())
			leader.belt->conveyors[leader.belt->firstActiveRight].updateRight();
	};

	auto leaderUpdate = [&](uint id) {
		Conveyor& leader = get(id);
		if (Sim::tick%2) {
			leaderUpdateLeft(leader);
			leaderUpdateRight(leader);
		} else {
			leaderUpdateRight(leader);
			leaderUpdateLeft(leader);
		}
	};

	channel<bool,-1> done;

	auto advance = [&](uint i, uint l) {
		for (; i < l; i++) leaderUpdate(leadersStraightNoSide[i]);
		done.send(true);
	};

	// Belts that don't side offload to another belt are
	// self-contained and can be updated in parallel
	uint s = leadersStraightNoSide.size();
	uint b = s/4;

	if (s > 100) {
		crew.job([&]() { advance(  0,   b); });
		crew.job([&]() { advance(  b, b*2); });
		crew.job([&]() { advance(b*2, b*3); });
		crew.job([&]() { advance(b*3,   s); });
	} else {
		advance(0, s);
	}

	// Circular belts go nowhere if completely full!
	// So nudge things along by moving the first item
	for (auto id: leadersCircular) {
		Conveyor& leader = get(id);

		if (leader.left.items[0].iid && leader.left.items[0].offset == 0) {
			uint iid = leader.left.items[0].iid;
			leader.left.items[0].iid = 0;
			leader.updateLeft();
			Conveyor& follower = get(leader.next);
			follower.left.items[follower.left.slots-1].iid = iid;
			follower.left.items[follower.left.slots-1].offset = follower.left.steps-1;
			continue;
		}

		if (leader.left.items[0].iid && leader.left.items[0].offset > 0) {
			uint iid = leader.left.items[0].iid;
			uint offset = leader.left.items[0].offset;
			leader.left.items[0].iid = 0;
			leader.left.items[0].offset = 0;
			leader.updateLeft();
			leader.left.items[0].iid = iid;
			leader.left.items[0].offset = offset-1;
			continue;
		}

		leader.updateLeft();
	}

	for (auto id: leadersCircular) {
		Conveyor& leader = get(id);

		if (leader.right.items[0].iid && leader.right.items[0].offset == 0) {
			uint iid = leader.right.items[0].iid;
			leader.right.items[0].iid = 0;
			leader.updateRight();
			Conveyor& follower = get(leader.next);
			follower.right.items[follower.right.slots-1].iid = iid;
			follower.right.items[follower.right.slots-1].offset = follower.right.steps-1;
			continue;
		}

		if (leader.right.items[0].iid && leader.right.items[0].offset > 0) {
			uint iid = leader.right.items[0].iid;
			uint offset = leader.right.items[0].offset;
			leader.right.items[0].iid = 0;
			leader.right.items[0].offset = 0;
			leader.updateRight();
			leader.right.items[0].iid = iid;
			leader.right.items[0].offset = offset-1;
			continue;
		}

		leader.updateRight();
	}

	if (s > 100) {
		done.recv();
		done.recv();
		done.recv();
		done.recv();
	}

	// leaders that side offload onto another belt can't
	// be updated in parallel as they may be accessing
	// the same target belt
	for (auto id: leadersStraightSide) {
		leaderUpdate(id);
	}

	// consume energy in bulk for the length of each belt, using the first normal conveyor
	// that isn't a more complex entity with its own consumption (tube, balancer etc) as a
	// baseline.
	auto network = ElectricityNetwork::primary();
	if (network) for (auto belt: ConveyorBelt::all) {
		if (!belt->bulkConsumeSpec) continue;
		network->consume(belt->bulkConsumeSpec, belt->bulkConsumeSpec->conveyorEnergyDrain, belt->conveyors.size());
	}
}

// input absolute location
Point Conveyor::input() {
	return en->spec->conveyorInput.transform(en->dir().rotation()) + en->pos();
}

// output absolute location
Point Conveyor::output() {
	return en->spec->conveyorOutput.transform(en->dir().rotation()) + en->pos();
}

// offset in our own belt
uint Conveyor::offset() {
	return belt ? this - belt->conveyors.data(): 0;
}

// last place? consider circular belts
bool Conveyor::last() {
	return !belt || !prev || this == &belt->conveyors.back();
}

// leader with nowhere to go
bool Conveyor::deadEnd() {
	return !next && !side;
}

// A new or recently-extant conveyor has been placed, so link it up with neighbours
Conveyor& Conveyor::manage() {
	ensure(!managed.has(id));
	ensure(unmanaged.has(id));
	ensure(!belt);

	ensure(!prev);
	ensure(!next);

	ensure(extant[en->spec] >= 0);
	extant[en->spec]++;

	changed.insert(id);

	Box ib = input().box().grow(0.1f);
	Box ob = output().box().grow(0.1f);

	for (auto eo: Entity::intersecting(ob, Entity::gridConveyors)) {
		if (eo->id == id) continue;
		if (eo->isGhost()) continue;
		Conveyor& co = eo->conveyor();
		if (!managed.has(co.id)) continue;
		if (co.input().box().grow(0.1f).intersects(ob)) {
			ensure(!co.prev);
			co.prev = id;
			next = eo->id;
			changed.insert(eo->id);
			break;
		}
	}

	for (auto ei: Entity::intersecting(ib, Entity::gridConveyors)) {
		if (ei->id == id) continue;
		if (ei->isGhost()) continue;
		Conveyor& ci = ei->conveyor();
		if (!managed.has(ci.id)) continue;
		if (ci.output().box().grow(0.1f).intersects(ib)) {
			ensure(!ci.next);
			ci.next = id;
			prev = ei->id;
			changed.insert(ei->id);
			break;
		}
	}

	// another sideloading here
	for (auto eo: Entity::intersecting(en->box().grow(0.5), Entity::gridConveyors)) {
		if (eo->id == id || eo->id == prev || eo->id == next) continue;
		if (eo->isGhost()) continue;
		if (eo->dir().cardinalParallel(en->dir())) continue;
		Conveyor& co = eo->conveyor();
		if (!managed.has(co.id)) continue;
		if (en->box().intersects(co.output().box().grow(0.1f))) {
			ensure(!co.side);
			co.side = id;
			changed.insert(co.side);
		}
	}

	// here sideloading to another
	if (!next && !side) {
		for (auto eo: Entity::intersecting(ob, Entity::gridConveyors)) {
			if (eo->id == id) continue;
			if (eo->isGhost()) continue;
			Conveyor& co = eo->conveyor();
			if (!managed.has(co.id)) continue;
			if (eo->dir().cardinalParallel(en->dir())) continue;
			side = eo->id;
			changed.insert(eo->id);
			break;
		}
	}

	belt = new ConveyorBelt;
	ConveyorBelt::all.insert(belt);
	ConveyorBelt::changed.insert(belt);
	belt->conveyors.push_back(*this);
	belt->conveyors.back().belt = belt;
	auto& link = managed[id];
	link.belt = belt;
	link.offset = (uint)belt->conveyors.size()-1;
	unmanaged.erase(id);

	en->cache.conveyor = &get(id);

	return get(id);
}

// A conveyor is being deconstructed or destroyed, so unlink it from neighbours
Conveyor& Conveyor::unmanage() {
	ensure(managed.has(id));
	ensure(!unmanaged.has(id));
	ensure(belt);

	extant[en->spec]--;
	ensure(extant[en->spec] >= 0);

	changed.insert(id);

	if (prev) {
		ensure(get(prev).next == id);
		get(prev).next = 0;
		changed.insert(prev);
		prev = 0;
	}

	if (next) {
		ensure(get(next).prev == id);
		get(next).prev = 0;
		changed.insert(next);
		next = 0;
	}

	side = 0;

	for (auto eo: Entity::intersecting(en->box().grow(0.5))) {
		if (eo->isGhost()) continue;
		if (eo->spec->conveyor) {
			Conveyor& co = get(eo->id);
			if (co.side == id) {
				ensure(managed.has(co.id));
				changed.insert(co.side);
				co.side = 0;
			}
		}
	}

	ConveyorBelt::changed.insert(belt);
	belt = nullptr;

	unmanaged[id] = *this;
	managed.erase(id);

	en->cache.conveyor = &get(id);

	return get(id);
}

bool ConveyorSegment::insert(uint slot, uint iid) {
	if (slots > slot && !items[slot].iid) {
		items[slot].iid = iid;
		items[slot].offset = steps/2;
		return true;
	}
	return false;
}

bool ConveyorSegment::deliver(uint iid) {
	uint slot = slots-1;
	if (!items[slot].iid) {
		items[slot].iid = iid;
		items[slot].offset = steps-1;
		return true;
	}
	return false;
}

bool ConveyorSegment::remove(uint iid) {
	for (uint i = 0; i < slots; i++) {
		if (items[i].iid == iid) {
			items[i].iid = 0;
			items[i].offset = 0;
			return true;
		}
	}
	return false;
}

bool ConveyorSegment::removeBack(uint iid) {
	for (uint i = 1; i < slots; i++) {
		if (items[i].iid == iid) {
			items[i].iid = 0;
			items[i].offset = 0;
			return true;
		}
	}
	return false;
}

uint ConveyorSegment::removeAny() {
	for (uint i = 0; i < slots; i++) {
		if (items[i].iid) {
			uint iid = items[i].iid;
			items[i].iid = 0;
			items[i].offset = 0;
			return iid;
		}
	}
	return 0;
}

uint ConveyorSegment::removeAnyBack() {
	for (uint i = 1; i < slots; i++) {
		if (items[i].iid) {
			uint iid = items[i].iid;
			items[i].iid = 0;
			items[i].offset = 0;
			return iid;
		}
	}
	return 0;
}

uint ConveyorSegment::count() {
	uint count = 0;
	for (uint i = 0; i < slots; i++) {
		if (items[i].iid) count++;
	}
	return count;
}

uint ConveyorSegment::countBack() {
	uint count = 0;
	for (uint i = 1; i < slots; i++) {
		if (items[i].iid) count++;
	}
	return count;
}

uint ConveyorSegment::countFront() {
	return items[0].iid ? 1:0;
}

uint ConveyorSegment::offloading() {
	return items[0].iid && items[0].offset == steps/2 ? items[0].iid: 0;
}

bool ConveyorSegment::offload(uint iid) {
	if (items[0].iid == iid && items[0].offset == steps/2) {
		items[0].iid = 0;
		items[0].offset = 0;
		return true;
	}
	return false;
}

void ConveyorSegment::flush() {
	for (uint i = 0; i < slots; i++) {
		items[i].iid = 0;
		items[i].offset = 0;
	}
}

bool ConveyorSegment::deliverable() {
	uint slot = slots-1;
	return items[slot].iid == 0;
}

bool ConveyorSegment::update(ConveyorSegment* snext, ConveyorSegment* sprev, ConveyorSegment* sside, uint slot, bool* stopped) {
	bool blocked = !snext && !sside;
	bool movement = false;

	while (items[0].iid) {
		uint snextBack = snext ? snext->slots-1: 0;

		if (snext && snext->items[snextBack].iid) {
			float a = (float)items[0].offset / (float)steps;
			float b = (float)snext->items[snextBack].offset / (float)snext->steps;
			uint aa = std::floor(a*100.0f);
			uint bb = std::floor(b*100.0f);
			if (aa <= bb) { blocked = true; break; }
		}

		if (!snext && items[0].offset <= steps/2 && !sside) {
			blocked = true;
			break;
		}

		if (items[0].offset > 0) {
			movement = true;
			items[0].offset--;
			break;
		}

		if (snext && snext->deliver(items[0].iid)) {
			movement = true;
			items[0].iid = 0;
			items[0].offset = 0;
			break;
		}

		if (sside && sside->insert(slot, items[0].iid)) {
			movement = true;
			items[0].iid = 0;
			items[0].offset = 0;
			break;
		}

		blocked = true;

		break;
	}

	uint count = items[0].iid ? 1:0;

	for (uint i = 1; i < slots; i++) {
		if (!items[i].iid) continue;
		uint p = i-1;
		count++;

		if (items[p].offset && items[i].offset <= items[p].offset) {
			continue;
		}

		if (items[i].offset > 0) {
			movement = true;
			items[i].offset--;
			continue;
		}

		if (!items[p].iid) {
			movement = true;
			items[p].iid = items[i].iid;
			items[p].offset = steps-1;
			items[i].iid = 0;
			items[i].offset = 0;
			continue;
		}
	}

	*stopped = blocked && !movement && count > 0;

	bool backedUp = blocked && !movement && count == slots;

	return !backedUp;
}

void Conveyor::updateLeft() {
	bool gap = left.update(
		next ? &(this-1)->left: nullptr,
		prev ? &(this+1)->left: nullptr,
		side ? (belt->sideLoad == SideLoadLeft ? &belt->cside->left: &belt->cside->right): nullptr,
		side && belt->sideLoad == SideLoadLeft ? 0: 1,
		&blockedLeft
	);

	if (gap) belt->activeLeft(offset());

	if (!gap && belt->firstActiveLeft == offset()) belt->firstActiveLeft++;

	if (last()) return;

	(this+1)->updateLeft();
}

void Conveyor::updateRight() {
	bool gap = right.update(
		next ? &(this-1)->right: nullptr,
		prev ? &(this+1)->right: nullptr,
		side ? (belt->sideLoad == SideLoadLeft ? &belt->cside->left: &belt->cside->right): nullptr,
		side && belt->sideLoad == SideLoadLeft ? 1: 0,
		&blockedRight
	);

	if (gap) belt->activeRight(offset());

	if (!gap && belt->firstActiveRight == offset()) belt->firstActiveRight++;

	if (last()) return;

	(this+1)->updateRight();
}

// Put an item on the back of this conveyor
bool Conveyor::insertLeft(uint slot, uint iiid) {
	if (left.insert(slot, iiid)) {
		belt->activeLeft(offset());
		return true;
	}
	return false;
}

// Put an item on the back of this conveyor
bool Conveyor::insertRight(uint slot, uint iiid) {
	if (right.insert(slot, iiid)) {
		belt->activeRight(offset());
		return true;
	}
	return false;
}

// Put an item on the back of this conveyor
bool Conveyor::insert(uint load, uint iiid) {
	if (load == SideLoadLeft) return insertLeft(0, iiid);
	if (load == SideLoadRight) return insertRight(0, iiid);
	return false;
}

// Put an item on this conveyor from direction
bool Conveyor::insertFrom(Point pos, uint iiid) {
	switch (en->pos().relative(en->dir(), pos)) {
		case Point::Ahead: return insertLeft(1, iiid) || insertLeft(0, iiid);
		case Point::Behind: return insertRight(1, iiid) || insertRight(0, iiid);
		case Point::Left: return insertLeft(1, iiid) || insertLeft(0, iiid);
		case Point::Right: return insertRight(1, iiid) || insertRight(0, iiid);
	}
	return false;
}

// Put an item on this conveyor from direction
bool Conveyor::insertNear(Point pos, uint iiid) {
	switch (en->pos().relative(en->dir(), pos)) {
		case Point::Ahead: return insertLeft(1, iiid);
		case Point::Behind: return insertRight(1, iiid);
		case Point::Left: return insertLeft(1, iiid);
		case Point::Right: return insertRight(1, iiid);
	}
	return false;
}

// Put an item on this conveyor from direction
bool Conveyor::insertFar(Point pos, uint iiid) {
	switch (en->pos().relative(en->dir(), pos)) {
		case Point::Ahead: return insertRight(1, iiid);
		case Point::Behind: return insertLeft(1, iiid);
		case Point::Left: return insertRight(1, iiid);
		case Point::Right: return insertLeft(1, iiid);
	}
	return false;
}

// Put an item anywhere on this conveyor
bool Conveyor::insertAny(uint iiid) {
	return insertAnyFront(iiid) || insertAnyBack(iiid);
}

bool Conveyor::insertAnyFront(uint iiid) {
	if (left.count() < right.count()) {
		return insertLeft(0, iiid) || insertRight(0, iiid);
	} else {
		return insertRight(0, iiid) || insertLeft(0, iiid);
	}
}

bool Conveyor::insertAnyBack(uint iiid) {
	if (left.count() < right.count()) {
		return insertLeft(1, iiid) || insertRight(1, iiid);
	} else {
		return insertRight(1, iiid) || insertLeft(1, iiid);
	}
}

void ConveyorBelt::activeLeft(uint offset) {
	ensure(offset <= conveyors.size());
	firstActiveLeft = std::min(firstActiveLeft, offset);
}

void ConveyorBelt::activeRight(uint offset) {
	ensure(offset <= conveyors.size());
	firstActiveRight = std::min(firstActiveRight, offset);
}

// Get an item from the back of this conveyor
bool Conveyor::removeLeft(uint iiid) {
	if (left.remove(iiid)) {
		belt->activeLeft(offset());
		return true;
	}
	return false;
}

// Get an item from the back of this conveyor
bool Conveyor::removeLeftBack(uint iiid) {
	if (left.removeBack(iiid)) {
		belt->activeLeft(offset());
		return true;
	}
	return false;
}

uint Conveyor::removeLeftAny() {
	uint iid = left.removeAny();
	if (iid) belt->activeLeft(offset());
	return iid;
}

// Get an item from the back of this conveyor
bool Conveyor::removeRight(uint iiid) {
	if (right.remove(iiid)) {
		belt->activeRight(offset());
		return true;
	}
	return false;
}

// Get an item from the back of this conveyor
bool Conveyor::removeRightBack(uint iiid) {
	if (right.removeBack(iiid)) {
		belt->activeRight(offset());
		return true;
	}
	return false;
}

uint Conveyor::removeRightAny() {
	uint iid = right.removeAny();
	if (iid) belt->activeRight(offset());
	return iid;
}

// Remove an item from the front of this conveyor
bool Conveyor::remove(uint load, uint iiid) {
	if (load == SideLoadLeft) return removeLeft(iiid);
	if (load == SideLoadRight) return removeRight(iiid);
	return false;
}

// Get an item from this conveyor from direction
bool Conveyor::removeFar(Point pos, uint iiid) {
	switch (en->pos().relative(en->dir(), pos)) {
		case Point::Ahead: return removeRight(iiid);
		case Point::Behind: return removeLeft(iiid);
		case Point::Left: return removeRight(iiid);
		case Point::Right: return removeLeft(iiid);
	}
	return false;
}

// Get an item from this conveyor from direction
bool Conveyor::removeNear(Point pos, uint iiid) {
	switch (en->pos().relative(en->dir(), pos)) {
		case Point::Ahead: return removeLeft(iiid);
		case Point::Behind: return removeRight(iiid);
		case Point::Left: return removeLeft(iiid);
		case Point::Right: return removeRight(iiid);
	}
	return false;
}

// Remove an item from the front of this conveyor
bool Conveyor::remove(uint iiid) {
	if (Sim::tick%2) {
		if (left.remove(iiid)) { belt->activeLeft(offset()); return true; }
		if (right.remove(iiid)) { belt->activeRight(offset()); return true; }
	} else {
		if (right.remove(iiid)) { belt->activeRight(offset()); return true; }
		if (left.remove(iiid)) { belt->activeLeft(offset()); return true; }
	}
	return false;
}

uint Conveyor::removeAny() {
	uint iid = 0;
	if (Sim::tick%2) {
		if (!iid) { iid = left.removeAny(); if (iid) belt->activeLeft(offset()); }
		if (!iid) { iid = right.removeAny(); if (iid) belt->activeRight(offset()); }
	} else {
		if (!iid) { iid = right.removeAny(); if (iid) belt->activeRight(offset()); }
		if (!iid) { iid = left.removeAny(); if (iid) belt->activeLeft(offset()); }
	}
	return iid;
}

uint Conveyor::removeAnyBack() {
	uint iid = 0;
	if (Sim::tick%2) {
		if (!iid) { iid = left.removeAnyBack(); if (iid) belt->activeLeft(offset()); }
		if (!iid) { iid = right.removeAnyBack(); if (iid) belt->activeRight(offset()); }
	} else {
		if (!iid) { iid = right.removeAnyBack(); if (iid) belt->activeRight(offset()); }
		if (!iid) { iid = left.removeAnyBack(); if (iid) belt->activeLeft(offset()); }
	}
	return iid;
}

uint Conveyor::countFront() {
	return left.countFront() + right.countFront();
}

uint Conveyor::countBack() {
	return left.countBack() + right.countBack();
}

// Pop an item from the front this conveyor
bool Conveyor::offloadLeft(uint iid) {
	if (left.offload(iid)) {
		belt->activeLeft(offset());
		return true;
	}
	return false;
}

// Pop an item from the front this conveyor
bool Conveyor::offloadRight(uint iid) {
	if (right.offload(iid)) {
		belt->activeRight(offset());
		return true;
	}
	return false;
}

// Push an item on the back of this conveyor
bool Conveyor::deliverLeft(uint iid) {
	if (left.deliver(iid)) {
		belt->activeLeft(offset());
		return true;
	}
	return false;
}

// Push an item on the back of this conveyor
bool Conveyor::deliverRight(uint iid) {
	if (right.deliver(iid)) {
		belt->activeRight(offset());
		return true;
	}
	return false;
}

bool Conveyor::blocked() {
	return blockedLeft && blockedRight;
}

bool Conveyor::empty() {
	for (uint i = 0; i < 3; i++) {
		if (i < left.slots && left.items[i].iid) return false;
		if (i < right.slots && right.items[i].iid) return false;
	}
	return true;
}

bool Conveyor::full() {
	for (uint i = 0; i < 3; i++) {
		if (i < left.slots && !left.items[i].iid) return false;
		if (i < right.slots && !right.items[i].iid) return false;
	}
	return true;
}

// prevent items leaving conveyor
void Conveyor::hold() {
	if (left.slots && left.items[0].iid) left.items[0].offset = std::max(left.items[0].offset, (uint16_t)(left.steps/2));
	if (right.slots && right.items[0].iid) right.items[0].offset = std::max(right.items[0].offset, (uint16_t)(right.steps/2));
}

std::vector<uint> Conveyor::items() {
	std::vector<uint> items;
	for (uint i = 0; i < 3; i++) {
		if (Sim::tick%2) {
			if (i < left.slots && left.items[i].iid) items.push_back(left.items[i].iid);
			if (i < right.slots && right.items[i].iid) items.push_back(right.items[i].iid);
		} else {
			if (i < right.slots && right.items[i].iid) items.push_back(right.items[i].iid);
			if (i < left.slots && left.items[i].iid) items.push_back(left.items[i].iid);
		}
	}
	return items;
}

void Conveyor::flush() {
	if (belt) belt->flush();
}

void Conveyor::upgrade(uint uid) {
	auto& uc = get(uid);

	ensure(left.items);
	ensure(uc.left.items);
	ensure(right.items);
	ensure(uc.right.items);

	for (uint i = 0; i < left.slots && i < uc.left.slots; i++) {
		left.items[i].iid = uc.left.items[i].iid;
		float offset = (float)(uc.left.items[i].offset) / (float)(uc.left.steps);
		left.items[i].offset = offset * (float)(left.steps);
	}

	for (uint i = 0; i < right.slots && i < uc.right.slots; i++) {
		right.items[i].iid = uc.right.items[i].iid;
		float offset = (float)(uc.right.items[i].offset) / (float)(uc.right.steps);
		right.items[i].offset = offset * (float)(right.steps);
	}
}

void ConveyorBelt::flush() {
	for (auto& conveyor: conveyors) {
		conveyor.left.flush();
		conveyor.right.flush();
	}
	activeLeft(0);
	activeRight(0);
}

std::vector<uint> Conveyor::upgradableGroup(uint from) {
	std::vector<uint> ids;
	if (!from) return ids;
	if (!managed.has(from)) return ids;

	auto& first = get(from);
	ensure(first.belt);
	auto belt = first.belt;

	int offset = &first - belt->conveyors.data();
	int low = offset;
	int high = offset;

	auto spec = Entity::get(from).spec;
	auto& group = spec->upgradeCascade;

	while (low > 0 && group.count(Entity::get(belt->conveyors[low-1].id).spec)) {
		low--;
	}

	ensure(belt->conveyors.size())
	int limit = (int)belt->conveyors.size();

	while (high < limit-1 && group.count(Entity::get(belt->conveyors[high+1].id).spec)) {
		high++;
	}

	while (low <= high) {
		auto& conveyor = belt->conveyors[low++];
		if (group.count(Entity::get(conveyor.id).spec))
			ids.push_back(conveyor.id);
	}

	return ids;
}