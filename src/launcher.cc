#include "common.h"
#include "launcher.h"

// Launcher components consume item shipments and fuel. Similar to a Crafter
// but without results and with a specific set of animation states.

void Launcher::reset() {
	all.clear();
}

void Launcher::tick() {
	for (auto& launcher: all) launcher.update();
}

Launcher& Launcher::create(uint id) {
	ensure(!all.has(id));
	Launcher& launcher = all[id];
	launcher.id = id;
	launcher.en = &Entity::get(id);
	launcher.store = &launcher.en->store();
	launcher.working = false;
	launcher.activate = false;
	launcher.progress = 0.0f;
	launcher.completed = 0;
	launcher.monitor = Monitor::Network;
	launcher.en->state = launcher.en->spec->launcherInitialState;
	return launcher;
}

Launcher& Launcher::get(uint id) {
	return all.refer(id);
}

void Launcher::destroy() {
	all.erase(id);
}

minivec<uint> Launcher::pipes() {
	minivec<uint> pipes;
	if (en->spec->pipe) pipes.push_back(id);

	auto candidates = Entity::intersecting(en->box().grow(1.0f), Entity::gridPipes);
	discard_if(candidates, [](Entity* ec) { return ec->isGhost(); });

	auto connections = en->spec->relativePoints(
		en->spec->pipeInputConnections,
		en->dir().rotation(), en->pos()
	);

	for (auto point: connections) {
		for (auto pid: Pipe::servicing(point.box(), candidates)) {
			pipes.push_back(pid);
		}
	}

	deduplicate(pipes);
	return pipes;
}

minimap<Amount,&Amount::fid> Launcher::fuelRequired() {
	minimap<Amount,&Amount::fid> require;
	for (auto amount: en->spec->launcherFuel)
		require[amount.fid].size = amount.size;
	return require;
}

minimap<Amount,&Amount::fid> Launcher::fuelAccessable() {
	auto plist = pipes();

	minimap<Amount,&Amount::fid> accessable;
	for (auto amount: en->spec->launcherFuel) {
		auto fid = amount.fid;
		if (!fid) continue;
		accessable[fid].size = 0;
		for (uint pid: plist) {
			Entity& pe = Entity::get(pid);
			if (pe.isGhost()) continue;
			Pipe& pipe = Pipe::get(pid);
			if (!pipe.network) continue;
			accessable[fid].size += pipe.network->count(fid);
		}
	}

	return accessable;
}

bool Launcher::fueled() {
	auto fuelFound = fuelAccessable();
	bool fluidsReady = true;

	for (auto amount: en->spec->launcherFuel) {
		if (fuelFound[amount.fid].size < amount.size) fluidsReady = false;
	}

	return fluidsReady;
}

bool Launcher::ready() {
	return fueled() && cargo.size();
}

// check network
bool Launcher::checkCondition() {
	bool rule = condition.valid();
//	en.setRuled(rule);

	bool permit = true;

//	if (rule && monitor == Monitor::Store) {
//		permit = condition.evaluate(store->signals());
//	}

	if (rule && en->spec->networker && monitor == Monitor::Network) {
		auto network = en->networker().input().network;
		permit = condition.evaluate(network ? network->signals: Signal::NoSignals);
	}

//	en.setBlocked(!permit);
	return permit;
}

void Launcher::update() {
	if (en->isGhost()) return;

	store->levels.clear();
	for (auto& stack: store->stacks) store->levelSet(stack.iid, 0, 0);
	for (auto& iid: cargo) store->levelSet(iid, en->spec->capacity.items(iid), en->spec->capacity.items(iid));

	// initial landing
	if (!working && en->state) {
		if (en->state) en->state++;
		if (en->state == en->spec->states.size()) en->state = 0;
	}

	if (working) {
		en->state++;
		if (en->state == en->spec->states.size()) {
			working = false;
			progress = 0.0f;
			en->state = 0;
			completed++;
			return;
		}
	}

	if (!working && ready()) {
		if (!activate) {
			activate = condition.valid() ? checkCondition(): store->isFull();
		}
		if (activate) {
			auto fuelPipes = pipes();
			for (auto amount: en->spec->launcherFuel) {
				for (uint pid: fuelPipes) {
					Entity& pe = Entity::get(pid);
					if (pe.isGhost()) continue;
					Pipe& pipe = Pipe::get(pid);
					if (!pipe.network) continue;
					amount.size -= pipe.network->extract(amount).size;
					if (!amount.size) continue;
				}
			}
			for (auto& iid: cargo) {
				if (!store->count(iid)) continue;
				auto stack = store->remove({iid,store->count(iid)});
				Item::get(iid)->consume(stack.size);
				Item::get(iid)->supply(stack.size);
			}
			working = true;
			en->state = 0;
			progress = 0.0f;
			activate = false;
		}
	}

	en->setBlocked(!working);
	progress = (float)en->state / (float)en->spec->states.size();
}

