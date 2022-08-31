#include "common.h"
#include "pipe.h"

// Pipe components transport fluid. They automatically link together with adjacent
// pipes to form a network holding a single fluid type.

// Fluid modelling is fairly simplistic so far; the network tracks the fluid level
// as a percentage of $totalNetworkCapacity and each Pipe just assumes it has
// ($localCapacity * $levelPercentage) fluid available.

void Pipe::reset() {
	all.clear();
	PipeNetwork::reset();
}

void Pipe::tick() {
	PipeNetwork::tick();

	for (auto id: transmitters) {
		auto& pipe = get(id);
		ensure(pipe.transmit);
		ensure(pipe.managed);
		ensure(pipe.network);
		auto& en = Entity::get(id);
		if (!en.isGhost() && en.spec->networker) {
			auto& networker = en.networker();
			Amount amount = {pipe.network->fid, (uint)(en.spec->pipeCapacity.value * pipe.network->level())};
			networker.output().write(amount);
		}
	}
}

Pipe& Pipe::create(uint id) {
	Entity& en = Entity::get(id);
	Pipe& pipe = all[id];
	pipe.id = id;
	pipe.network = NULL;
	pipe.cacheFid = 0;
	pipe.cacheTally = 0;
	pipe.partner = 0;
	pipe.underground = en.spec->pipeUnderground;
	pipe.underwater = en.spec->pipeUnderwater;
	pipe.valve = en.spec->pipeValve;
	pipe.managed = false;
	pipe.transmit = false;
	pipe.filter = 0;
	pipe.overflow = 0.5f;
	pipe.src = 0;
	pipe.dst = 0;
	return pipe;
}

Pipe& Pipe::get(uint id) {
	return all.refer(id);
}

void Pipe::destroy() {
	all.erase(id);
}

PipeSettings* Pipe::settings() {
	if (!valve) return nullptr;
	return new PipeSettings(*this);
}

PipeSettings::PipeSettings(Pipe& pipe) {
	overflow = pipe.overflow;
	filter = pipe.filter;
}

void Pipe::setup(PipeSettings* settings) {
	overflow = settings->overflow;
	filter = settings->filter;
}

void Pipe::manage() {
	ensure(!managed);
	ensure(!connections.size());
	ensure(!partner);

	managed = true;

	localvec<uint> affected;

	for (auto a: pipeConnections()) {
		Box ab = a.box().grow(0.1f);
		for (auto ep: Entity::intersecting(ab, Entity::gridPipes)) {
			connect(ep->id, affected);
		}
	}

	if (valve) {
		ensure(!connections.size());
	}

	for (auto ep: Entity::intersecting(Entity::get(id).box().grow(1.0f), Entity::gridPipes)) {
		get(ep->id).valveLinks();
	}

	findPartner(affected);

	for (uint aid: affected) {
		changed.insert(aid);
		for (auto pid: get(aid).connections) {
			changed.insert(pid);
		}
	}

	changed.insert(id);
	for (auto pid: connections) {
		changed.insert(pid);
	}

	if (transmit) {
		transmitters.insert(id);
	}
}

void Pipe::unmanage() {
	ensure(managed);

	if (network) {
		network->pipes.erase(id);
		network = NULL;
	}

	managed = false;

	localvec<uint> affected;

	changed.insert(id);
	for (auto pid: connections) {
		changed.insert(pid);
	}

	while (connections.size()) {
		ensure(disconnect(connections.front(), affected));
	}

	for (uint aid: affected) {
		changed.insert(aid);
		for (auto pid: get(aid).connections) {
			changed.insert(pid);
		}
	}

	for (auto ep: Entity::intersecting(Entity::get(id).box().grow(1.0f), Entity::gridPipes)) {
		get(ep->id).valveLinks();
	}

	ensure(!partner);

	transmitters.erase(id);
}

