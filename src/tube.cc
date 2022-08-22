#include "tube.h"

// Tube components move items between points like elevated single-sided
// conveyor belts that do not need to interact with Arms

std::size_t Tube::memory() {
	std::size_t size = all.memory();
	return size;
}

void Tube::reset() {
	all.clear();
}

void Tube::tick() {
	auto network = ElectricityNetwork::primary();
	for (auto& tube: all) tube.update(network);
}

Tube& Tube::create(uint id) {
	ensure(!all.has(id));
	Tube& tube = all[id];
	tube.id = id;
	tube.en = &Entity::get(id);
	tube.next = 0;
	tube.length = 0;
	tube.input = Mode::BeltOrTube;
	tube.output = Mode::BeltOrTube;
	tube.lastInput = Last::Tube;
	tube.lastOutput = Last::Tube;
	tube.accepted = 0;
	return tube;
}

Tube& Tube::get(uint id) {
	return all.refer(id);
}

void Tube::destroy() {
	all.erase(id);
}

TubeSettings* Tube::settings() {
	return new TubeSettings(*this);
}

TubeSettings::TubeSettings(Tube& tube) {
	target = tube.target() - tube.origin();
	input = tube.input;
	output = tube.output;
}

void Tube::setup(TubeSettings* settings) {
	for (auto et: Entity::intersecting((en->pos() + settings->target).box().grow(0.1))) {
		if (et->spec->tube) {
			et->tube().connect(id);
			break;
		}
	}
	input = settings->input;
	output = settings->output;
}

bool Tube::canOutputTube() {
	if (output == Mode::BeltOnly) return false;
	return next && (!stuff.size() || stuff.back().offset >= 500);
}

void Tube::outputTube(uint iid) {
	ensuref(canOutputTube(), "outputTube invalid call");
	stuff.push_back({.iid = iid, .offset = 0});
	lastOutput = Last::Tube;
}

bool Tube::canOutputBelt() {
	if (output == Mode::TubeOnly) return false;
	if (!en->spec->conveyor) return false;
	auto& conveyor = en->conveyor();
	return conveyor.countFront() < 2;
}

void Tube::outputBelt(uint iid) {
	ensuref(canOutputBelt() && iid, "outputBelt invalid call");
	auto& conveyor = en->conveyor();
	conveyor.insertAnyFront(iid);
	lastOutput = Last::Belt;
}

bool Tube::canInputTube() {
	if (input == Mode::BeltOnly) return false;
	return accepted != 0;
}

uint Tube::inputTube() {
	ensuref(canInputTube(), "inputTube invalid call");
	uint iid = accepted;
	accepted = 0;
	lastInput = Last::Tube;
	return iid;
}

bool Tube::canInputBelt() {
	if (input == Mode::TubeOnly) return false;
	if (!en->spec->conveyor) return false;
	auto& conveyor = en->conveyor();
	if (conveyor.deadEnd()) return !conveyor.empty();
	return conveyor.countBack() > 0;
}

uint Tube::inputBelt() {
	lastInput = Last::Belt;
	auto& conveyor = en->conveyor();
	uint iid = conveyor.removeAnyBack();
	if (!iid && conveyor.deadEnd()) iid = conveyor.removeAny();
	ensuref(iid, "inputBelt invalid call; can't remove from conveyor");
	return iid;
}

bool Tube::isTubeBlocked() {
	return !canOutputTube();
}

bool Tube::isBeltBlocked() {
	auto& conveyor = en->conveyor();
	return conveyor.blockedLeft && conveyor.blockedRight;
}

bool Tube::tryTubeInTubeOut() {
	if (canInputTube() && canOutputTube()) {
		outputTube(inputTube());
		return true;
	}
	return false;
}

bool Tube::tryTubeInBeltOut() {
	if (canInputTube() && canOutputBelt()) {
		outputBelt(inputTube());
		return true;
	}
	return false;
}

bool Tube::tryBeltInBeltOut() {
	if (canInputBelt() && canOutputBelt()) {
		outputBelt(inputBelt());
		return true;
	}
	return false;
}

