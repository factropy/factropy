#include "common.h"
#include "entity.h"
#include "drone.h"

// Drone components are deployed by Depots to move items between Stores.

void Drone::reset() {
	all.clear();
}

void Drone::tick() {
	for (auto& drone: all) {
		drone.update();
	}
	// purge expired paths
	while (queue.size() && queue.front()->until <= Sim::tick) {
		auto path = queue.shift();
		auto a = path->origin;
		auto b = path->target;
		cache[a].erase(b);
		if (!cache[a].size()) cache.erase(a);
	}
}

Drone& Drone::create(uint id) {
	ensure(!all.has(id));
	Drone& drone = all[id];
	drone.id = id;
	drone.en = &Entity::get(id);
	drone.iid = 0;
	drone.dep = 0;
	drone.src = 0;
	drone.dst = 0;
	drone.srcGhost = false;
	drone.dstGhost = false;
	drone.repairing = false;
	drone.stage = Stranded;
	drone.altitude = 0.0f;
	drone.range = 0.0f;
	return drone;
}

Drone& Drone::get(uint id) {
	return all.refer(id);
}

void Drone::destroy() {
	if (dep && Entity::exists(dep)) {
		Entity::get(dep).depot().drones.erase(id);
	}
	all.erase(id);
}

float Drone::flightPathAltitude(Point a, Point b) {
	// check for a recent duplicate flight (within ~10s)
	if (cache.count(a) && cache[a].count(b)) {
		auto& path = cache[a][b];
		if (path.until > Sim::tick) {
			return path.altitude;
		}
	}

	Point dir = (b-a).normalize();
	real dist = a.distance(b);
	float height = 0.0f;

	auto checkPointTerrain = [&](Point stepPoint) {
		height = std::max(height, world.elevation(stepPoint));
	};

	auto checkPointEntities = [&](Point stepPoint) {
		Box stepBox = {stepPoint.x, 0.0f, stepPoint.z, 1.0f, 1000.0f, 1.0f};
		for (auto eh: Entity::grid.dump(stepBox)) {
			if (eh->spec->drone) continue;
			if (eh->spec->missile) continue;
			if (eh->spec->zeppelin) continue;
			if (eh->spec->flightPath) continue;
			if (eh->spec->explosion) continue;
			if (eh->spec->starship) continue;
			if (eh->pos().y < 1.1f) continue;
			height = std::max((real)height, eh->pos().y + eh->spec->collision.h*0.5f);
		}
	};

	real terrainStep = 5.0;
	int terrainSteps = std::max(1, (int)(dist/terrainStep));

	for (int i = 0; i <= terrainSteps; i++) {
		checkPointTerrain(a + (dir * (terrainStep * (real)i)));
	}

	if (height < 10) {
		real entityStep = Entity::GRID;
		int entitySteps = std::max(1, (int)(dist/entityStep));

		for (int i = 0; i <= entitySteps && height < 10; i++) {
			checkPointEntities(a + (dir * (entityStep * (real)i)));
		}
	}

	cache[a][b] = {
		.origin = a,
		.target = b,
		.altitude = height,
		.until = Sim::tick+(60*10),
	};

	queue.push(&cache[a][b]);

	return height;
}

bool Drone::travel(Entity* te) {
	if (!te) return false;
	float speed = en->spec->droneSpeed * speedFactor;

	Point dronePoint = {0, te->spec->collision.h*0.5f + 1.0f, 0};
	Point arrivalPoint = te->pos() + (te->spec->dronePoint == Point::Zero ? dronePoint: te->spec->dronePoint);

	if (altitude < 0.5f) {
		altitude = flightPathAltitude(en->pos().floor(0.0f), arrivalPoint.floor(0.0f));
		altitude = std::max(std::max((real)altitude, arrivalPoint.y), en->pos().y);
	}

	Point headingPoint = {arrivalPoint.x, en->pos().y, arrivalPoint.z};

	en->lookAt(headingPoint);

	float arrivalDistSquared = en->pos().distanceSquared(arrivalPoint);

	if (arrivalDistSquared < (speed*speed*1.1f)) {
		en->move(arrivalPoint);
		return false;
	}

	float arrivalDistGroundSquared = en->pos().floor(0.0f).distanceSquared(arrivalPoint.floor(0.0f));

	if (std::min(arrivalDistSquared, arrivalDistGroundSquared) < altitude*altitude) {
		Point arrivalDir = (arrivalPoint - en->pos()).normalize();
		en->move(en->pos() + (arrivalDir * speed));
		return true;
	}

	Point headingDir = (headingPoint - en->pos()).normalize();

	// 45deg
	if (en->pos().y < altitude) {
		Point delta = (headingDir * (speed/1.175)) + (Point::Up * (speed/1.175));
		en->move(en->pos() + delta);
		return true;
	}

	en->move(en->pos() + (headingDir * speed));
	return true;
}

void Drone::rangeCheck(Point home) {
	if (range < 1.0f) return;
	if (stage == ToDep) return;

	auto se = Entity::find(src);
	auto de = Entity::find(dst);

	float sr = se ? se->pos().distance(home) : range*2;
	float dr = de ? de->pos().distance(home) : range*2;

	if (stage == ToSrc && (sr > range || dr > range)) {
		iid = 0;
		stage = ToDep;
		altitude = 0.0f;
	}

	if (stage == ToDst && dr > range) {
		stage = ToDep;
		altitude = 0.0f;
	}
}

void Drone::update() {
	if (en->isGhost()) return;

	switch (stage) {
		case ToSrc: {
			auto se = Entity::find(src);

			if (!se || se->isGhost() != srcGhost) {
				stage = ToDep;
				iid = 0;
				altitude = 0.0f;
				break;
			}

			if (!travel(se)) {
				Store& store = srcGhost ? se->ghost().store: se->store();
				stack = store.remove(stack);
				store.drones.erase(id);
				altitude = 0.0f;
				if (stack.size) {
					stage = ToDst;
					iid = stack.iid;
					break;
				}
				stage = ToDep;
				iid = 0;
				break;
			}

			break;
		}

		case ToDst: {
			auto de = Entity::find(dst);

			if (!de || de->isGhost() != dstGhost) {
				stage = ToDep;
				altitude = 0.0f;
				break;
			}

			if (!travel(de)) {
				if (repairing) {
					de->repair(Item::get(iid)->repair);
					stage = ToDep;
					iid = 0;
					altitude = 0.0f;
					repairing = false;
					break;
				}
				Store& store = dstGhost ? de->ghost().store: de->store();
				store.insert(stack);
				store.drones.erase(id);
				stage = ToDep;
				iid = 0;
				altitude = 0.0f;
				break;
			}

			break;
		}

		case ToDep: {
			auto ed = Entity::find(dep);

			if (!ed) {
				stage = Stranded;
				iid = 0;
				altitude = 0.0f;
				break;
			}

			if (!travel(ed)) {
				if (iid && ed->spec->store) {
					ed->store().insert(stack);
				}
				en->remove();
				break;
			}

			break;
		}

		case Stranded: {
			// crash?
			en->remove();
			break;
		}
	}
}