void Pipe::findPartner(localvec<uint>& affected) {
	if (underground) {
		for (auto ep: Entity::intersecting(undergroundRange(), Entity::gridPipes)) {
			connect(ep->id, affected);
		}
	}
	if (underwater) {
		for (auto ep: Entity::intersecting(underwaterRange(), Entity::gridPipes)) {
			connect(ep->id, affected);
		}
	}
}

bool Pipe::connect(uint pid, localvec<uint>& affected) {
	if (pid == id) return false;

	auto& other = get(pid);
	if (!managed) return false;
	if (valve || other.valve) return false;
	if (!other.managed) return false;

	if (connections.has(other.id) || other.connections.has(id)) {
		ensure(connections.has(other.id));
		ensure(other.connections.has(id));
		return true;
	}

	for (auto a: pipeConnections()) {
		Box ab = a.box().grow(0.1f);
		for (Point b: other.pipeConnections()) {
			Box bb = b.box().grow(0.1f);
			if (ab.intersects(bb)) {
				connections.insert(other.id);
				other.connections.insert(id);
				return true;
			}
		}
	}

	if ((underground && other.underground) || (underwater && other.underwater)) {

		bool aligned = Entity::get(id).dir() == Entity::get(other.id).dir().oppositeCardinal();
		bool inRangeUnderground = underground && undergroundRange().intersects(other.undergroundRange());
		bool inRangeUnderwater = underwater && underwaterRange().intersects(other.underwaterRange());

		if (aligned && (inRangeUnderground || inRangeUnderwater)) {

			if (!partner && !other.partner) {
				partner = other.id;
				other.partner = id;
				connections.insert(other.id);
				other.connections.insert(id);
				return true;
			}

			if (!partner && other.partner) {
				float distanceHere = Entity::get(id).pos().distance(Entity::get(other.id).pos());
				float distanceThere = Entity::get(other.partner).pos().distance(Entity::get(other.id).pos());
				if (distanceHere < distanceThere) {
					ensure(get(other.partner).disconnect(other.id, affected));
					partner = other.id;
					other.partner = id;
					connections.insert(other.id);
					other.connections.insert(id);
					return true;
				}
			}

			if (partner && !other.partner) {
				float distanceHere = Entity::get(id).pos().distance(Entity::get(other.id).pos());
				float distanceThere = Entity::get(id).pos().distance(Entity::get(partner).pos());
				if (distanceHere < distanceThere) {
					ensure(get(partner).disconnect(id, affected));
					partner = other.id;
					other.partner = id;
					connections.insert(other.id);
					other.connections.insert(id);
					return true;
				}
			}
		}
	}

	return false;
}

bool Pipe::disconnect(uint pid, localvec<uint>& affected) {
	auto& other = get(pid);
	bool ok = false;

	affected.push_back(id);
	affected.push_back(other.id);

	if (connections.has(other.id) || other.connections.has(id)) {
		ensure(connections.has(other.id));
		ensure(other.connections.has(id));
		connections.erase(other.id);
		other.connections.erase(id);
		ok = true;
	}

	if (partner == other.id) {
		ensure(other.partner == id);
		partner = 0;
		other.partner = 0;
	}

	if (ok) {
		affected.push_back(id);
		affected.push_back(other.id);
	}

	return ok;
}

void Pipe::valveLinks() {
	src = 0;
	dst = 0;

	if (!valve) return;
	if (!managed) return;

	auto links = pipeConnections();

	auto check = [&](Point p) {
		Box box = p.box().grow(0.1f);
		for (auto sen: Entity::intersecting(box, Entity::gridPipes)) {
			if (sen->id == id) continue;
			if (sen->isGhost()) continue;
			if (!sen->spec->pipe) continue;

			Pipe& other = Pipe::get(sen->id);
			if (!other.managed) continue;

			for (Point op: other.pipeConnections()) {
				if (box.intersects(op.box().grow(0.1f))) {
					return sen->id;
				}
			}
		}
		return 0u;
	};

	if (links.size() > 0) {
		src = check(links[0]);
	}

	if (links.size() > 1) {
		dst = check(links[1]);
	}
}

