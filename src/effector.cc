#include "common.h"
#include "effector.h"

// Effector components transmit effects to adjacent entities, such as
// overall speed (energy consumption) and recharge rates

void Effector::reset() {
	active.clear();
	all.clear();
}

void Effector::tick() {
	active.tick();
	for (auto effector: active) {
		effector->update();
	}
//	for (auto spec: specs) {
//		Entity::bulkConsumeElectricity(spec, spec->effectorEnergyDrain, spec->count.extant);
//	}
}

float Effector::effect(uint eid, enum Type type) {
	float factor = 1.0;
	if (assist.count(eid)) {
		auto espec = Entity::get(eid).spec;
		minivec<uint> remove;
		auto& ids = assist[eid];
		for (auto bid: ids) {
			if (!all.has(bid)) {
				remove.push_back(bid);
				continue;
			}
			auto bspec = all[bid].en->spec;
			switch (type) {
				case SPEED:
					if (espec->consumeElectricity) factor += bspec->effectorElectricity;
					if (espec->consumeFuel) factor += bspec->effectorFuel;
					break;
				case CHARGE:
					if (espec->consumeCharge) factor += bspec->effectorCharge;
					break;
			}
		}
		for (auto bid: remove) {
			ids.erase(bid);
		}
	}
	return factor;
}

float Effector::speed(uint eid) {
	return effect(eid, SPEED);
}

Effector& Effector::create(uint id) {
	ensure(!all.has(id));
	Effector& effector = all[id];
	effector.id = id;
	effector.en = &Entity::get(id);
	effector.tid = 0;
	active.insert(&effector);
	specs.insert(effector.en->spec);
	return effector;
}

Effector& Effector::get(uint id) {
	return all.refer(id);
}

void Effector::destroy() {
	active.erase(this);
	all.erase(id);
}

Point Effector::output() {
	return en->pos() + en->dir();
}

void Effector::update() {
	if (en->isGhost()) {
		active.pause(this);
		return;
	}

	bool reset = false;

	if (tid && !reset && !Entity::exists(tid)) {
		reset = true;
	}

	if (tid && !reset && !Entity::get(tid).box().contains(output())) {
		reset = true;
	}

	if (reset) {
		assist[tid].erase(id);
		if (!assist[tid].size())
			assist.erase(tid);
		tid = 0;
	}

	if (!tid) {
		auto et = Entity::at(output());
		if (et && !et->spec->effector) {
			tid = et->id;
			assist[tid].insert(id);
		}
	}

	active.pause(this);
}

