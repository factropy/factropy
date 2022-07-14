#include "common.h"
#include "flight-logistic.h"
#include <set>

// Logistic flights

void FlightLogistic::reset() {
	all.clear();
}

void FlightLogistic::tick() {
	for (auto& logi: all) logi.update();
}

FlightLogistic& FlightLogistic::create(uint id) {
	ensure(!all.has(id));
	FlightLogistic& logi = all[id];
	logi.id = id;
	logi.dst = 0;
	logi.src = 0;
	logi.dep = 0;
	logi.stage = Home;
	logi.en = &Entity::get(id);
	logi.flight = &logi.en->flightPath();
	logi.store = &logi.en->store();
	return logi;
}

FlightLogistic& FlightLogistic::get(uint id) {
	return all.refer(id);
}

void FlightLogistic::destroy() {
	all.erase(id);
}

void FlightLogistic::releaseAllExcept(uint pid) {
	for (auto& pad: FlightPad::all) {
		if (pad.id == pid) continue;
		if (pad.claimId == id) pad.release(id);
	}
}

void FlightLogistic::updateHome() {
	ensure(!src);
	ensure(!dst);

	if (!dep || (dep && (!Entity::exists(dep) || !FlightPad::get(dep).reserved(id)))) {
		dep = 0;
		stage = ToDep;
		return;
	}

	ensure(dep && FlightPad::get(dep).reserved(id));

	if (!store->isEmpty()) {
		src = dep;
		dep = 0;
		stage = ToDst;
	}

	for (auto& send: FlightPad::all) {
		if (send.en->isGhost()) continue;
		if (!send.en->spec->flightPadSend) continue;
		if (send.store->usage() < store->limit()) continue;
		if (!send.reserve(id)) continue;
		src = send.id;
		stage = ToSrc;
	}
}

void FlightLogistic::updateToSrc() {
	ensure(src);
	ensure(!dst);
	ensure(dep);

	if (!Entity::exists(src)) {
		src = 0;
		stage = ToDep;
		return;
	}

	auto& spad = FlightPad::get(src);
	if (!spad.reserve(id)) return;

	flight->depart(spad.home() + Point(0,en->spec->collision.h/2,0), spad.en->dir());
	stage = ToSrcDeparting;
}

void FlightLogistic::updateToSrcDeparting() {
	ensure(src);
	ensure(!dst);
	ensure(dep);

	if (!Entity::exists(src)) {
		releaseAllExcept(0);
		flight->cancel();
		src = 0;
		stage = ToDep;
		return;
	}

	if (!flight->flying()) return;

	stage = ToSrcFlight;
	releaseAllExcept(src);
	dep = 0;
}

void FlightLogistic::updateToSrcFlight() {
	ensure(src);
	ensure(!dst);

	if (!Entity::exists(src)) {
		releaseAllExcept(0);
		flight->cancel();
		src = 0;
		stage = ToDep;
		return;
	}

	if (!flight->done()) return;

	stage = Loading;
}

void FlightLogistic::updateLoading() {
	ensure(src);
	ensure(!dep);
	ensure(!dst);

	if (!Entity::exists(src)) {
		src = 0;
		stage = ToDep;
		return;
	}

	auto& lstore = Store::get(id);
	auto& sstore = Store::get(src);

	for (auto stack: sstore.stacks) {
		stack.size = std::min(stack.size, en->spec->flightLogisticRate);
		auto remain = lstore.insert(stack);
		sstore.remove({stack.iid, stack.size-remain.size});
		break;
	}

	if (!lstore.isFull()) return;

	stage = ToDst;
}

void FlightLogistic::updateToDst() {
	ensure(src);
	ensure(!dst);
	ensure(!dep);

	if (store->isEmpty()) {
		stage = ToDep;
		return;
	}

	for (auto& recv: FlightPad::all) {
		if (recv.en->isGhost()) continue;
		if (!recv.en->spec->flightPadRecv) continue;
		bool candidate = false;
		for (auto& stack: store->stacks) {
			if (recv.store->isAccepting(stack.iid) && recv.store->countAcceptable(stack.iid) >= stack.size) {
				candidate = true;
				break;
			}
		}
		if (candidate && recv.reserve(id)) {
			flight->depart(recv.home() + Point(0,en->spec->collision.h/2,0), recv.en->dir());
			dst = recv.id;
			stage = ToDstDeparting;
			return;
		}
	}
}

