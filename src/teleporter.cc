#include "common.h"
#include "teleporter.h"

// Teleporter components move shipments of items between points.

void Teleporter::reset() {
	all.clear();
}

void Teleporter::tick() {
	for (auto& teleporter: all) teleporter.update();
}

Teleporter& Teleporter::create(uint id) {
	ensure(!all.has(id));
	Teleporter& teleporter = all[id];
	teleporter.id = id;
	teleporter.en = &Entity::get(id);
	teleporter.store = &teleporter.en->store();
	teleporter.trigger = false;
	teleporter.working = false;
	teleporter.progress = 0;
	teleporter.completed = 0;
	teleporter.energyUsed = 0;
	teleporter.shipment.clear();
	return teleporter;
}

Teleporter& Teleporter::get(uint id) {
	return all.refer(id);
}

void Teleporter::destroy() {
	all.erase(id);
}

void Teleporter::update() {
	if (en->isGhost()) return;

	if (working) {
		energyUsed += en->consume(en->spec->energyConsume);
		progress = energyUsed.portion(en->spec->teleporterEnergyCycle);
		if (++en->state >= en->spec->states.size()) en->state = 0;
	}

	if (en->spec->teleporterSend && working && energyUsed >= en->spec->teleporterEnergyCycle) {
		for (auto& stack: shipment) store->remove(stack);
		shipment.clear();
		trigger = false;
		working = false;
		completed++;
		progress = 0;
		energyUsed = 0;
		partner = 0;
	}

	if (en->spec->teleporterRecv && working && energyUsed >= en->spec->teleporterEnergyCycle) {
		for (auto& stack: shipment) store->insert(stack);
		shipment.clear();
		trigger = false;
		working = false;
		completed++;
		progress = 0;
		energyUsed = 0;
		partner = 0;
	}

	if (en->spec->teleporterSend && !working && (store->isFull() || trigger)) {
		for (auto& teleporter: all) {
			if (!teleporter.en->spec->teleporterRecv) continue;
			if (!teleporter.store->isEmpty()) continue;
			if (teleporter.working) continue;
			bool candidate = true;
			for (auto& stack: store->stacks) {
				candidate = candidate && teleporter.store->isAccepting(stack.iid);
			}
			if (candidate) {
				shipment = store->stacks;
				trigger = false;
				working = true;
				energyUsed = 0;
				progress = 0;
				partner = teleporter.id;
				teleporter.shipment = store->stacks;
				teleporter.trigger = false;
				teleporter.working = true;
				teleporter.energyUsed = 0;
				teleporter.progress = 0;
				teleporter.partner = id;
				break;
			}
		}
	}
}

