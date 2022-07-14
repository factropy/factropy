#include "common.h"
#include "zeppelin.h"
#include "sim.h"

void Zeppelin::reset() {
	all.clear();
}

void Zeppelin::tick() {
	for (auto& zeppelin: all) {
		zeppelin.update();
	}
}

Zeppelin& Zeppelin::create(uint id) {
	ensure(!all.has(id));
	auto& en = Entity::get(id);
	Zeppelin& zeppelin = all[id];
	zeppelin.id = id;
	zeppelin.target = Point::Zero;
	zeppelin.speed = 0.0f;
	zeppelin.altitude = en.spec->zeppelinAltitude;
	zeppelin.moving = false;
	return zeppelin;
}

Zeppelin& Zeppelin::get(uint id) {
	return all.refer(id);
}

void Zeppelin::destroy() {
	all.erase(id);
}

void Zeppelin::update() {
	Entity& en = Entity::get(id);
	if (en.isGhost()) return;
	if (!en.isEnabled()) return;

	en.state++;
	if (en.state == en.spec->states.size()) en.state = 0;

	if (en.spec->depot) {
		auto& depot = en.depot();
		depot.recall = false;
	}

	if (target == Point::Zero) target = en.pos().floor(0.0f);
	if (target == en.pos().floor(0.0f)) { speed = 0.0f; moving = false; return; }

	moving = true;

	float fueled = en.consumeRate(en.spec->energyConsume);
	float maxSpeed = std::max(0.01f, en.spec->zeppelinSpeed * fueled);
	float minSpeed = std::max(0.01f, (en.spec->zeppelinSpeed/5.0f) * fueled);
	float turnSpeed = std::max(0.005f, (en.spec->zeppelinSpeed/50.0f) * fueled);

	auto elevation = world.elevation(target);
	Point arrivalPoint = target.floor(std::max(0.0f, elevation) + en.spec->zeppelinAltitude);

	Point headingPoint = {arrivalPoint.x, en.pos().y, arrivalPoint.z};
	Point headingDir = (headingPoint - en.pos()).normalize();
	float arrivalDist = en.pos().distance(arrivalPoint);
	float arrivalDistGround = en.pos().floor(0.0f).distance(arrivalPoint.floor(0.0f));

	if (en.spec->depot) {
		auto& depot = en.depot();
		depot.recall = true;
		if (arrivalDist > en.spec->depotRange && depot.drones.size()) return;
	}

	en.lookAtPivot(headingPoint, turnSpeed);

	bool turning = en.dir().degrees(headingDir) > 1.0f;
	if (turning) maxSpeed *= 0.5f;

	if (arrivalDist < speed*1.1f) {
		en.move(arrivalPoint);
		speed = 0.0f;
		moving = false;
		return;
	}

	// moving
	speed = std::max(minSpeed, speed);

	// descending
	if (std::min(arrivalDist, arrivalDistGround) < altitude) {
		speed = std::max(minSpeed, speed*0.99f);
		Point arrivalDir = (arrivalPoint - en.pos()).normalize();
		en.move(en.pos() + (arrivalDir * speed));
		return;
	}

	// ascending from ground
	if (en.pos().y < altitude/2.0f) {
		speed = std::min(maxSpeed/2.0f, speed*1.01f);
		Point delta = Point::Up * speed;
		en.move(en.pos() + delta);
		return;
	}

	// ascending
	if (en.pos().y < altitude) {
		speed = std::min(maxSpeed, speed*1.01f);
		Point delta = (headingDir * (speed/1.175)) + (Point::Up * (speed/1.175));
		en.move(en.pos() + delta);
		return;
	}

	speed = std::min(maxSpeed, speed*1.01f);
	en.move(en.pos() + (headingDir * speed));
}

void Zeppelin::flyOver(Point point) {
	Entity& en = Entity::get(id);
	target = point.floor(0.0f);
	altitude = std::max((real)en.spec->zeppelinAltitude, flightPathAltitude(en.pos().floor(0.0f), target) + en.spec->collision.h);
}

float Zeppelin::flightPathAltitude(Point a, Point b) {
	Point dir = (b-a).normalize();
	float height = 0.0f;

	auto checkPoint = [&](Point stepPoint) {
		auto elevation = world.elevation(stepPoint);
		height = std::max(height, elevation);

		Box stepBox = {stepPoint.x, 0.0f, stepPoint.z, 1.0f, 1000.0f, 1.0f};
		for (auto eh: Entity::grid.dump(stepBox)) {
			if (eh->id == id) continue;
			if (eh->spec->drone) continue;
			if (eh->spec->flightPath) continue;
			if (eh->spec->zeppelin) continue;
			if (eh->spec->missile) continue;
			if (eh->spec->explosion) continue;
			if (eh->pos().y < 1.1f) continue;
			height = std::max((real)height, eh->pos().y + eh->spec->collision.h*0.5f);
		}
	};

	float step = 1.0f;
	int steps = (int)std::floor(a.distance(b) / step);

	for (int i = 0; i < steps; i++) {
		checkPoint(a + (dir * (step * (float)i)));
	}

	checkPoint(b);

	return height;
}