void FlightLogistic::updateToDstDeparting() {
	ensure(src);
	ensure(dst);
	ensure(!dep);

	if (!Entity::exists(dst)) {
		releaseAllExcept(0);
		flight->cancel();
		dst = 0;
		stage = ToDep;
		return;
	}

	if (!flight->flying()) return;

	stage = ToDstFlight;
	releaseAllExcept(dst);
	src = 0;
}

void FlightLogistic::updateToDstFlight() {
	ensure(!src);
	ensure(dst);
	ensure(!dep);

	if (!Entity::exists(dst)) {
		releaseAllExcept(0);
		flight->cancel();
		dst = 0;
		stage = ToDep;
		return;
	}

	if (!flight->done()) return;
	stage = Unloading;
}

void FlightLogistic::updateUnloading() {
	ensure(!src);
	ensure(dst);
	ensure(!dep);

	if (!Entity::exists(dst)) {
		dst = 0;
		stage = ToDep;
		return;
	}

	auto& lstore = Store::get(id);
	auto& dstore = Store::get(dst);

	bool activity = false;
	for (auto stack: lstore.stacks) {
		if (!dstore.isAccepting(stack.iid)) continue;
		stack.size = std::min(stack.size, en->spec->flightLogisticRate);
		stack.size = std::min(stack.size, dstore.countAcceptable(stack.iid));
		auto remain = dstore.insert(stack);
		lstore.remove({stack.iid, stack.size-remain.size});
		activity = true;
		break;
	}

	if (lstore.isEmpty()) {
		dst = 0;
		stage = ToDep;
		return;
	}

	if (!activity) {
		src = dst;
		dst = 0;
		stage = ToDst;
		return;
	}
}

void FlightLogistic::updateToDep() {
	if (dep && Entity::exists(dep) && FlightPad::get(dep).reserved(id)) {
		FlightPad::get(dep).release(id);
	}
	dep = 0;
	for (auto& pad: FlightPad::all) {
		if (pad.en->isGhost()) continue;
		if (pad.en->spec->flightPadDepot && pad.reserve(id)) {
			dep = pad.id;
			flight->depart(pad.home() + Point(0,en->spec->collision.h/2,0), pad.en->dir());
			stage = ToDepDeparting;
			return;
		}
	}
}

void FlightLogistic::updateToDepDeparting() {
	ensure(dep);

	if (!Entity::exists(dep)) {
		releaseAllExcept(0);
		flight->cancel();
		dep = 0;
		stage = ToDep;
		return;
	}

	if (!flight->flying()) return;

	stage = ToDepFlight;
	releaseAllExcept(dep);
	dst = 0;
}

void FlightLogistic::updateToDepFlight() {
	ensure(dep);

	if (!Entity::exists(dep)) {
		releaseAllExcept(0);
		flight->cancel();
		dep = 0;
		stage = ToDep;
		return;
	}

	if (!flight->done()) return;
	stage = Home;
}

void FlightLogistic::update() {
	if (en->isGhost()) return;

	switch (stage) {
		case Home: {
			updateHome();
			break;
		}
		case ToSrc: {
			updateToSrc();
			break;
		}
		case ToSrcDeparting: {
			updateToSrcDeparting();
			break;
		}
		case ToSrcFlight: {
			updateToSrcFlight();
			break;
		}
		case Loading: {
			updateLoading();
			break;
		}
		case ToDst: {
			updateToDst();
			break;
		}
		case ToDstDeparting: {
			updateToDstDeparting();
			break;
		}
		case ToDstFlight: {
			updateToDstFlight();
			break;
		}
		case Unloading: {
			updateUnloading();
			break;
		}
		case ToDep: {
			updateToDep();
			break;
		}
		case ToDepDeparting: {
			updateToDepDeparting();
			break;
		}
		case ToDepFlight: {
			updateToDepFlight();
			break;
		}
	}
}