localvec<Point> Pipe::pipeConnections() {
	Entity& en = Entity::get(id);
	return en.spec->relativePoints(en.spec->pipeConnections, en.dir().rotation(), en.pos());
}

localvec<uint> Pipe::servicing(Box box, const localvec<Entity*>& candidates) {
	localvec<uint> hits;
	for (auto en: candidates) {
		if (!en->spec->pipe) continue;
		Pipe& pipe = en->pipe();
		// only pipes adjacent to the box, pointing inward
		if (en->box().intersects(box.shrink(0.1f))) continue;

		for (Point point: pipe.pipeConnections()) {
			if (box.intersects(point.box().grow(0.1f))) {
				hits.push_back(pipe.id);
			}
		}
	}
	deduplicate(hits);
	return hits;
}

localvec<uint> Pipe::servicing(Box box) {
	return servicing(box, Entity::intersecting(box.grow(0.5f), Entity::gridPipes));
}

localvec<uint> Pipe::siblings() {
	ensure(managed);

	if (valve) return {id};

	std::set<uint> members = {id};
	std::list<uint> flood = {id};

	while (!flood.empty()) {
		uint pid = flood.front();
		flood.pop_front();
		for (auto cid: get(pid).connections) {
			if (members.count(cid)) continue;
			Pipe& con = get(cid);
			if (con.valve) continue;
			members.insert(cid);
			flood.push_back(cid);
		}
	}

	return {members.begin(), members.end()};
}

void PipeNetwork::reset() {
	while (all.size()) {
		delete *(all.begin());
	}
}

void PipeNetwork::tick() {
	if (Pipe::changed.size()) {
		hashset<uint> affected;

		// When one or more pipes have changed (placed, rotated, removed)
		// flood-fill from that position to find the affected networks, and
		// rebuild only as required.

		for (uint pid: Pipe::changed) {
			if (!Pipe::all.has(pid)) continue;
			if (affected.has(pid)) continue;
			auto& pipe = Pipe::get(pid);
			if (!pipe.managed) continue;
			for (auto pid: pipe.siblings()) {
				affected.insert(pid);
			}
		}

		Pipe::changed.clear();

		for (auto pid: affected) {
			auto& pipe = Pipe::get(pid);
			ensure(pipe.managed);
			if (pipe.network) delete pipe.network;
			ensure(!pipe.network);
		}

		localvec<PipeNetwork*> networks;

		for (auto pid: affected) {
			auto& pipe = Pipe::get(pid);
			if (pipe.network) continue;
			PipeNetwork* network = new PipeNetwork();
			networks.push_back(network);
			for (auto& pid: pipe.siblings()) {
				auto& pipe = Pipe::get(pid);
				ensure(!pipe.network);
				pipe.network = network;
				network->pipes.insert(pid);
			}
		}

		for (auto network: networks) {
			for (uint id: network->pipes) {
				Pipe& pipe = Pipe::get(id);
				if (pipe.cacheFid) {
					network->fid = pipe.cacheFid;
					//network->tally = pipe.cacheTally;
					break;
				}
			}
		}

		for (auto network: networks) {
			for (uint id: network->pipes) {
				Entity& en = Entity::get(id);
				Pipe& pipe = Pipe::get(id);
				network->limit += en.spec->pipeCapacity;
				if (pipe.cacheFid == network->fid) {
					network->tally += pipe.cacheTally;
				}
				pipe.cacheFid = network->fid;
				pipe.cacheTally = 0;
			}
			if (!network->tally) {
				network->fid = 0;
			}
			if (network->tally > network->limit.value) {
				network->tally = network->limit.value;
			}
		}

		// When the last pipe in a network becomes unmanaged, clean up.
		localvec<PipeNetwork*> drop;
		for (auto network: all) {
			if (!network->pipes.size())
				drop.push_back(network);
		}
		for (auto network: drop) {
			delete network;
		}
	}

	for (auto& network: all) {
		if (network->valve()) network->first().valveTick();
	}
}

