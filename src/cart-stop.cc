#include "common.h"
#include "cart-stop.h"

// CartStop components are markers with rules used by Carts to load/unload

void CartStop::tick() {
	for (auto& stop: all) stop.update();
}

void CartStop::reset() {
	all.clear();
}

CartStop& CartStop::create(uint id) {
	ensure(!all.has(id));
	CartStop& stop = all[id];
	stop.id = id;
	stop.en = &Entity::get(id);
	stop.depart = Depart::Inactivity;
	return stop;
}

CartStop& CartStop::get(uint id) {
	return all.refer(id);
}

void CartStop::destroy() {
	all.erase(id);
}

CartStopSettings* CartStop::settings() {
	return new CartStopSettings(*this);
}

CartStopSettings::CartStopSettings(CartStop& stop) {
	depart = stop.depart;
}

void CartStop::setup(CartStopSettings* settings) {
	depart = settings->depart;
}

void CartStop::update() {
	if (en->isGhost()) return;
}
