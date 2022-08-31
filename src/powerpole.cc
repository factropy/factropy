#include "common.h"
#include "entity.h"
#include "powerpole.h"
#include "electricity.h"

// PowerPole components create the electricity grid

void PowerPole::reset() {
	all.clear();
}

void PowerPole::tick() {
}

PowerPole* PowerPole::covering(Box box) {
	for (auto pole: gridCoverage.search(box)) {
		if (pole->coverage().intersects(box)) return pole;
	}
	return nullptr;
}

bool PowerPole::covered(Box box) {
	return covering(box) != nullptr;
}

PowerPole& PowerPole::create(uint id) {
	ensure(!all.has(id));
	PowerPole& pole = all[id];
	pole.id = id;
	pole.en = &Entity::get(id);
	pole.managed = false;
	pole.network = nullptr;
	return pole;
}

PowerPole& PowerPole::get(uint id) {
	return all.refer(id);
}

void PowerPole::destroy() {
	ensure(!managed);
	all.erase(id);
}

void PowerPole::manage() {
	ensure(!managed);
	managed = true;
	network = nullptr;
	connect();
	gridCoverage.insert(coverage(), this);
	ElectricityNetwork::rebuild = true;
}

void PowerPole::unmanage() {
	ensure(managed);
	ensure(network);
	managed = false;
	network->poles.erase(id);
	network = nullptr;
	disconnect();
	gridCoverage.remove(coverage(), this);
	ElectricityNetwork::rebuild = true;
}

Cylinder PowerPole::range() {
	return Cylinder(point(), en->spec->powerpoleRange+0.001, 1000);
}

Box PowerPole::coverage() {
	return en->pos().box().grow(en->spec->powerpoleCoverage);
}

localvec<uint> PowerPole::siblings() {
	localvec<uint> out;
	for (auto se: Entity::intersecting(range())) {
		if (se->id == id) continue;
		if (!se->spec->powerpole) continue;
		auto& sibling = PowerPole::get(se->id);
		if (!sibling.range().contains(point())) continue;
		if (!range().contains(sibling.point())) continue;
		out.push_back(se->id);
	}
	return out;
}

Point PowerPole::point() {
	return en->pos() + en->spec->powerpolePoint;
}

void PowerPole::connect() {
	for (auto se: Entity::intersecting(range())) {
		if (se->id == id) continue;
		if (!se->spec->powerpole) continue;
		if (se->isGhost()) continue;
		auto& sibling = get(se->id);
		if (!sibling.range().contains(point())) continue;
		if (!range().contains(sibling.point())) continue;
		links.insert(se->id);
		sibling.links.insert(id);
	}
}

void PowerPole::disconnect() {
	for (auto sid: links) {
		auto& sibling = get(sid);
		sibling.links.erase(id);
	}
	links.clear();
}