bool Tube::tryBeltInTubeOut() {
	if (canInputBelt() && canOutputTube()) {
		outputTube(inputBelt());
		return true;
	}
	return false;
}

bool Tube::tryBeltBackedTubeOut() {
	if (isBeltBlocked() && canInputBelt() && canOutputTube()) {
		outputTube(inputBelt());
		return true;
	}
	return false;
}

bool Tube::tryTubeBackedBeltOut() {
	if (isTubeBlocked() && canInputTube() && canOutputBelt()) {
		outputBelt(inputTube());
		return true;
	}
	return false;
}

bool Tube::tryAnyInAnyOut() {
	return (lastInput == Last::Belt)
		? tryTubeInBeltOut() || tryBeltInTubeOut() || tryTubeInTubeOut()
		: tryBeltInTubeOut() || tryTubeInBeltOut() || tryTubeInTubeOut();
//	return tryTubeInTubeOut() || tryBeltInBeltOut() || tryTubeInBeltOut() || tryBeltInTubeOut();
}

bool Tube::tryAnyInBeltOut() {
	return tryTubeInBeltOut();
}

bool Tube::tryBeltInAnyOut() {
	return tryBeltInTubeOut();
}

bool Tube::tryAnyInTubeOut() {
	return tryTubeInTubeOut() || tryBeltInTubeOut();
}

bool Tube::tryTubeInAnyOut() {
	return tryTubeInTubeOut() || tryTubeInBeltOut();
}

void Tube::update(ElectricityNetwork* network) {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;

	// last tower with no link
	if (!next || (next && !all.has(next))) {
		next = 0;
		stuff.clear();
	}

	if (next) {
		auto& sib = all[next];
		length = std::floor(sib.origin().distance(origin())) * 1000.0f;

		Energy require = en->spec->energyConsume * ((float)length/(float)en->spec->tubeSpan);
		Energy energy = network->consume(en->spec, require);

		float fueled = energy.portion(require);
		uint speed = std::ceil(std::max(10.0f, (float)en->spec->tubeSpeed * fueled)); // mm/s

		ensure(length);

		// advance tube items
		if (stuff.size()) {
			auto& thing = stuff.front();
			thing.offset += speed;
			thing.offset = std::min(thing.offset, length);
		}
		for (uint i = 1, l = stuff.size(); i < l; i++) {
			auto& thing = stuff[i];
			auto& ahead = stuff[i-1];
			int limit = (int)ahead.offset - 500;
			int offset = thing.offset + speed;
			thing.offset = std::max(0, std::min(offset, limit));
		}

		bool canUnload = stuff.size() && stuff.front().offset >= length;

		// stuff already in the outgoing tube is unaffected by mode changes
		if (canUnload && sib.accept(stuff.front().iid)) {
			stuff.pop_front();
		}
	}

	if (output == Mode::TubeOnly && en->spec->conveyor) {
		en->conveyor().hold();
	}

	for (;;) {
		if (input == Mode::BeltOrTube) {
			if (output == Mode::BeltOrTube) {
				tryAnyInAnyOut();
			}

			if (output == Mode::BeltOnly) {
				tryAnyInBeltOut();
			}

			if (output == Mode::TubeOnly) {
				tryAnyInTubeOut();
			}

			if (output == Mode::BeltPriority) {
				tryTubeInBeltOut() || tryTubeInTubeOut() || tryBeltBackedTubeOut();
			}

			if (output == Mode::TubePriority) {
				tryTubeInTubeOut() || tryBeltInTubeOut() || tryTubeBackedBeltOut();
			}
		}

		if (input == Mode::BeltOnly) {
			if (output == Mode::BeltOrTube) {
				tryBeltInAnyOut();
			}

			if (output == Mode::BeltOnly) {
				// n/a
			}

			if (output == Mode::TubeOnly) {
				tryBeltInTubeOut();
			}

			if (output == Mode::BeltPriority) {
				tryBeltBackedTubeOut();
			}

			if (output == Mode::TubePriority) {
				tryBeltInTubeOut();
			}
		}

		if (input == Mode::TubeOnly) {
			if (output == Mode::BeltOrTube) {
				tryTubeInAnyOut();
			}

			if (output == Mode::BeltOnly) {
				tryTubeInBeltOut();
			}

			if (output == Mode::TubeOnly) {
				tryTubeInTubeOut();
			}

			if (output == Mode::BeltPriority) {
				tryTubeInBeltOut() || tryTubeInTubeOut();
			}

			if (output == Mode::TubePriority) {
				tryTubeInTubeOut() || tryTubeInBeltOut();
			}
		}

		if (input == Mode::BeltPriority) {
			if (output == Mode::BeltOrTube) {
				tryBeltBackedTubeOut() || tryBeltInTubeOut() || tryTubeInTubeOut() || tryTubeInBeltOut();
			}

			if (output == Mode::BeltOnly) {
				tryTubeInBeltOut();
			}

			if (output == Mode::TubeOnly) {
				tryBeltInTubeOut() || tryTubeInTubeOut();
			}

			if (output == Mode::BeltPriority) {
				tryTubeInBeltOut() || tryBeltBackedTubeOut();
			}

			if (output == Mode::TubePriority) {
				tryBeltInTubeOut() || tryTubeInTubeOut() || tryTubeBackedBeltOut();
			}
		}

		if (input == Mode::TubePriority) {
			if (output == Mode::BeltOrTube) {
				tryTubeInTubeOut() || tryTubeInBeltOut() || tryAnyInAnyOut();
			}

			if (output == Mode::BeltOnly) {
				tryTubeInBeltOut();
			}

			if (output == Mode::TubeOnly) {
				tryTubeInTubeOut() || tryBeltInTubeOut();
			}

			if (output == Mode::BeltPriority) {
				tryTubeInBeltOut() || tryTubeInTubeOut() || tryBeltBackedTubeOut();
			}

			if (output == Mode::TubePriority) {
				tryTubeInTubeOut() || tryBeltInTubeOut() || tryTubeBackedBeltOut();
			}
		}

		break;
	}
}

