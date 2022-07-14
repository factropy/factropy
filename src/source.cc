#include "common.h"
#include "source.h"

// Source components generate items or fluids

void Source::reset() {
	all.clear();
}

void Source::tick() {
	for (auto& source: all) source.update();
}

Source& Source::create(uint id) {
	ensure(!all.has(id));
	Source& source = all[id];
	source.id = id;
	source.en = &Entity::get(id);
	source.pipe = source.en->spec->sourceFluid ? &source.en->pipe(): nullptr;
	source.store = source.en->spec->sourceItem ? &source.en->store(): nullptr;
	return source;
}

Source& Source::get(uint id) {
	return all.refer(id);
}

void Source::destroy() {
	all.erase(id);
}

void Source::update() {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;

	if (++en->state >= en->spec->states.size()) en->state = 0;

	en->consumeRate(en->spec->energyConsume * Effector::speed(id));

	if (en->spec->sourceFluid) {
		if (pipe->network) pipe->network->inject({en->spec->sourceFluid->id, en->spec->sourceFluidRate});
	}

	if (en->spec->sourceItem) {
		store->insert({en->spec->sourceItem->id, en->spec->sourceItemRate});
	}
}

