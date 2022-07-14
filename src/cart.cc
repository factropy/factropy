#include "common.h"
#include "cart.h"
#include <set>

// Logistic carts

void Cart::reset() {
	all.clear();
}

void Cart::tick() {

	// check waypoints with reservations; those with a reserving cart still intersecting, extend reservation

	std::set<uint> reserved;

	for (auto& waypoint: CartWaypoint::all) {
		if (!waypoint.reserverId) continue;
		if (!all.has(waypoint.reserverId)) {
			waypoint.reserverId = 0;
			continue;
		}
		auto& ew = Entity::get(waypoint.id);
		if (ew.isGhost()) continue;
		auto& cart = get(waypoint.reserverId);
		auto& ec = Entity::get(cart.id);
		if (ec.isGhost()) {
			waypoint.reserverId = 0;
			continue;
		}
		if (waypoint.reserveUntil > Sim::tick) {
			reserved.insert(waypoint.id);
			cart.status = fmt("%u holds %u", waypoint.reserverId, waypoint.id);
			continue;
		}
		if (ec.groundSphere().intersects(ew.groundSphere())) {
			waypoint.reserveUntil = Sim::tick+30;
			reserved.insert(waypoint.id);
			cart.status = fmt("%u renews %u", waypoint.reserverId, waypoint.id);
		}
	}

	// identify carts over unreserved waypoints; closest has reservation

	struct cartProximity {
		Cart* cart = nullptr;
		double dist = 0.0f;
	};

	std::map<CartWaypoint*,std::vector<cartProximity>> proximity;

	for (auto& cart: all) {
		auto& ec = Entity::get(cart.id);
		if (ec.isGhost()) continue;
		auto space = ec.groundSphere();
		for (auto ew: Entity::intersecting(ec.box().grow(2.0f), Entity::gridCartWaypoints)) {
			ensure(ew->spec->cartWaypoint);
			if (ew->isGhost()) continue;
			if (!ew->groundSphere().intersects(space)) continue;
			auto& waypoint = ew->cartWaypoint();
			if (reserved.count(waypoint.id)) continue;
			proximity[&waypoint].push_back({&cart, ec.pos().distance(ew->pos())});
		}
	}

	for (auto& [waypoint,cartProximities]: proximity) {
		std::sort(cartProximities.begin(), cartProximities.end(), [&](auto a, auto b) { return a.dist < b.dist; });
		waypoint->reserverId = cartProximities[0].cart->id;
		waypoint->reserveUntil = Sim::tick+30;
		cartProximities[0].cart->status = fmt("%u reserves %u", waypoint->reserverId, waypoint->id);
	}

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

std::vector<Point> Cart::sensors() {
	Point front = en->pos() + (en->dir() * (en->spec->collision.d*0.5f));
	Point left  = front + (en->dir().transform(Mat4::rotateY(glm::radians( 90.0f))) * (en->spec->collision.w*0.25f));
	Point right = front + (en->dir().transform(Mat4::rotateY(glm::radians(-90.0f))) * (en->spec->collision.w*0.25f));
	return std::vector<Point>{left, right};
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

	for (auto point: sensors()) {
		if (!world.isLand(point)) {
			halt = true;
			blocked = true;
			pause = Sim::tick + 30;
			return;
		}
	}

	bool overReservedWaypoint = false;
	auto nearEntities = Entity::intersecting(en->box().grow(2.0f));

	for (auto ec: nearEntities) {
		if (ec->id == id) continue;

		if (ec->spec->junk) {
			ec->deconstruct();
		}

		if (ec->spec->cartWaypoint && ec->box().contains(en->pos().floor(0.0f))) {
			if (ec->cartWaypoint().reserverId == id) {
				overReservedWaypoint = true;
			}
		}

		float offset = en->spec->collision.d/2.0f;
		Point ahead = en->pos() + (en->dir()*(offset*1.1f));

		if (ec->spec->cartWaypoint && ec->box().contains(ahead.floor(0.0f))) {
			auto& waypoint = ec->cartWaypoint();
			if (waypoint.reserveUntil < Sim::tick) {
				waypoint.reserverId = id;
				waypoint.reserveUntil = Sim::tick+30;
				status = fmt("%u waypoint reserved", ec->id);
			}
		}

		if (ec->spec->cart) {
			if (ec->cuboid().intersects(Sphere(ahead, 0.1f))) {
				halt = true;
				blocked = std::abs(en->dir().degrees(ec->dir())) > 120.0f;
				status = fmt("%u crash cart %u", id, ec->id);
			}
		}
	}

	if (!halt && !overReservedWaypoint) {
		for (auto ec: nearEntities) {
			if (ec->id == id) continue;
			if (ec->spec->drone) continue;
//			if (ec->isGhost() && ec->spec->junk) continue;

			float offset = en->spec->collision.d/2.0f;
			Point ahead = en->pos() + (en->dir()*(offset*1.1f));

			if (ec->spec->cartWaypoint && ec->box().contains(ahead.floor(0.0f))) {
				auto& waypoint = ec->cartWaypoint();
				if (waypoint.reserverId != id) {
					halt = true;
					blocked = false;
					status = fmt("%u waypoint ahead not reserved %u", id, ec->id);
				}
				continue;
			}

			if (ec->box().contains(ahead)) {
				halt = true;
				blocked = true;
				status = fmt("%u crash %u", id, ec->id);
				continue;
			}
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
