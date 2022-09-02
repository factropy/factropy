#include "monorail.h"

// Monorail components are towers connected by Curves and coloured routes,
// navigated by Monorail cars

namespace {
	struct PathForward : Route<Monorail*> {
		std::vector<Monorail*> getNeighbours(Monorail* tower) {
			localset<Monorail*> sibs;
			for (uint i = 0; i < 3; i++) {
				if (tower->out[i]) sibs.insert(&Monorail::get(tower->out[i]));
			}
			return {sibs.begin(), sibs.end()};
		}

		double calcCost(Monorail* towerA, Monorail* towerB) {
			return towerA->en->pos().distance(towerB->en->pos());
		}

		double calcHeuristic(Monorail* tower) {
			return tower->en->pos().distance(target->en->pos());
		}

		bool rayCast(Monorail* towerA, Monorail* towerB) {
			return false;
		}
	};

	struct PathEither : Route<Monorail*> {
		std::vector<Monorail*> getNeighbours(Monorail* tower) {
			localset<Monorail*> sibs;
			for (uint i = 0; i < 3; i++) {
				if (tower->out[i]) sibs.insert(&Monorail::get(tower->out[i]));
			}
			for (auto id: tower->in) {
				if (id) sibs.insert(&Monorail::get(id));
			}
			return {sibs.begin(), sibs.end()};
		}

		double calcCost(Monorail* towerA, Monorail* towerB) {
			return towerA->en->pos().distance(towerB->en->pos());
		}

		double calcHeuristic(Monorail* tower) {
			return tower->en->pos().distance(target->en->pos());
		}

		bool rayCast(Monorail* towerA, Monorail* towerB) {
			return false;
		}
	};
}

void Monorail::reset() {
	all.clear();
}

void Monorail::tick() {
	for (auto& monorail: all) monorail.update();

//	for (auto& monorail: all) {
//		uint id = monorail.id;
//		for (int i = 0; i < 3; i++) {
//			uint oid = monorail.out[i];
//			if (!oid) continue;
//			ensuref(all.has(oid), "%u links out to stale %u", id, oid);
//			ensuref(get(oid).in.has(id), "%u links to unknowing %u", id, oid);
//		}
//		for (auto oid: monorail.in) {
//			ensuref(all.has(oid), "%u links in from stale %u", id, oid);
//		}
//	}
}

Monorail& Monorail::create(uint id) {
	ensure(!all.has(id));
	Monorail& monorail = all[id];
	monorail.id = id;
	monorail.en = &Entity::get(id);
	monorail.filling = true;
	monorail.emptying = true;
	monorail.transmit.contents = false;
	monorail.out[0] = 0;
	monorail.out[1] = 0;
	monorail.out[2] = 0;
	return monorail;
}

Monorail& Monorail::get(uint id) {
	return all.refer(id);
}

void Monorail::destroy() {
	for (int i = 0; i < 3; i++) {
		if (!out[i]) continue;
		disconnectOut(i, out[i]);
	}
	all.erase(id);
}

MonorailSettings* Monorail::settings() {
	return new MonorailSettings(*this);
}

MonorailSettings::MonorailSettings(Monorail& monorail) {
	redirections = monorail.redirections;
	filling = monorail.filling;
	emptying = monorail.emptying;
	transmit.contents = monorail.transmit.contents;
	dir = monorail.en->dir();

	out[Monorail::Red] = monorail.out[Monorail::Red] && Monorail::all.has(monorail.out[Monorail::Red])
		? Monorail::get(monorail.out[Monorail::Red]).en->pos() - monorail.en->pos() : Point::Zero;

	out[Monorail::Blue] = monorail.out[Monorail::Blue] && Monorail::all.has(monorail.out[Monorail::Blue])
		? Monorail::get(monorail.out[Monorail::Blue]).en->pos() - monorail.en->pos() : Point::Zero;

	out[Monorail::Green] = monorail.out[Monorail::Green] && Monorail::all.has(monorail.out[Monorail::Green])
		? Monorail::get(monorail.out[Monorail::Green]).en->pos() - monorail.en->pos() : Point::Zero;
}