void Pipe::valveTick() {
	Entity& en = Entity::get(id);
	ensure(network);

	if (filter != network->fid) {
		network->flush();
		network->fid = filter;
	}

	if (!filter) return;

	if (src) {
		Pipe& spipe = get(src);
		if (spipe.network->fid == filter) {
			float difference = spipe.network->level() - network->level();
			if (difference > 0.0f) {
				Amount transfer = {filter, (uint)std::ceil(difference * (float)en.spec->pipeCapacity.value)};
				network->inject(spipe.network->extract(transfer));
			}
		}
	}

	if (dst) {
		Pipe& dpipe = get(dst);
		if (dpipe.network->fid == filter || !dpipe.network->fid) {
			float difference = network->level() - overflow;
			if (difference > 0.0f) {
				Amount potential = {filter, (uint)std::ceil(difference * (float)en.spec->pipeCapacity.value)};
				Amount excess = dpipe.network->inject(potential);
				Amount actual = {filter,potential.size-excess.size};
				network->extract(actual);
			}
		}
	}
}

Amount Pipe::contents() {
	if (network && network->fid) {
		Entity& en = Entity::get(id);
		uint fid = network->fid;
		float fill = network->level();
		uint n = std::ceil((float)en.spec->pipeCapacity.fluids(fid) * fill);
		return {fid, n};
	}
	return {0,0};
}

Box Pipe::undergroundRange() {
	Entity& en = Entity::get(id);
	Point away = (en.dir() * en.spec->pipeUndergroundRange) + en.pos();
	return Box(en.pos(), away).grow(0.1f);
}

Box Pipe::underwaterRange() {
	Entity& en = Entity::get(id);
	Point away = (en.dir() * en.spec->pipeUnderwaterRange) + en.pos();
	return Box(en.pos(), away).grow(0.1f);
}

void Pipe::flush() {
	if (network) {
		network->flush();
	}
}

PipeNetwork::PipeNetwork() {
	all.insert(this);
	fid = 0;
	limit = 0;
	tally = 0;
}

PipeNetwork::~PipeNetwork() {
	cacheState();
	for (uint id: pipes) {
		Pipe& pipe = Pipe::get(id);
		pipe.network = NULL;
	}
	pipes.clear();
	all.erase(this);
}

void PipeNetwork::cacheState() {
	float fill = level();
	for (uint id: pipes) {
		Entity& en = Entity::get(id);
		Pipe& pipe = Pipe::get(id);
		pipe.cacheFid = fid;
		pipe.cacheTally = fid ? (uint)std::ceil((float)en.spec->pipeCapacity.fluids(fid) * fill): 0;
	}
}

Amount PipeNetwork::inject(Amount amount) {
	if (valve() && first().filter != amount.fid) {
		return amount;
	}
	if (!fid) {
		fid = amount.fid;
	}
	if (amount.fid == fid) {
		uint count = std::min(space(fid), amount.size);
		amount.size -= count;
		tally += count;
	}
	return amount;
}

Amount PipeNetwork::extract(Amount amount) {
	if (amount.fid == fid) {
		uint count = std::min(tally, (int)amount.size);
		amount.size = count;
		tally -= count;
		return amount;
	}
	return {0,0};
}

uint PipeNetwork::count(uint ffid) {
	return fid == ffid ? tally: 0;
}

uint PipeNetwork::space(uint ffid) {
	if (!fid) return limit;
	return fid == ffid ? limit.fluids(fid)-tally: 0;
}

float PipeNetwork::level() {
	return fid ? Liquid(Fluid::get(fid)->liquid.value * tally).portion(limit): 0.0f;
}

void PipeNetwork::flush() {
	fid = 0;
	tally = 0;
}

bool PipeNetwork::valve() {
	return first().valve;
}

Pipe& PipeNetwork::first() {
	return Pipe::get(*(pipes.begin()));
}
