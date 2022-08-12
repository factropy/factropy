#include "common.h"
#include "arm.h"
#include "sim.h"

// Arm components move items between Stores and Conveyors.

std::size_t Arm::memory() {
	std::size_t size = all.memory();
	for (auto& arm: all) size += arm.filter.memory();
	return size;
}

void Arm::reset() {
	all.clear();
}

void Arm::tick() {
	for (auto& arm: all) {
		arm.update();
	}
}

Arm& Arm::create(uint id) {
	ensure(!all.has(id));
	Arm& arm = all[id];
	arm.id = id;
	arm.en = &Entity::get(id);
	arm.iid = 0;
	arm.inputId = 0;
	arm.inputStoreId= 0;
	arm.outputId = 0;
	arm.outputStoreId = 0;
	arm.monitor = Monitor::InputStore;
	arm.stage = Input;
	arm.orientation = 0.0f;
	arm.speed = 0.0f;
	arm.pause = 0;
	arm.inputNear = true;
	arm.inputFar = true;
	arm.outputNear = true;
	arm.outputFar = true;
	return arm;
}

Arm& Arm::get(uint id) {
	return all.refer(id);
}

void Arm::destroy() {
	all.erase(id);
}

ArmSettings* Arm::settings() {
	return new ArmSettings(*this);
}

ArmSettings::ArmSettings(Arm& arm) {
	filter = arm.filter;
	monitor = arm.monitor;
	condition = arm.condition;
	inputNear = arm.inputNear;
	inputFar = arm.inputFar;
	outputNear = arm.outputNear;
	outputFar = arm.outputFar;
}

void Arm::setup(ArmSettings* settings) {
	filter = settings->filter;
	monitor = settings->monitor;
	condition = settings->condition;
	inputNear = settings->inputNear;
	inputFar = settings->inputFar;
	outputNear = settings->outputNear;
	outputFar = settings->outputFar;
}

// absolute input location
Point Arm::input() {
	return en->spec->armInput.transform(en->dir().rotation()) + en->pos();
}

// absolute output location
Point Arm::output() {
	return en->spec->armOutput.transform(en->dir().rotation()) + en->pos();
}

// Arms check their source and destination entities periodically. For static entities, which
// is most of them, this is cheap. For vehicles this behaviour should be profiled at scale
void Arm::updateProximity() {
	auto ei = Entity::find(inputId);
	auto eo = Entity::find(outputId);

	if (!ei || (ei && !ei->box().contains(input()))) {
		inputId = 0;
		ei = nullptr;
	}

	if (!eo || (eo && !eo->box().contains(output()))) {
		outputId = 0;
		eo = nullptr;
	}

	auto at = [&](const Point& p) {
		auto stores = Entity::intersecting(p.box().grow(0.1), Entity::gridStores);
		if (stores.size()) return stores.front();
		auto fuelStores = Entity::intersecting(p.box().grow(0.1), Entity::gridStoresFuel);
		if (fuelStores.size()) return fuelStores.front();
		auto conveyors = Entity::intersecting(p.box().grow(0.1), Entity::gridConveyors);
		if (conveyors.size()) return conveyors.front();
		Entity* ep = nullptr;
		return ep;
	};

	if (!inputId) {
		ei = at(input());
		inputId = ei ? ei->id: 0;
	}

	if (!outputId) {
		eo = at(output());
		outputId = eo ? eo->id: 0;
	}
}

Stack Arm::transferStoreToStore(Store& dst, Store& src) {
	Entity& de = *dst.en; //Entity::get(dst.id);
	Entity& se = *src.en; //Entity::get(src.id);

	if (de.isGhost() || se.isGhost()) return {0,0};

	// crafter to crafter, strict respect of levels
	if (dst.strict() && src.strict()) {
		for (auto& sl: src.levels) {
			if (filter.size() && !filter.count(sl.iid)) continue;
			uint have = src.countProviding(sl.iid, &sl);
			for (auto& dl: dst.levels) {
				if (dl.iid != sl.iid) continue;
				uint need = dst.countRequesting(dl.iid, &dl);
				if (have && need) return {sl.iid,1};
			}
		}
		return {0,0};
	}

	// container to crafter, strict respect of dst levels
	if (dst.strict() && !src.strict()) {
		for (auto& ss: src.stacks) {
			if (filter.size() && !filter.count(ss.iid)) continue;
			uint have = ss.size;
			for (auto& dl: dst.levels) {
				if (dl.iid != ss.iid) continue;
				uint need = dst.countRequesting(dl.iid, &dl);
				if (have && need) return {ss.iid,1};
			}
		}
		return {0,0};
	}

	// crafter to container, strict respect of src levels
	if (!dst.strict() && src.strict()) {
		for (auto& sl: src.levels) {
			if (filter.size() && !filter.count(sl.iid)) continue;
			uint have = src.countProviding(sl.iid, &sl);
			auto dl = dst.level(sl.iid);
			uint space = dl ? dst.countAccepting(dl->iid, dl): dst.countSpace(sl.iid);
			if (have && space) return {sl.iid,1};
		}
		return {0,0};
	}

	// container to container, just move stuff
	for (auto& ss: src.stacks) {
		if (filter.size() && !filter.count(ss.iid)) continue;
		auto sl = src.level(ss.iid);
		uint have = sl ? src.countProviding(sl->iid, sl): src.countNet(ss.iid);
		auto dl = dst.level(ss.iid);
		uint space = dl ? dst.countAccepting(dl->iid, dl): dst.countSpace(ss.iid);
		if (have && space) return {ss.iid,1};
	}

	return {0,0};
}

