#include "common.h"
#include "entity.h"
#include "powerpole.h"

// PowerPole components create an electricity grid

void PowerPole::reset() {
	all.clear();
}

void PowerPole::tick() {
}

PowerPole& PowerPole::create(uint id) {
	ensure(!all.has(id));
	PowerPole& pole = all[id];
	pole.id = id;
	pole.en = &Entity::get(id);
	pole.managed = false;
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
	connect();
}

void PowerPole::unmanage() {
	ensure(managed);
	managed = false;
	disconnect();
}

Cylinder PowerPole::range() {
	return Cylinder(point(), en->spec->powerpoleRange+0.001, 1000);
}

Box PowerPole::coverage() {
	return en->pos().box().grow(en->spec->powerpoleCoverage);
}

minivec<uint> PowerPole::siblings() {
	minivec<uint> out;
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
	return en->pos() + Point::Up*(en->spec->collision.h/2.0);
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
