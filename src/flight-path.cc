#include "common.h"
#include "flight-path.h"
#include "route.h"

// Cooperative flight component

void FlightPath::reset() {
	all.clear();
}

void FlightPath::tick() {
	for (auto& flight: all) {
		flight.update();
	}
}

FlightPath& FlightPath::create(uint id) {
	ensure(!all.has(id));
	FlightPath& flight = all[id];
	flight.id = id;
	flight.en = &Entity::get(id);
	flight.destination = Point::Zero;
	flight.arrived = 0;
	flight.origin = Point::Zero;
	flight.departed = 0;
	flight.orient = Point::Zero;
	flight.speed = 0.0f;
	flight.fueled = 1.0f;
	flight.moving = false;
	flight.pause = 0;
	flight.request = false;
	flight.step = 0;
	return flight;
}

FlightPath& FlightPath::get(uint id) {
	return all.refer(id);
}

void FlightPath::destroy() {
	all.erase(id);
}

void FlightPath::update() {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;
	if (pause > Sim::tick) return;

	if (request) {
		step = 0;
		path = sky.path(en->pos(), destination, id);
		request = path.waypoints.size() == 0;
		moving = !request;

		if (!moving) {
			pause = Sim::tick + 60;
		}
	}

	if (!moving) return;

	en->state++;
	if (en->state >= en->spec->states.size()) en->state = 0;

	double destinationDist = en->pos().distance(destination);
	for (auto block: path.blocks) {
		if (block->centroid().distance(destination) < destinationDist+sky.chunk())
			block->reserve(id);
	}

	fueled = en->consumeRate(en->spec->energyConsume);

	ensure(step >= 0 && step <= (int)path.waypoints.size());
	bool finalStep = (int)path.waypoints.size() == step;

	Point waypoint = finalStep ? destination: path.waypoints[step];
	double waypointDist = en->pos().distance(waypoint);

	Point headingPoint = {waypoint.x, en->pos().y, waypoint.z};
	Point headingDir = (headingPoint - en->pos()).normalize();

	Point lookDir = (finalStep && orient != Point::Zero) ? orient: headingDir;
	Point lookAt = en->pos() + lookDir;

	float maxSpeed = std::max(0.01f, en->spec->flightPathSpeed * fueled);
	float turnSpeed = std::max(0.005f, (en->spec->flightPathSpeed/25.0f) * fueled);

	bool waypointClose = waypointDist < speed*1.1f;
	bool destinationClose = waypointClose && finalStep;

	if (waypointDist < en->spec->collision.d && !finalStep) {
		ensure(step < (int)path.waypoints.size());
		step++;
		waypoint = (int)path.waypoints.size() > step ? path.waypoints[step]: destination;
		waypointDist = en->pos().distance(waypoint);
	}

	if (destinationClose) {
		ensure(step == (int)path.waypoints.size());
		en->move(waypoint, lookDir);
		speed = 0.0f;
		moving = false;
		arrived = Sim::tick;
		path.blocks.clear();
		path.waypoints.clear();
		step = 0;
		return;
	}

	if (waypointClose) {
		en->move(waypoint, lookDir);
		step++;
		ensure(step < (int)path.waypoints.size());
		return;
	}

	if (!en->lookingAt(lookAt)) {
		en->lookAtPivot(lookAt, turnSpeed);
	}

	auto bestSpeed = [&](float targetSpeed) {
		return std::min(targetSpeed, !en->lookingAt(lookAt) ? maxSpeed/2.0f*0.99f: maxSpeed);
	};

	Point cruiseDir = (waypoint - en->pos()).normalize();
	move(cruiseDir, bestSpeed(maxSpeed));
}

bool FlightPath::move(Point dir, float targetSpeed) {
	float maxSpeed = std::max(0.01f, en->spec->flightPathSpeed * fueled);
	float minSpeed = std::max(0.01f, (en->spec->flightPathSpeed/5.0f) * fueled);

	speed = std::max(minSpeed, speed);

	if (speed < targetSpeed) {
		speed = speed*1.01f;
		speed = std::min(targetSpeed, speed);
	}

	if (speed > targetSpeed) {
		speed = speed*0.99f;
		speed = std::max(targetSpeed, speed);
	}

	speed = std::min(maxSpeed, speed);

	Point pos = en->pos() + (dir*speed);
	en->move(pos);

	return true;
}

void FlightPath::depart(Point to, Point at) {
	float y = std::max(to.y, en->spec->collision.h/2.0f);

	orient = at;
	origin = en->pos();
	destination = {to.x, y, to.z};
	departed = Sim::tick;
	arrived = 0;
	request = true;
}

void FlightPath::cancel() {
	moving = false;
	request = false;
	path.blocks.clear();
	path.waypoints.clear();
	step = 0;
	speed = 0;
}

bool FlightPath::departing() {
	return request && !moving;
}

bool FlightPath::flying() {
	return !request && moving;
}

bool FlightPath::done() {
	return !request && !moving;
}
