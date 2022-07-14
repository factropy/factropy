#include "common.h"
#include "entity.h"
#include "networker.h"
#include "hashset.h"

// Networker components connect hubs and leaves into Wifi networks

void Networker::reset() {
	all.clear();
	allHubs.clear();
	gridRanges.clear();
	while (networks.size()) {
		delete networks.back();
		networks.pop_back();
	}
}

void Networker::tick() {
	if (rebuild) {
		rebuild = false;

		while (networks.size()) {
			delete networks.back();
			networks.pop_back();
		}

		for (auto& networker: all) {
			for (auto& interface: networker.interfaces) {
				interface.network = nullptr;
			}
		}

		for (uint id: allHubs) {
			get(id).connect();
		}

		for (auto network: networks) {
			for (auto id: network->nodes) {
				auto& en = Entity::get(id);
				if (en.spec->store && en.spec->logistic) {
					network->logisticStores.push_back(id);
				}
			}
		}
	}

	// nodes may span multiple networks, so clear() once in advance
	for (auto network: networks) {
		network->signals.clear();
	}
	// iterating by network skips everything with networking capability that
	// isn't in use, which is probably a lot (eg most arms and containers)
	for (auto network: networks) {
		for (auto nid: network->nodes) {
			get(nid).publish();
		}
	}
}

Networker& Networker::create(uint id) {
	ensure(!all.has(id));
	Networker& networker = all[id];
	networker.id = id;
	networker.managed = false;
	networker.en = &Entity::get(id);
	networker.interfaces.resize(networker.en->spec->networkInterfaces);
	if (networker.isHub()) allHubs.insert(id);
	return networker;
}

Networker& Networker::get(uint id) {
	return all.refer(id);
}

void Networker::destroy() {
	ensure(!managed);
	allHubs.erase(id);
	all.erase(id);
}

NetworkerSettings* Networker::settings() {
	return new NetworkerSettings(*this);
}

NetworkerSettings::NetworkerSettings(Networker& networker) {
	for (uint i = 0; i < networker.interfaces.size(); i++) {
		ssids.push_back(networker.interfaces[i].ssid);
	}
}

void Networker::setup(NetworkerSettings* settings) {
	for (uint i = 0, l = std::min(interfaces.size(), settings->ssids.size()); i < l; i++) {
		interfaces[i].ssid = settings->ssids[i];
	}
	rebuild = true;
}

void Networker::manage() {
	ensure(!managed);
	managed = true;

	for (auto& interface: interfaces) {
		rebuild = rebuild || !interface.ssid.empty();
	}

	Box box = aerial().box().grow(range());
	gridRanges.insert(box, this);
}

void Networker::unmanage() {
	ensure(managed);
	managed = false;

	for (auto& interface: interfaces) {
		rebuild = rebuild || !interface.ssid.empty();
	}

	Box box = aerial().box().grow(range());
	gridRanges.remove(box, this);
}

