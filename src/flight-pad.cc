#include "common.h"
#include "flight-pad.h"

void FlightPad::reset() {
	all.clear();
}

FlightPad& FlightPad::create(uint id) {
	ensure(!all.has(id));
	FlightPad& pad = all[id];
	pad.id = id;
	pad.claimId = 0;
	pad.en = &Entity::get(id);
	pad.store = pad.en->spec->store ? &pad.en->store(): nullptr;
	return pad;
}

FlightPad& FlightPad::get(uint id) {
	return all.refer(id);
}

void FlightPad::destroy() {
	all.erase(id);
}

bool FlightPad::reserve(uint cid) {
	if (claimId != cid && reserved()) return false;
	claimId = cid;
	return true;
}

void FlightPad::release(uint cid) {
	ensure(claimId == cid);
	claimId = 0;
}

bool FlightPad::reserved(uint cid) {
	if (cid) return claimId == cid && Entity::exists(claimId);
	return claimId && Entity::exists(claimId);
}

Point FlightPad::home() {
	return en->pos() + en->spec->flightPadHome.transform(en->dir().rotation());
}
