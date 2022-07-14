#include "common.h"
#include "sim.h"
#include "pile.h"

// Pile components turn lakes into flat ground after construction,
// and auto remove when fully under ground.

void Pile::reset() {
	active.clear();
	all.clear();
}

Pile& Pile::create(uint id) {
	ensure(!all.has(id));
	Pile& pile = all[id];
	pile.id = id;
	active.insert(id);
	return pile;
}

Pile& Pile::get(uint id) {
	return all.refer(id);
}

void Pile::destroy() {
	active.erase(id);
	all.erase(id);
}

void Pile::tick() {
	active.tick();
	for (auto pid: active) {
		get(pid).update();
	}
}

void Pile::update() {
	active.pause(id);

	auto& en = Entity::get(id);
	if (en.isGhost()) return;

	Box fill = en.box().shrink(0.1f);
	if (!world.isLand(fill)) world.flatten(fill);

	Box surround = en.box().grow(0.1f);
	if (world.isLand(surround)) en.remove();
}

