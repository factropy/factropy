#include "common.h"
#include "shipyard.h"

// Shipyard components attach additional behaviour to a Crafter, animating
// any Recipe with a Ship outputSpec after construction

void Shipyard::reset() {
	all.clear();
}

void Shipyard::tick() {
	for (auto& shipyard: all) shipyard.update();
}

Shipyard& Shipyard::create(uint id) {
	ensure(!all.has(id));
	Shipyard& shipyard = all[id];
	shipyard.id = id;
	shipyard.en = &Entity::get(id);
	ensure(shipyard.en->spec->crafter);
	shipyard.crafter = &shipyard.en->crafter();
	shipyard.ship = nullptr;
	shipyard.stage = Stage::Start;
	shipyard.completed = 0;
	return shipyard;
}

Shipyard& Shipyard::get(uint id) {
	return all.refer(id);
}

void Shipyard::destroy() {
	all.erase(id);
}

bool Shipyard::ready(Spec* spec) {
	return stage == Stage::Start && !ship;
}

void Shipyard::create(Spec* spec) {
	ship = spec;
	stage = Stage::Build;
	delta = 1;
	en->state = 0;
}

void Shipyard::complete(Spec* spec) {
	ship = spec;
	stage = Stage::RollOut;
}

void Shipyard::update() {
	if (en->isGhost()) return;

	switch (stage) {
		case Stage::Start: start(); break;
		case Stage::Build: build(); break;
		case Stage::RollOut: rollOut(); break;
		case Stage::Launch: launch(); break;
		case Stage::RollIn: rollIn(); break;
	}
}

Point Shipyard::shipPos() {
	switch (stage) {
		case Stage::Start: {
			return en->spec->shipyardBuild;
		}
		case Stage::Build: {
			return en->spec->shipyardBuild;
		}
		case Stage::RollOut: {
			int state = std::max(100, (int)en->state);
			float progress = (float)(state-100)/100.0f;
			return en->spec->shipyardBuild + ((en->spec->shipyardLaunch-en->spec->shipyardBuild) * progress);
		}
		case Stage::Launch: {
			return en->spec->shipyardLaunch;
		}
		case Stage::RollIn: {
			return Point::Zero;
		}
	}
	ensure(false);
	return Point::Zero;
}

bool Shipyard::shipGhost() {
	return stage == Stage::Start || stage == Stage::Build;
}

void Shipyard::start() {
	en->state = 0;
	delta = 1;
	ship = nullptr;
	if (crafter->working && crafter->recipe) {
		stage = Stage::Build;
		ship = crafter->recipe->outputSpec;
		ensure(ship && ship->ship);
		completed = crafter->completed;
	}
}

void Shipyard::build() {
	if (en->state <= 1) delta = 1;
	en->state += delta;
	if (en->state >= 99) delta = -1;
	if (!crafter->working) {
		stage = crafter->completed > completed ? Stage::RollOut: Stage::Start;
	}
}

void Shipyard::rollOut() {
	en->state++;
	if (en->state == 199) stage = Stage::Launch;
}

void Shipyard::launch() {
	ensure(ship);
	auto& es = Entity::create(Entity::next(), ship);
	es.move(shipPos().transform(en->dir().rotation()) + en->pos(), en->dir());
	es.materialize();
	es.spec->count.constructed++;
	stage = Stage::RollIn;
	ship = nullptr;
}

void Shipyard::rollIn() {
	en->state++;
	if (en->state == 299) stage = Stage::Start;
}

