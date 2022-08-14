#include "common.h"
#include "venter.h"

// Venters are rate-limited fluid sinks.

void Venter::reset() {
	all.clear();
}

void Venter::tick() {
	for (auto& venter: all) {
		venter.update();
	}
}

Venter& Venter::create(uint id) {
	ensure(!all.has(id));
	Venter& venter = all[id];
	venter.id = id;
	venter.en = &Entity::get(id);
	return venter;
}

Venter& Venter::get(uint id) {
	return all.refer(id);
}

void Venter::destroy() {
	all.erase(id);
}

void Venter::update() {
	if (en->isGhost()) return;

	if (!en->isEnabled()) {
		en->state = 0;
		return;
	}

	auto& pipe = en->pipe();

	float speed = en->consumeRate(en->spec->energyConsume * Effector::speed(id));
	auto count = (uint)std::max(1.0f, (float)en->spec->venterRate.value * speed);

	if (!pipe.network || !pipe.network->fid || Liquid(pipe.network->count(pipe.network->fid)) < Mass(count) || speed < 1.0f) {
		en->state = 0;
		return;
	}

	if (++en->state >= en->spec->states.size()) en->state = 0;

	pipe.network->extract({pipe.network->fid,count});
}
