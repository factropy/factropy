#include "common.h"
#include "sim.h"
#include "unveyor.h"

// Unveyor (underground conveyor) components link up with Conveyors to allow
// short distance underground belts.

void Unveyor::reset() {
	all.clear();
}

void Unveyor::tick() {
	for (auto& unveyor: all) {
		unveyor.update();
	}
}

Unveyor& Unveyor::create(uint id) {
	ensure(!all.has(id));
	Entity& en = Entity::get(id);
	Unveyor& unveyor = all[id];
	unveyor.id = id;
	unveyor.partner = 0;
	unveyor.entry = en.spec->unveyorEntry;
	return unveyor;
}

Unveyor& Unveyor::get(uint id) {
	return all.refer(id);
}

void Unveyor::destroy() {
	ensure(!partner);
	all.erase(id);
}

Box Unveyor::range() {
	Entity& en = Entity::get(id);
	Point dir = entry ? en.dir(): -en.dir();
	Point far = (dir * en.spec->unveyorRange) + en.pos();
	return Box(en.pos(), far).grow(0.1f);
}

void Unveyor::manage() {
	if (!partner) {
		float dist = 0.0f;
		Entity& en = Entity::get(id);

		for (auto eo: Entity::intersecting(range())) {
			if (id == eo->id) continue;
			if (!all.has(eo->id)) continue;
			Unveyor& other = get(eo->id);
			if (other.partner) continue;
			if (eo->isGhost()) continue;
			if (en.spec->unveyorPartner != eo->spec) continue;
			if (!(en.dir() == eo->dir() && entry != other.entry)) continue;
			float d = en.pos().distance(eo->pos());
			if (!partner || d < dist) {
				partner = eo->id;
				dist = d;
			}
		}

		if (partner) {
			get(partner).partner = id;
			link();
		}
	}
}

void Unveyor::unmanage() {
	if (partner) {
		Unveyor& other = get(partner);
		ensure(other.partner == id);

		unlink();

		other.partner = 0;
		partner = 0;
	}
}

void Unveyor::link() {
	ensure(partner);
	ensure(get(partner).partner == id);
}

void Unveyor::unlink() {
	ensure(partner);

	Unveyor& other = get(partner);
	ensure(other.partner == id);
}

void Unveyor::update() {
	if (!partner || !entry) return;

	auto& send = Conveyor::get(id);
	auto& recv = Conveyor::get(partner);

	if (!send.blocked() && !send.empty()) {
		send.en->consume(send.en->spec->conveyorEnergyDrain * send.en->pos().distance(recv.en->pos()));
	}

	if (left && recv.deliverLeft(left)) {
		left = 0;
	}

	if (right && recv.deliverRight(right)) {
		right = 0;
	}

	if (!left) {
		left = send.left.offloading();
		if (left) ensure(send.offloadLeft(left));
	}

	if (!right) {
		right = send.right.offloading();
		if (right) ensure(send.offloadRight(right));
	}
}
