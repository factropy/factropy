#include "common.h"
#include "ghost.h"
#include "entity.h"
#include "sim.h"

void Ghost::reset() {
	all.clear();
}

void Ghost::tick() {
	for (auto& ghost: all) {
		ghost.update();
	}
}

Ghost& Ghost::create(uint id, uint sid) {
	ensure(!all.has(id));
	Ghost& ghost = all[id];
	ghost.id = id;
	ghost.store.ghostInit(id, sid);
	return ghost;
}

Ghost& Ghost::get(uint id) {
	return all.refer(id);
}

void Ghost::destroy() {
	store.ghostDestroy();
	all.erase(id);
}

bool Ghost::isConstruction() {
	return Entity::get(id).isConstruction();
}

bool Ghost::isDeconstruction() {
	return Entity::get(id).isDeconstruction();
}

void Ghost::update() {
	Entity& en = Entity::get(id);

	if (en.isConstruction() && store.isRequesterSatisfied()) {
		en.complete();
		en.materialize();
		en.spec->count.constructed++;
		return;
	}

	if (en.isDeconstruction() && store.isEmpty()) {
		en.destroy();
		return;
	}
}
