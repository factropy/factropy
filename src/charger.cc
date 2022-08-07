#include "common.h"
#include "entity.h"
#include "charger.h"
#include "powerpole.h"

// Charger components buffer electricity for local or grid consumption.

void Charger::tick() {
	for (auto& charger: all) charger.charge();
}

void Charger::reset() {
	all.clear();
}

Charger& Charger::create(uint id) {
	ensure(!all.has(id));
	Charger& charger = all[id];
	charger.id = id;
	charger.en = &Entity::get(id);
	charger.energy = 0;
	charger.buffer = charger.en->spec->consumeChargeBuffer;
	return charger;
}

Charger& Charger::get(uint id) {
	return all.refer(id);
}

void Charger::destroy() {
	all.erase(id);
}

Energy Charger::chargeRate() {
	float effect = en->spec->consumeChargeEffect ? Effector::effect(en->id, Effector::CHARGE): 1.0f;
	return en->spec->consumeChargeRate * effect;
}

void Charger::charge() {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;

	auto pole = PowerPole::covering(en->box());
	auto network = pole ? pole->network: nullptr;

	if (network && energy < buffer) {
		Energy require = std::min(chargeRate(), buffer-energy);
		Energy transfer = network->consume(en->spec, require);
		energy = std::min(buffer, energy+transfer);
	}
}

Energy Charger::consume(Energy e) {
	Energy c = energy < e ? energy: e;
	energy = std::max(Energy(0), energy-c);
	return c;
}

float Charger::level() {
	return energy.portion(buffer);
}