void Monorail::setup(MonorailSettings* settings) {
	redirections = settings->redirections;
	filling = settings->filling;
	emptying = settings->emptying;
	transmit.contents = settings->transmit.contents;

	auto rotA = settings->dir.rotation();
	auto rotB = en->dir().rotation();

	auto rot = [&](Point p) {
		if (p == Point::Zero) return p;
		return p.transform(rotA.invert()).transform(rotB);
	};

	if (settings->out[Monorail::Red] != Point::Zero) {
		auto et = Entity::at(rot(settings->out[Monorail::Red]) + en->pos());
		//if (et && et->spec->monorail) et->monorail().connectIn(Monorail::Red, id);
		if (et && et->spec->monorail) connectOut(Monorail::Red, et->id);
	}

	if (settings->out[Monorail::Blue] != Point::Zero) {
		auto et = Entity::at(rot(settings->out[Monorail::Blue]) + en->pos());
		//if (et && et->spec->monorail) et->monorail().connectIn(Monorail::Blue, id);
		if (et && et->spec->monorail) connectOut(Monorail::Blue, et->id);
	}

	if (settings->out[Monorail::Green] != Point::Zero) {
		auto et = Entity::at(rot(settings->out[Monorail::Green]) + en->pos());
		//if (et && et->spec->monorail) et->monorail().connectIn(Monorail::Green, id);
		if (et && et->spec->monorail) connectOut(Monorail::Green, et->id);
	}
}

Point Monorail::arrive() {
	return en->pos() + en->spec->monorailArrive.transform(en->dir().rotation());
}

Point Monorail::depart() {
	return en->pos() + en->spec->monorailDepart.transform(en->dir().rotation());
}

Point Monorail::origin() {
	return en->pos() + (Point::Up*(en->spec->collision.h/2.0));
}

bool Monorail::claim(uint cid) {
	if (!claimer || claimer == cid || claimed < Sim::tick) {
		claimer = cid;
		claimed = Sim::tick+10;
		return true;
	}
	return false;
}

bool Monorail::occupied() {
	return claimer && claimed > Sim::tick;
}

Rail Monorail::railTo(Monorail* sib) {
	ensuref(sib != this && sib->en->pos() != en->pos(), "cannot rail to self");
	return en->spec->railTo(en->pos(), en->dir(), sib->en->spec, sib->en->pos(), sib->en->dir());
}

std::vector<Monorail::RailLine> Monorail::railsOut() {
	std::vector<Monorail::RailLine> rails;
	for (int i = 0; i < 3; i++) {
		if (!out[i]) continue;
		if (!all.has(out[i])) continue;
		auto& sib = get(out[i]);
		rails.push_back({.rail = railTo(&sib), .line = i});
//		ensure(en->spec->railOk(rails.back().rail));
	}
	return rails;
}

Point Monorail::positionUnload() {
	return en->spec->monorailStopUnload.transform(en->dir().rotation()) + en->pos();
}

Point Monorail::positionEmpty() {
	return en->spec->monorailStopEmpty.transform(en->dir().rotation()) + en->pos();
}

Point Monorail::positionFill() {
	return en->spec->monorailStopFill.transform(en->dir().rotation()) + en->pos();
}

Point Monorail::positionLoad() {
	return en->spec->monorailStopLoad.transform(en->dir().rotation()) + en->pos();
}

Entity* Monorail::containerAt(Point pos) {
	for (auto ec: Entity::intersecting(pos.box().grow(0.1))) {
		if (ec->spec->monorailContainer) return ec;
	}
	return nullptr;
}

bool Monorail::canUnload() {
	if (!en->spec->monorailStop) return false;
	return !containers.size() || containers.front().state > Container::Unloaded;
}

void Monorail::unload(uint cid) {
	containerAdd(Container::Unloaded, cid);
}

bool Monorail::canLoad() {
	if (!en->spec->monorailStop) return false;
	return !containers.size() || containers.front().state > Container::Unloaded;
}

uint Monorail::load() {
	return containers.size() && containers.back().state == Container::Loaded && !containers.back().en->isGhost() ? containers.pop().cid: 0;
}

bool Monorail::containerAdd(Monorail::Container::State s, uint cid) {
	if (!en->spec->monorailStop) return false;
	for (auto& container: containers) if (container.state == s) return false;
	for (auto& container: containers) if (container.cid == cid) return false;
	containers.push({.state = s, .cid = cid, .en = &Entity::get(cid), .store = &Store::get(cid)});
	std::sort(containers.begin(), containers.end(), [](auto a, auto b) { return a.state < b.state; });
	return true;
}