void Tube::flush() {
	stuff.clear();
}

// incoming connection
bool Tube::connect(uint nid) {
	if (nid != id && all.has(nid)) {
		auto& other = get(nid);
		auto posA = en->pos();
		auto posB = other.en->pos();
		auto dist = posA.distance(posB);
		auto distGround = posA.floor(0).distance(posB.floor(0));
		if (distGround > 0.5 && dist > 0.5 && dist < ((real)other.en->spec->tubeSpan / 1000.0 + 0.01)) {
			other.next = id;
			return true;
		}
	}
	return false;
}

bool Tube::disconnect(uint nid) {
	if (next == nid) {
		next = 0;
		return true;
	}
	return false;
}

Point Tube::origin() {
	return en->pos() + (Point::Up * en->spec->tubeOrigin);
}

Point Tube::target() {
	return next && all.has(next) ? get(next).origin(): origin();
}

bool Tube::accept(uint iid) {
	if (en->isGhost()) return false;
	if (!en->isEnabled()) return false;

	if (input == Mode::BeltOnly) return false;
	if (accepted) return false;

	accepted = iid;
	return true;
}

void Tube::upgrade(uint uid) {
	auto& other = get(uid);
	next = other.next;
	length = other.length;
	accepted = other.accepted;

	for (auto et: Entity::intersecting(en->box().grow((real)en->spec->tubeSpan / 1000.0 + 1), Entity::gridTubes)) {
		auto& tube = et->tube();
		if (tube.next == uid) {
			tube.next = id;
		}
	}
}

std::vector<uint> Tube::upgradableGroup(uint from) {
	if (!from) return {};
	if (!all.has(from)) return {};

	auto& first = get(from);
	if (first.en->isGhost()) return {};

	std::set<uint> ids;
	auto spec = first.en->spec;

	for (uint id = from; !ids.count(id) && all.has(id); id = get(id).next) {
		auto& tube = get(id);
		if (tube.en->isGhost()) break;
		if (!spec->upgradeCascade.count(tube.en->spec)) break;
		ids.insert(id);
	}

	return {ids.begin(), ids.end()};
}