Stack Arm::transferStoreToBelt(Store& src) {
	Entity& se = *src.en; //Entity::get(src.id);

	if (se.isGhost()) return {0,0};

	if (src.strict()) {
		for (auto& sl: src.levels) {
			if (filter.size() && !filter.count(sl.iid)) continue;
			if (src.countProviding(sl.iid, &sl)) return {sl.iid,1};
		}
		return {0,0};
	}

	for (auto& ss: src.stacks) {
		if (filter.size() && !filter.count(ss.iid)) continue;
		auto sl = src.level(ss.iid);
		uint have = sl ? src.countProviding(sl->iid, sl): src.countNet(ss.iid);
		if (have) return {ss.iid,1};
	}

	return {0,0};
}

Stack Arm::transferBeltToStore(Store& dst, Stack stack) {
	Entity& de = *dst.en; //Entity::get(dst.id);

	if (de.isGhost()) return {0,0};

	if (filter.size() && !filter.count(stack.iid)) return {0,0};

	if (dst.strict()) {
		if (dst.countAccepting(stack.iid)) return {stack.iid,1};
		return {0,0};
	}

	auto dl = dst.level(stack.iid);
	uint have = dl ? dst.countAccepting(dl->iid, dl): dst.countSpace(stack.iid);
	if (have) return {stack.iid,1};

	return {0,0};
}

// while parked, check if an item is ready for transfer
bool Arm::updateReady() {

	if (inputId && outputId) {

		Entity& ei = Entity::get(inputId);
		Entity& eo = Entity::get(outputId);

		for (Store* si: ei.stores()) {
			for (Store* so: eo.stores()) {
				Stack stack = transferStoreToStore(*so, *si);
				if (stack.iid && stack.size) {
					return true;
				}
			}
		}

		if (eo.spec->conveyor && (outputNear || outputFar)) {
			for (Store* si: ei.stores()) {
				Stack stack = transferStoreToBelt(*si);
				if (stack.iid && stack.size) {
					return true;
				}
			}
		}

		if (ei.spec->conveyor && !ei.isGhost() && (inputNear || inputFar)) {
			for (auto ciid: ei.conveyor().items()) {
				for (Store* so: eo.stores()) {
					Stack stack = transferBeltToStore(*so, {ciid,1});
					if (stack.iid && stack.size) {
						return true;
					}
				}
			}
		}

		if (ei.spec->conveyor && eo.spec->conveyor && !ei.isGhost() && !eo.isGhost()
			&& (inputNear || inputFar) && (outputNear || outputFar)
		){
			for (auto ciid: ei.conveyor().items()) {
				if (filter.size() && !filter.has(ciid)) continue;
				return true;
			}
		}
	}

	return false;
}

// when about to pick up, check if item and source entity still exist
bool Arm::updateInput() {

	if (inputId && outputId) {

		Entity& ei = Entity::get(inputId);
		Entity& eo = Entity::get(outputId);
		inputStoreId = 0;
		outputStoreId = 0;

		for (Store* si: ei.stores()) {
			for (Store* so: eo.stores()) {
				Stack stack = transferStoreToStore(*so, *si);
				if (stack.iid && stack.size) {
					outputStoreId = so->sid;
					so->promise({stack.iid,1});
					so->arms.insert(id);
					si->remove({stack.iid,1});
					iid = stack.iid;
					stage = ToOutput;
					return true;
				}
			}
		}

		if (eo.spec->conveyor && (outputNear || outputFar)) {
			for (Store* si: ei.stores()) {
				Stack stack = transferStoreToBelt(*si);
				if (stack.iid && stack.size) {
					si->remove({stack.iid,1});
					iid = stack.iid;
					stage = ToOutput;
					return true;
				}
			}
		}

		// conveyor to store
		if (ei.spec->conveyor && !ei.isGhost() && (inputNear || inputFar)) {
			for (auto ciid: ei.conveyor().items()) {
				for (Store* so: eo.stores()) {
					Stack stack = transferBeltToStore(*so, {ciid,1});
					if (stack.iid && stack.size) {
						outputStoreId = so->sid;
						so->promise({ciid,1});
						so->arms.insert(id);

						bool ok = false;

						ok = ok || (inputNear && inputFar && ei.conveyor().remove(ciid));
						ok = ok || (inputNear && !inputFar && ei.conveyor().removeNear(en->pos(), ciid));
						ok = ok || (!inputNear && inputFar && ei.conveyor().removeFar(en->pos(), ciid));

						if (ok) {
							iid = ciid;
							stage = ToOutput;
							return true;
						}
					}
				}
			}
		}

		// conveyor to conveyor
		if (ei.spec->conveyor && eo.spec->conveyor && !ei.isGhost() && !eo.isGhost()
			&& (inputNear || inputFar) && (outputNear || outputFar)
		){
			for (auto ciid: ei.conveyor().items()) {
				if (filter.size() && !filter.has(ciid)) continue;

				bool ok = false;

				ok = ok || (inputNear && inputFar && ei.conveyor().remove(ciid));
				ok = ok || (inputNear && !inputFar && ei.conveyor().removeNear(en->pos(), ciid));
				ok = ok || (!inputNear && inputFar && ei.conveyor().removeFar(en->pos(), ciid));

				if (ok) {
					iid = ciid;
					stage = ToOutput;
					return true;
				}
			}
		}
	}

	return false;
}