void Monorail::update() {
	minivec<uint> drop;
	for (auto oid: in) if (!all.has(oid)) drop.push(oid);
	for (auto oid: drop) in.erase(oid);

	for (int i = 0; i < 3; i++) {
		uint oid = out[i];
		if (!oid) continue;
		auto other = all.point(oid);
		if (!other) { oid = 0; continue; }
		other->in.insert(id);
	}

	if (en->isGhost()) return;
	en->consume(en->spec->energyConsume);

	if (!en->spec->monorailStop) {
		ensure(!containers.size());
		return;
	}

	for (auto it = containers.begin(); it != containers.end(); ) {
		auto& container = *it;
		if (!Entity::exists(container.cid) || container.en->pos().distance(en->pos()) > 10) {
			it = containers.erase(it);
			continue;
		}
		++it;
	}

	if (en->spec->networker && transmit.contents) {
		auto& networker = en->networker();
		if (networker.interfaces.front().network) {
			for (auto& container: containers) {
				for (auto& stack: container.store->stacks)
					networker.interfaces.front().write(stack);
			}
		}
	}

	for (int i = 0, l = containers.size(); i < l; i++) {
		auto container = &containers[i];
		auto ahead = i < l-1 ? &containers[i+1]: nullptr;

		auto clear = [&](Container::State s) {
			return !ahead || ahead->state > s;
		};

		auto advance = [&](Point target, Container::State next) {
			auto pos = container->en->pos();
			if (pos.distance(target) > 0.02) {
				Point dir = (target - pos).normalize();
				container->en->bump(pos + (dir * 0.02), en->dir());
			}
			else {
				container->en->bump(target, en->dir());
				container->state = next;
			}
		};

		switch (container->state) {
			case Container::Unloaded: {
				if (clear(Container::Empty))
					container->state = Container::Descending;
				break;
			}
			case Container::Descending: {
				if (!clear(Container::Empty)) return;
				advance(positionEmpty(), Container::Empty);
				break;
			}
			case Container::Empty: {
				if ((!emptying || container->store->isEmpty()) && clear(Container::Fill))
					container->state = Container::Shunting;
				break;
			}
			case Container::Shunting: {
				if (!clear(Container::Fill)) return;
				advance(positionFill(), Container::Fill);
				break;
			}
			case Container::Fill: {
				if ((!filling || container->store->isFull()) && clear(Container::Loaded))
					container->state = Container::Ascending;
				break;
			}
			case Container::Ascending: {
				advance(positionLoad(), Container::Loaded);
				break;
			}
			case Container::Loaded: {
				break;
			}
		}
	}

	auto containerWithState = [&](Container::State s) {
		for (auto& container: containers) {
			if (container.state == s) return true;
		}
		return false;
	};

	if (containerAt(positionEmpty()) && !containerWithState(Container::Descending) && !containerWithState(Container::Empty)) {
		containerAdd(Container::Empty, containerAt(positionEmpty())->id);
	}

	if (containerAt(positionFill()) && !containerWithState(Container::Shunting) && !containerWithState(Container::Fill)) {
		containerAdd(Container::Fill, containerAt(positionFill())->id);
	}
}

// outgoing connection
bool Monorail::connectOut(int line, uint oid) {
	if (out[line])
		disconnectOut(line, out[line]);

	if (oid != id && all.has(oid)) {
		auto& other = get(oid);
		Rail rail = en->spec->railTo(en->pos(), en->dir(), other.en->spec, other.en->pos(), other.en->dir());
		if (en->spec->railOk(rail)) {
			out[line] = oid;
			other.in.insert(id);
			return true;
		}
	}
	return false;
}

// outgoing disconnection
bool Monorail::disconnectOut(int line, uint oid) {
	if (!oid) oid = out[line];

	if (out[line] == oid) {
		out[line] = 0;
		if (all.has(oid)) {
			auto& other = get(oid);
			bool duplicates = false;
			for (int i = 0; i < 3 && !duplicates; i++)
				if (out[i] == oid) duplicates = true;
			if (!duplicates) other.in.erase(id);
		}
		return true;
	}
	return false;
}

int Monorail::redirect(int line, minimap<Signal,&Signal::key> signals) {
	for (auto& redirection: redirections) {
		if (redirection.condition.evaluate(signals)) return redirection.line;
	}
	return line;
}

Monorail* Monorail::next(int *line, minimap<Signal,&Signal::key> signals) {
	*line = redirect(*line, signals);
	return out[*line] && all.has(out[*line]) ? &get(out[*line]): nullptr;
}

