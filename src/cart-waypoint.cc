#include "common.h"
#include "cart-waypoint.h"

// CartWaypoint components are markers with rules used by Carts to navigate

void CartWaypoint::reset() {
	all.clear();
}

CartWaypoint& CartWaypoint::create(uint id) {
	ensure(!all.has(id));
	CartWaypoint& waypoint = all[id];
	waypoint.id = id;
	waypoint.relative[Red] = Point::Zero;
	waypoint.relative[Blue] = Point::Zero;
	waypoint.relative[Green] = Point::Zero;
	return waypoint;
}

CartWaypoint& CartWaypoint::get(uint id) {
	return all.refer(id);
}

void CartWaypoint::destroy() {
	all.erase(id);
}

Point CartWaypoint::getPos() const {
	return Entity::get(id).pos();
}

void CartWaypoint::setNext(int line, Point absolute) {
	relative[line] = absolute - getPos();
}

void CartWaypoint::clrNext(int line) {
	relative[line] = Point::Zero;
}

Point CartWaypoint::getNext(int line) const {
	return getPos() + relative[line];
}

bool CartWaypoint::hasNext(int line) const {
	return relative[line] != Point::Zero;
}

CartWaypointSettings* CartWaypoint::settings() {
	return new CartWaypointSettings(*this);
}

CartWaypointSettings::CartWaypointSettings(CartWaypoint& waypoint) {
	auto& en = Entity::get(waypoint.id);
	dir = en.dir();

	relative[CartWaypoint::Red] = waypoint.relative[CartWaypoint::Red];
	relative[CartWaypoint::Blue] = waypoint.relative[CartWaypoint::Blue];
	relative[CartWaypoint::Green] = waypoint.relative[CartWaypoint::Green];

	for (auto& redirection: waypoint.redirections)
		redirections.push_back(redirection);
}

void CartWaypoint::setup(CartWaypointSettings* settings) {
	auto& en = Entity::get(id);

	auto rotA = settings->dir.rotation();
	auto rotB = en.dir().rotation();

	relative[Red] = settings->relative[Red];
	relative[Blue] = settings->relative[Blue];
	relative[Green] = settings->relative[Green];

	auto rot = [&](Point p) {
		if (p == Point::Zero) return p;
		return p.transform(rotA.invert()).transform(rotB);
	};

	relative[Red] = rot(relative[Red]);
	relative[Blue] = rot(relative[Blue]);
	relative[Green] = rot(relative[Green]);

	redirections.clear();
	for (auto& redirection: settings->redirections)
		redirections.push_back(redirection);
}

int CartWaypoint::redirect(int line, minimap<Signal,&Signal::key> signals) {
	for (auto& redirection: redirections) {
		if (redirection.condition.evaluate(signals)) return redirection.line;
	}
	return line;
}
