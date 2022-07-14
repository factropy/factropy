#include "common.h"
#include "entity.h"
#include "explosion.h"

// Explosion components deal damage within an expanding sphere

void Explosion::reset() {
	all.clear();
}

void Explosion::tick() {
	for (auto& explosion: all) {
		explosion.update();
	}
}

Explosion& Explosion::create(uint id) {
	ensure(!all.has(id));
	Explosion& explosion = all[id];
	explosion.id = id;
	explosion.radius = 0;
	explosion.range = 0;
	explosion.rate = 0;
	explosion.damage = 0;
	return explosion;
}

Explosion& Explosion::get(uint id) {
	return all.refer(id);
}

void Explosion::destroy() {
	all.erase(id);
}

void Explosion::update() {
	Entity& en = Entity::get(id);
	if (en.isGhost()) return;

	if (radius >= range) {
		for (auto te: Entity::intersecting(en.pos(), range)) {
			if (te->id == id) continue;
			te->damage(damage);
		}
		en.remove();
		return;
	}

	radius += rate;
}

void Explosion::define(Health ddamage, float rradius, float rrate) {
	damage = ddamage;
	radius = 0.0;
	range = rradius;
	rate = rrate;
}
