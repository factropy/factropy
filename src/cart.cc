#include "common.h"
#include "cart.h"
#include <set>

// Logistic carts

void Cart::reset() {
	all.clear();
}

void Cart::tick() {
	for (auto& cart: all) cart.update();
	for (auto& cart: all) if (cart.blocked) Sim::alerts.vehiclesBlocked++;
}

Cart& Cart::create(uint id) {
	ensure(!all.has(id));
	Cart& cart = all[id];
	cart.id = id;
	cart.en = &Entity::get(id);
	cart.state = State::Start;
	cart.target = Point::Zero;
	cart.origin = Point::Zero;
	cart.next = Point::Zero;
	cart.line = CartWaypoint::Red;
	cart.wait = 0;
	cart.pause = 0;
	cart.surface = 0;
	cart.fueled = 0;
	cart.halt = false;
	cart.lost = false;
	cart.slab = false;
	cart.blocked = false;
	cart.signal.value = 1;
	cart.regionNext = 0;
	return cart;
}

Cart& Cart::get(uint id) {
	return all.refer(id);
}

void Cart::destroy() {
	all.erase(id);
}

CartSettings* Cart::settings() {
	return new CartSettings(*this);
}

CartSettings::CartSettings(Cart& cart) {
	line = cart.line;
	signal = cart.signal;
}

void Cart::setup(CartSettings* settings) {
	line = settings->line;
	signal = settings->signal;
}

void Cart::update() {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;
	if (pause > Sim::tick) return;

	switch (state) {
		case State::Start: start(); break;
		case State::Travel: travel(); break;
		case State::Stop: stop(); break;
		case State::Ease: ease(); break;
	}
}

CartStop* Cart::getStop(Point point) {
	for (auto ew: Entity::intersecting(point.floor(0).box().grow(0.1f), Entity::gridCartWaypoints)) {
		if (ew->spec->cartStop) return &ew->cartStop();
	}
	return nullptr;
}

CartWaypoint* Cart::getWaypoint(Point point) {
	for (auto ew: Entity::intersecting(point.floor(0).box().grow(0.1f), Entity::gridCartWaypoints)) {
		if (ew->spec->cartWaypoint) return &ew->cartWaypoint();
	}
	return nullptr;
}

float Cart::maxSpeed() {
	return slab ? en->spec->cartSpeed*1.5: en->spec->cartSpeed;
}

const std::array<Point,2> Cart::sensors() {
	Point right = en->dir().transform(Mat4::rotateY(glm::radians( 90.0f)));
	Point front = en->pos() + (en->dir() * (en->spec->collision.d*0.51f));
	msensors[0] = front + (right * (en->spec->collision.w*0.25f));
	msensors[1] = front + (right * (-en->spec->collision.w*0.25f));
	return msensors;
}

Point Cart::ahead() {
	return en->pos() + (en->dir() * (en->spec->collision.d*0.5f)) + (en->dir()*0.5f);
}

Point Cart::park() {
	Point pos = target;
	if (getStop(target)) {
		Point dir = (target-en->pos()).normalize();
		// adjusts for longer truck to stop neatly on grid
		pos = target - (dir * ((en->spec->collision.d-2.0)/2.0));
	}
	return pos;
}

void Cart::start() {
	CartWaypoint* waypoint = getWaypoint(en->pos());
	lost = true;
	if (waypoint) {
		next = waypoint->getPos();
		next.y = en->pos().y;
		target = next;
		origin = en->pos();
		state = State::Travel;
		lost = false;
	}
}

void Cart::stop() {
	CartStop* stop = getStop(en->pos());

	halt = false;
	lost = false;
	blocked = false;

	if (stop && stop->depart == CartStop::Depart::Inactivity) {
		if (en->spec->store && Sim::tick >= en->spec->cartWait && en->store().activity > Sim::tick-en->spec->cartWait) {
			wait = en->store().activity + en->spec->cartWait;
		}
	}

	if (stop && stop->depart == CartStop::Depart::Empty) {
		if (en->spec->store && !en->store().isEmpty()) {
			wait = Sim::tick + en->spec->cartWait;
		}
	}

	if (stop && stop->depart == CartStop::Depart::Full) {
		if (en->spec->store && !en->store().isFull()) {
			wait = Sim::tick + en->spec->cartWait;
		}
	}

	if (!stop) {
		lost = true;
	}

	if (wait <= Sim::tick) {
		state = State::Ease;
	}

	if (wait > Sim::tick) {
		// shorter than waypoint.reserve()
		pause = Sim::tick + 15;
	}
}

