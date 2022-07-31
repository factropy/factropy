#include "common.h"
#include "entity.h"
#include "powerpole.h"

// PowerPole components create the electricity grid

namespace {
	struct Trace : Route<PowerPole*> {
		std::vector<PowerPole*> getNeighbours(PowerPole* a) {
			std::vector<PowerPole*> hits;
			for (auto sid: a->links) {
				auto sib = PowerPole::all.point(sid);
				if (!sib) continue;
				hits.push_back(sib);
			}
			return hits;
		}

		double calcCost(PowerPole* a, PowerPole* b) {
			return a->en->pos().distance(b->en->pos());
		}

		double calcHeuristic(PowerPole* a) {
			return a->en->pos().distance(target->en->pos());
		}

		bool rayCast(PowerPole* a, PowerPole* b) {
			return false;
		}
	};
}

void PowerPole::reset() {
	all.clear();
}

void PowerPole::tick() {
	queue.tick();
	for (auto pole: queue) {
		pole->root = 0;
		for (auto root: roots) {
			Trace path;
			path.init(pole, root);
			if (path.run()) {
				pole->root = root->id;
				break;
			}
		}
		queue.insert(pole);
	}
}

bool PowerPole::covered(Box box) {
	for (auto pole: gridCoverage.search(box)) {
		if (pole->coverage().intersects(box)) return true;
	}
	return false;
}

bool PowerPole::powered(Box box) {
	for (auto pole: gridCoverage.search(box)) {
		if (pole->root && pole->coverage().intersects(box)) return true;
	}
	return false;
}

PowerPole& PowerPole::create(uint id) {
	ensure(!all.has(id));
	PowerPole& pole = all[id];
	pole.id = id;
	pole.en = &Entity::get(id);
	pole.managed = false;
	queue.insert(&pole);
	if (pole.en->spec->powerpoleRoot)
		roots.insert(&pole);
	return pole;
}

PowerPole& PowerPole::get(uint id) {
	return all.refer(id);
}

void PowerPole::destroy() {
	ensure(!managed);
	if (en->spec->powerpoleRoot)
		roots.erase(this);
	queue.erase(this);
	all.erase(id);
}


void PowerPole::manage() {
	ensure(!managed);
	managed = true;
	connect();
	gridCoverage.insert(coverage(), this);
}

void PowerPole::unmanage() {
	ensure(managed);
	managed = false;
	disconnect();
	gridCoverage.remove(coverage(), this);
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
