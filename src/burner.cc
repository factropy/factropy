#include "common.h"
#include "entity.h"
#include "burner.h"

// Burner components convert fuel items into energy. They include a Store as
// a sub-component to hold the fuel.

void Burner::reset() {
	all.clear();
}

Burner& Burner::create(uint id, uint sid, std::string category) {
	ensure(!all.has(id));
	Burner& burner = all[id];
	burner.id = id;
	burner.energy = 0;
	burner.buffer = Energy::MJ(1);
	burner.store.burnerInit(id, sid, Mass::kg(3), category);
	return burner;
}

Burner& Burner::get(uint id) {
	return all.refer(id);
}

void Burner::destroy() {
	store.burnerDestroy();
	all.erase(id);
}

Energy Burner::consume(Energy e) {
	while (e > energy && refuel());
	Energy c = std::min(energy, e);
	energy = std::max(Energy(0), energy-c);
	return c;
}

bool Burner::refuel() {
	Stack stack = store.removeFuel(store.fuelCategory, 1);
	if (!stack.iid) {
		Entity& en = Entity::get(id);
		if (en.spec->store) {
			stack = en.store().removeFuel(store.fuelCategory, 1);
		}
	}
	if (stack.iid && stack.size) {
		buffer = Item::get(stack.iid)->fuel.energy;
		Item::get(stack.iid)->consume(1);
		energy += buffer;
		return true;
	}
	return false;
}

bool Burner::fueled() {
	if (energy == Energy(0)) refuel();
	return energy > Energy(0);
}
