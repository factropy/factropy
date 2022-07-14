#include "common.h"
#include "sim.h"
#include "explosive.h"

// Explosive components turn hills into flat ground after construction,
// and auto-remove when detonated.

void Explosive::reset() {
	active.clear();
	all.clear();
}

Explosive& Explosive::create(uint id) {
	ensure(!all.has(id));
	Explosive& explosive = all[id];
	explosive.id = id;
	active.insert(id);
	return explosive;
}

Explosive& Explosive::get(uint id) {
	return all.refer(id);
}

void Explosive::destroy() {
	active.erase(id);
	all.erase(id);
}

void Explosive::tick() {
	active.tick();
	for (auto pid: active) {
		get(pid).update();
	}
}

float Explosive::radius() {
	auto& en = Entity::get(id);
	ensure(en.spec->explosionSpec);
	return en.spec->explosionSpec->explosionRadius;
}

void Explosive::update() {
	active.pause(id);

	auto& en = Entity::get(id);
	if (en.isGhost()) return;

	auto r = radius();
	r = world.blast(Sphere(en.pos(), r));

	for (auto je: Entity::intersecting(Sphere(en.pos().floor(0.0), r))) {
		if (je->id == id) continue;
		if (!je->spec->junk && je->ground().y < 0.25f) continue;
		if (je->spec->drone) continue;
		if (je->spec->zeppelin) continue;
		if (je->spec->flightPath) continue;
		if (je->spec->missile) continue;
		real dist = je->pos().floor(0.0).distance(en.pos().floor(0.0));
		if (dist > r) continue;
		je->remove();
	}

	en.explode();
}