void Cart::travel() {

	halt = false;
	lost = false;
	blocked = false;

	float distance = en->pos().distance(target);

	// When approaching a waypoint, move it gradually along the vector toward
	// then next waypoint so the cart appears to smoothly turn the corner
	if (distance > maxSpeed()/2.0f && distance < en->spec->collision.d/2.0f && !getStop(next)) {
		CartWaypoint* waypoint = getWaypoint(next);
		if (waypoint) {
			line = waypoint->redirect(line, signals());
		}
		if (waypoint && waypoint->hasNext(line)) {
			Point ahead = waypoint->getNext(line);
			ahead = {ahead.x, en->pos().y, ahead.z};
			// don't turn into the back of another cart
			if (ahead.distance(target) > en->spec->collision.d*2.0f) {
				Point v = ahead-target;
				Point n = v.normalize();
				Point s = n * (maxSpeed()/2.0f);
				target = target+s;
				origin = en->pos();
				distance = target.distance(en->pos());
				en->lookAt(target);
			}
		}
	}
	else
	if (!en->lookAtPivot(target, maxSpeed())) {
		return;
	}

	auto nearEntities = Entity::intersecting(en->box().grow(2.0f));

	// remove previous colliders out of range
	localvec<uint> drop;
	for (auto cid: colliders) {
		bool near = false;
		for (auto ec: nearEntities)
			 if (ec->id == cid && ec->cart().halt) near = true;
		if (!near) drop.push(cid);
	}
	for (auto cid: drop) {
		colliders.erase(cid);
	}

	for (auto point: sensors()) {
		if (halt) break;

		if (regionNext < Sim::tick) {
			region = world.region(en->box().grow(5));
			regionNext = Sim::tick+180;
		}

		if (!region.isLand(point)) {
			halt = true;
			blocked = true;
			break;
		}

		for (auto ec: nearEntities) {
			if (ec->id == id) continue;
			if (ec->spec->cartWaypoint) continue;
			if (ec->spec->drone) continue;
			if (ec->spec->slab) continue;

			if (!ec->spec->cart && !ec->box().contains(point)) continue;
			if (!ec->cuboid().contains(point)) continue;

			if (ec->spec->cart) {
				// they've seen us and stopped; carry on
				if (colliders.has(ec->id)) continue;
				// they haven't seen us; stop and let them know
				ec->cart().colliders.insert(id);
			}

			blocked = !ec->spec->cart;
			halt = true;
			break;
		}
	}

	if (halt) {
		pause = Sim::tick + 30;
		return;
	}

	if (surface <= Sim::tick) {
		surface = Sim::tick+60;
		slab = false;
		for (auto es: Entity::intersecting(en->pos().floor(0.0).box().grow(0.5), Entity::gridSlabs)) {
			slab = slab || (!es->isGhost() && es->spec->slab);
		}
	}

	fueled = en->consumeRate(en->spec->energyConsume);
	speed = std::max(0.01f, maxSpeed() * fueled);

	// trucks pull up early
	Point parked = park();

	if (parked.distance(en->pos()) < speed) {
		en->move(parked);
		CartStop* stop = getStop(en->pos());
		if (!stop) {
			CartWaypoint* waypoint = getWaypoint(next);
			if (waypoint) {
				line = waypoint->redirect(line, signals());
			}
			if (waypoint && waypoint->hasNext(line)) {
				next = waypoint->getNext(line);
				next = {next.x,en->pos().y,next.z};
				target = next;
				origin = en->pos();
				return;
			}
		}
		wait = Sim::tick + en->spec->cartWait;
		state = State::Stop;
		return;
	}

	Point p = en->pos();
	if (origin != Point::Zero && origin.distance(target) > 1.0) {
		p = p.nearestPointOnLine(origin, target);
	}

	Point v = target - p;
	Point n = v.normalize();
	Point s = n * speed;
	en->move(p + s);
}

// if truck pulled up slightly short, ease ahead to exit cleanly
// no-op for carts
void Cart::ease() {
	fueled = en->consumeRate(en->spec->energyConsume);
	speed = std::max(0.01f, maxSpeed() * fueled);
	if (en->pos().distance(target) < speed) {
		CartWaypoint* waypoint = getWaypoint(en->pos());
		if (waypoint) {
			line = waypoint->redirect(line, signals());
		}
		if (waypoint && waypoint->hasNext(line)) {
			next = waypoint->getNext(line);
			next.y = en->pos().y;
			target = next;
			origin = en->pos();
			state = State::Travel;
		}
		return;
	}

	Point p = en->pos();
	if (origin != Point::Zero && origin.distance(target) > 1.0) {
		p = p.nearestPointOnLine(origin, target);
	}

	Point v = target - p;
	Point n = v.normalize();
	Point s = n * speed;
	en->move(p + s);
}

void Cart::travelTo(Point point) {
	next = {point.x, en->pos().y, point.z};
	target = next;
	origin = en->pos();
	state = State::Travel;
}

minimap<Signal,&Signal::key> Cart::signals() {
	auto sigs = en->store().signals();
	if (signal.valid()) sigs.insert(signal);
	return sigs;
}
