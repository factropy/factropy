#include "common.h"
#include "entity.h"
#include "charger.h"

// Charger components buffer electricity for local or grid consumption.

void Charger::tickCharge() {
	for (auto& charger: all) {
		if (!charger.en->spec->bufferElectricity) charger.chargePrimary();
	}

	for (auto& charger: all) {
		if (charger.en->spec->bufferElectricityState && charger.en->spec->states.size()) {
			float speed = charger.level() * 50.0f;
			uint steps = (uint)std::round(speed);
			charger.en->state += std::max(1u, steps);
			if (charger.en->state >= charger.en->spec->states.size()) {
				charger.en->state = charger.en->state - charger.en->spec->states.size();
			}
		}
	}

	if (Entity::electricity.demand >= Entity::electricity.supply) return;
	Energy excess = Entity::electricity.capacityReady - Entity::electricity.demand;

	Energy predict = 0;
	for (auto& charger: all) {
		predict += charger.chargeSecondaryPredict();
	}

	// -1% to sit just under generator capacity
	float rate = excess.portion(predict) * 0.99f;

	for (auto& charger: all) {
		charger.chargeSecondary(rate);
	}
}

void Charger::tickDischarge() {
	Energy predict = 0;
	for (auto& charger: all) {
		predict += charger.dischargeSecondaryPredict();
	}

	// +2% to allow generators (which lag load slightly) to adjust before kick in
	if (Entity::electricity.demand <= (Entity::electricity.supply*1.02f)) return;
	Energy shortfall = Entity::electricity.demand - Entity::electricity.supply;

	float rate = shortfall.portion(predict);

	for (auto& charger: all) {
		charger.dischargeSecondary(rate);
	}
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

Energy Charger::chargePrimaryRate() {
	float effect = en->spec->consumeChargeEffect ? Effector::effect(en->id, Effector::CHARGE): 1.0f;
	return en->spec->consumeChargeRate * effect;
}

void Charger::chargePrimary() {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;
	if (en->spec->bufferElectricity) return;

	if (energy < buffer) {
		Energy rate = chargePrimaryRate();
		Energy require = std::min(rate, buffer-energy);
		Energy transfer = require * Entity::electricity.satisfaction;
		Entity::electricity.demand += require;
		energy = std::min(buffer, energy+transfer);
		en->spec->statsGroup->energyConsumption.add(Sim::tick, transfer);
	}
}

Energy Charger::chargeSecondaryPredict() {
	if (en->isGhost()) return 0;
	if (!en->isEnabled()) return 0;
	if (!en->spec->bufferElectricity) return 0;
	return std::min(buffer-energy, en->spec->consumeChargeRate);
}

void Charger::chargeSecondary(float rate) {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;
	if (!en->spec->bufferElectricity) return;

	if (energy < buffer) {
		Energy require = std::min(en->spec->consumeChargeRate * rate, buffer-energy);
		Energy transfer = require * Entity::electricity.satisfaction;
		Entity::electricity.demand += require;
		energy = std::min(buffer, energy+transfer);
		en->spec->statsGroup->energyConsumption.add(Sim::tick, transfer);
	}
}

Energy Charger::dischargeSecondaryPredict() {
	if (en->isGhost()) return 0;
	if (!en->isEnabled()) return 0;
	if (!en->spec->bufferElectricity) return 0;

	auto rate = std::min(energy, en->spec->bufferDischargeRate);
	Entity::electricity.bufferedLevel += energy;
	Entity::electricity.bufferedLimit += buffer;
	Entity::electricity.capacityBufferedReady += rate;
	return rate;
}

void Charger::dischargeSecondary(float rate) {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;

	Energy supplied = consume(en->spec->bufferDischargeRate * rate);
	en->spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
	Entity::electricity.supply += supplied;
}

Energy Charger::consume(Energy e) {
	Energy c = energy < e ? energy: e;
	energy = std::max(Energy(0), energy-c);
	return c;
}

float Charger::level() {
	return energy.portion(buffer);
}