// when about to put down, check if destination entity still exists and has space
bool Arm::updateOutput() {

	if (outputId) {
		Entity& eo = Entity::get(outputId);

		if (eo.spec->store || eo.spec->consumeFuel) {
			for (Store* so: eo.stores()) {
				if (so->sid == outputStoreId) {
					if (so->insert({iid,1}).size == 0) {
						so->arms.erase(id);
						break;
					}
				}
			}
			outputStoreId = 0;
			iid = 0;
			stage = ToInput;
			return true;
		}

		if (eo.spec->conveyor && !eo.isGhost()) {
			auto& conveyor = eo.conveyor();

			if (outputNear && outputFar && (conveyor.insertNear(en->pos(), iid) || conveyor.insertFar(en->pos(), iid))) {
				iid = 0;
				stage = ToInput;
				return true;
			}

			if (outputNear && !outputFar && conveyor.insertNear(en->pos(), iid)) {
				iid = 0;
				stage = ToInput;
				return true;
			}

			if (!outputNear && outputFar && conveyor.insertFar(en->pos(), iid)) {
				iid = 0;
				stage = ToInput;
				return true;
			}
		}
	}

	return false;
}

// check network
bool Arm::checkCondition() {
	bool rule = condition.valid();
	en->setRuled(rule);

	bool permit = true;

	if (rule && monitor == Monitor::InputStore && inputId) {
		auto store = Store::all.point(inputId);
		permit = store && condition.evaluate(store->signals());
	}

	if (rule && monitor == Monitor::OutputStore && outputId) {
		auto store = Store::all.point(outputId);
		permit = store && condition.evaluate(store->signals());
	}

	if (rule && monitor == Monitor::Network && en->spec->networker) {
		auto network = en->networker().input().network;
		permit = condition.evaluate(network ? network->signals: Signal::NoSignals);
	}

	en->setBlocked(!permit);
	return permit;
}

// Expected spec states:
// 0-359: rotation
// 360-?: parking

void Arm::update() {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;
	if (pause > Sim::tick) return;

	uint maxState = en->spec->states.size()-1;
	ensure(maxState > 360);

	switch (stage) {
		case Parked: {
			updateProximity();
			en->state = maxState;
			if (checkCondition() && updateReady()) {
				stage = Unparking;
			} else {
				pause = Sim::tick+10;
			}
			break;
		}

		case Parking: {
			if (en->state >= maxState) {
				en->state = maxState;
				stage = Parked;
			} else {
				en->state++;
			}
			break;
		}

		case Unparking: {
			if (en->state <= 360) {
				en->state = 0;
				stage = Input;
			} else {
				en->state--;
			}
			break;
		}

		case Input: {
			updateProximity();
			if (!checkCondition() || !updateInput()) {
				stage = Parking;
				en->state = 360;
			}
			break;
		}

		case ToInput: {
			speed = std::max(en->consumeRate(en->spec->energyConsume) * (en->spec->armSpeed * speedFactor), 0.001f);

			orientation = std::min(1.0f, orientation+speed);
			if (std::abs(orientation-1.0f) < en->spec->armSpeed) {
				orientation = 0.0f;
				stage = Input;
			}
			en->state = (uint)std::floor(orientation*360.f);
			break;
		}

		case Output: {
			updateProximity();
			if (!updateOutput()) {
				// short delay means finding smaller belt gaps
				pause = Sim::tick+5;
			}
			break;
		}

		case ToOutput: {
			speed = std::max(en->consumeRate(en->spec->energyConsume) * (en->spec->armSpeed * speedFactor), 0.001f);

			orientation = std::min(0.5f, orientation+speed);
			if (std::abs(orientation-0.5f) < en->spec->armSpeed) {
				orientation = 0.5f;
				stage = Output;
			}
			en->state = (uint)std::floor(orientation*360.f);
			break;
		}
	}
}