void Networker::connect() {
	if (!isHub()) return;
	if (!managed) return;

	for (auto& interface: interfaces) {
		if (interface.network) continue;
		if (interface.ssid.empty()) continue;

		interface.network = new Network();
		interface.network->ssid = interface.ssid;
		interface.network->hubs.insert(id);
		interface.network->nodes.insert(id);
		networks.push_back(interface.network);

		hashset<Networker*> flooded;
		hashset<Networker*> flooding;

		flooding.insert(this);

		while (flooding.size()) {
			Networker* hub = *flooding.begin();
			flooding.erase(hub);

			ensure(hub->isHub());

			if (flooded.has(hub)) continue;

			for (auto& hi: hub->interfaces) {
				if (hi.ssid != interface.ssid) continue;

				flooded.insert(hub);

				Sphere coverage = hub->aerial().sphere().grow(hub->range());
				for (Networker* sib: gridRanges.search(coverage)) {
					if (hub == sib) continue;
					if (!sib->isHub()) continue;
					if (flooding.has(sib) || flooded.has(sib)) continue;

					float dist = hub->aerial().distance(sib->aerial());
					if (dist > hub->range() && dist > sib->range()) continue;

					flooding.insert(sib);
				}
				break;
			}
		}

		hashset<Networker*> hubs;
		hubs.insert(this);

		for (Networker* hub: flooded) {
			if (hub == this) continue;
			for (auto& hi: hub->interfaces) {
				if (hi.ssid != interface.ssid) continue;
				hi.network = interface.network;
				interface.network->hubs.insert(hub->id);
				interface.network->nodes.insert(hub->id);
				hubs.insert(hub);
				break;
			}
		}

		for (Networker* hub: hubs) {
			Sphere coverage = hub->aerial().sphere().grow(hub->range());
			for (Networker* sib: gridRanges.search(coverage)) {
				if (sib->isHub()) continue;

				float dist = hub->aerial().distance(sib->aerial());
				if (dist > hub->range()) continue;

				for (auto& si: sib->interfaces) {
					if (si.ssid != interface.ssid) continue;
					if (si.network) continue;
					si.network = interface.network;
					interface.network->nodes.insert(sib->id);
					break;
				}
			}
		}
	}
}

bool Networker::isHub() {
	return en->spec->networkHub;
}

Point Networker::aerial() {
	return en->spec->networkWifi.transform(en->dir().rotation()) + en->pos();
}

float Networker::range() {
	return std::max(en->spec->networkRange, 1.0f);
}

void Networker::publish() {
	if (!managed) return;

	for (auto& interface: interfaces) {
		if (interface.network) {
			for (auto& signal: interface.signals) {
				if (signal.key.type == Signal::Type::Special) continue;
				interface.network->signals[signal.key].value += signal.value;
			}
		}
		interface.signals.clear();
	}
}

Networker::Interface& Networker::input() {
	return interfaces[0];
}

Networker::Interface& Networker::output() {
	return interfaces[0];
}

std::vector<std::string> Networker::scan() {
	std::set<std::string> ssids;
	for (auto& other: all) {
		if (!other.isHub()) continue;
		float dist = other.aerial().distance(aerial());
		for (auto& interface: other.interfaces) {
			if (dist < other.range() || dist < range())
				ssids.insert(interface.ssid);
		}
	}
	return {ssids.begin(), ssids.end()};
}

std::vector<std::string> Networker::ssids() {
	std::set<std::string> ssids;
	for (auto& other: all) {
		if (!other.isHub()) continue;
		for (auto& interface: other.interfaces) {
			ssids.insert(interface.ssid);
		}
	}
	return {ssids.begin(), ssids.end()};
}

bool Networker::used() {
	for (auto& interface: interfaces) {
		if (interface.ssid != "") return true;
	}
	return false;
}

void Networker::Interface::write(const Signal& signal) {
	if (signal.key.type == Signal::Type::Special) return;
	signals[signal.key].value += signal.value;
}

void Networker::Interface::write(const Stack& stack) {
	write(Signal(stack));
}

void Networker::Interface::write(const Amount& amount) {
	write(Signal(amount));
}

std::vector<uint> Networker::links() {
	std::set<uint> ids;
	auto here = aerial();
	if (isHub()) {
		for (auto& interface: interfaces) {
			if (!interface.network) continue;
			for (auto& nid: interface.network->nodes) {
				if (nid == id) continue;
				auto& node = get(nid);
				auto there = node.aerial();
				auto dist = here.distance(there);
				if (dist > range() && dist > node.range()) continue;
				ids.insert(nid);
			}
		}
	}
	else {
		for (auto& interface: interfaces) {
			if (!interface.network) continue;
			for (auto& nid: interface.network->hubs) {
				if (nid == id) continue;
				auto& hub = get(nid);
				auto there = hub.aerial();
				auto dist = here.distance(there);
				if (dist > hub.range()) continue;
				ids.insert(nid);
			}
		}
	}
	return {ids.begin(), ids.end()};
}