#include "common.h"
#include "vehicle.h"
#include "sim.h"

// Vehicle components are ground cars that path-find and drive around the map.
// They can be manualy told to move, or to follow a series of waypoints in patrol
// mode.

const uint Vehicle::DepartItem::Eq = 1;
const uint Vehicle::DepartItem::Ne = 2;
const uint Vehicle::DepartItem::Lt = 3;
const uint Vehicle::DepartItem::Lte = 4;
const uint Vehicle::DepartItem::Gt = 5;
const uint Vehicle::DepartItem::Gte = 6;

void Vehicle::reset() {
	all.clear();
}

void Vehicle::tick() {
	for (auto& vehicle: all) {
		vehicle.update();
	}
}

Vehicle& Vehicle::create(uint id) {
	ensure(!all.has(id));
	Vehicle& vehicle = all[id];
	vehicle.id = id;
	vehicle.pause = 0;
	vehicle.patrol = false;
	vehicle.waypoint = nullptr;
	vehicle.handbrake = false;
	vehicle.pathRequest = nullptr;
	return vehicle;
}

Vehicle& Vehicle::get(uint id) {
	return all.refer(id);
}

void Vehicle::destroy() {
	if (pathRequest) {
		pathRequest->cancel = true;
		pathRequest = nullptr;
	}
	for (auto wp: waypoints) {
		if (waypoint == wp) {
			waypoint = nullptr;
		}
		delete wp;
	}
	if (waypoint) {
		delete waypoint;
	}
	all.erase(id);
}

void Vehicle::update() {
	Entity& en = Entity::get(id);
	if (en.isGhost()) return;
	if (!en.isEnabled()) return;
	if (handbrake || pause > Sim::tick) return;

	if (path.empty() && waypoint) {
		for (auto condition: waypoint->conditions) {
			if (!condition->ready(waypoint, this)) {
				return;
			}
		}
		waypoint = nullptr;
	}

	// check an existing pathfinding request for completion
	if (pathRequest && pathRequest->done) {

		if (pathRequest->success) {
			notef("path success");
			for (Point point: pathRequest->result) {
				path.push_back(point);
			}
		}

		if (waypoints.size()) {
			waypoint = waypoints.front();
			waypoints.pop_front();
			ensure(waypoint);

			if (patrol && pathRequest->success) {
				waypoints.push_back(waypoint);
				waypoint = nullptr;
			}

			if (waypoint) {
				delete waypoint;
				waypoint = nullptr;
			}
		}

		if (!pathRequest->success) {
			notef("path failure");
			pause = Sim::tick + 180;
		}

		delete pathRequest;
		pathRequest = nullptr;
	}

	// request pathfinding for the next leg of our route
	if (!pathRequest && path.empty() && !waypoints.empty()) {
		pathRequest = new Route(this);
		pathRequest->origin = Point(en.pos().x, 0.0f, en.pos().z).tileCentroid();
		pathRequest->target = waypoints.front()->position;
		pathRequest->submit();
		notef("send path request %f,%f,%f", pathRequest->target.x, pathRequest->target.y, pathRequest->target.z);
		return;
	}

	if (path.empty()) {
		pause = Sim::tick + 15;
		return;
	}

	Point centroid = en.pos().floor(0);
	Point target = path.front().floor(0);

	float fueled = en.consumeRate(en.spec->energyConsume);
	float distance = target.distance(centroid);
	float speed = std::max(0.01f, 0.1f * fueled);
	float clearance = en.spec->clearance;

	// When approaching a waypoint, move it gradually along the vector toward
	// then next waypoint so the vehicle appears to smoothly turn the corner
	if (distance < clearance && path.size() > 1) {
		auto it = path.begin(); ++it;
		Point next = (*it).floor(0);
		if (next.distance(target) > speed) {
			Point v = next-target;
			Point n = v.normalize();
			Point s = n * (speed/2.0f);
			path.front() = target+s;
			target = path.front();
			distance = target.distance(centroid);
			en.look(Point(target.x, en.pos().y, target.z)-en.pos());
		}
	}
	else
	if (!en.lookAtPivot({target.x, en.pos().y, target.z})) {
		return;
	}

	bool arrived = distance < speed;

	if (arrived) {
		en.move(target.floor(en.spec->collision.h/2.0f));
		path.pop_front();
		if (!path.size()) {
			pause = Sim::tick + 120;
		}
		return;
	}

	Point v = target - centroid;
	Point n = v.normalize();
	Point s = n * speed;

	bool crash = false;

	auto hits = Entity::intersecting(en.sphere().grow(speed*2.0f));

	discard_if(hits, [&](Entity* eh) {
		return eh->id == id || !all.has(eh->id) || !get(eh->id).moving();
	});

	for (auto p: sensors()) {
		auto s = p.sphere().grow(0.1f);
		for (auto eh: hits) {
			if (eh->sphere().intersects(s)) {
				crash = true;
				break;
			}
		}
	}

	if (!crash) {
		en.move((centroid + s).floor(en.spec->collision.h/2.0f));
	}
}

std::vector<Point> Vehicle::sensors() {
	Entity& en = Entity::get(id);
	Point ahead = en.pos() + (en.dir() * (en.spec->collision.d*0.5f));
	Point left  = ahead + (en.dir().transform(Mat4::rotateY(glm::radians( 90.0f))) * (en.spec->collision.w*0.25f));
	Point right = ahead + (en.dir().transform(Mat4::rotateY(glm::radians(-90.0f))) * (en.spec->collision.w*0.25f));
	return std::vector<Point>{left, right};
}

bool Vehicle::moving() {
	return (!path.empty() || pathRequest) && pause < Sim::tick;
}

bool Vehicle::collide(Spec* spec) {
	return !(spec->vehicle || spec->vehicleStop || spec->cart || spec->cartStop || spec->cartWaypoint);
}

Vehicle::Waypoint* Vehicle::addWaypoint(Point p) {
	if (!patrol) {
		path.clear();
	}
	p.y = 0.0f;
	waypoints.push_back(
		new Waypoint(p.tileCentroid())
	);
	return waypoints.back();
}

Vehicle::Waypoint* Vehicle::addWaypoint(uint eid) {
	if (!patrol) {
		path.clear();
	}
	waypoints.push_back(
		new Waypoint(eid)
	);
	return waypoints.back();
}

Vehicle::Route::Route(Vehicle *v) : Path() {
	vehicle = v;
}

std::vector<Point> Vehicle::Route::getNeighbours(Point p) {
	p = p.tileCentroid();
	float clearance = Entity::get(vehicle->id).spec->clearance;

	auto entities = Entity::intersecting(p.box().grow(clearance));

	std::vector<Point> points = {
		(p + Point( 1.0f, 0.0f, 0.0f)).tileCentroid(),
		(p + Point(-1.0f, 0.0f, 0.0f)).tileCentroid(),
		(p + Point( 0.0f, 0.0f, 1.0f)).tileCentroid(),
		(p + Point( 0.0f, 0.0f,-1.0f)).tileCentroid(),
		(p + Point( 1.0f, 0.0f, 1.0f)).tileCentroid(),
		(p + Point(-1.0f, 0.0f, 1.0f)).tileCentroid(),
		(p + Point( 1.0f, 0.0f,-1.0f)).tileCentroid(),
		(p + Point(-1.0f, 0.0f,-1.0f)).tileCentroid(),
	};

	for (auto hit: entities) {
		if (!collide(hit->spec)) continue;
		if (vehicle->id != hit->id) {
			Box box = hit->box();
			for (auto it = points.begin(); it != points.end(); ) {
				if (*it == target) {
					it++;
					continue;
				}
				if (box.contains(*it)) {
					points.erase(it);
					continue;
				}
				it++;
			}
		}
	}

	float range = origin.distance(target);

	for (auto it = points.begin(); it != points.end(); ) {
		if (!Entity::isLand(it->box().grow(clearance))) {
			points.erase(it);
			continue;
		}
		// capsule shaped range limit
		if ((*it).lineDistance(origin, target) > range) {
			points.erase(it);
			continue;
		}
		it++;
	}

	for (auto it = points.begin(); it != points.end(); ) {
		if (origin.tileCentroid() == *it) {
			*it = origin;
		}
		if (target.tileCentroid() == *it) {
			*it = target;
		}
		it++;
	}

	return points;
}

double Vehicle::Route::calcCost(Point a, Point b) {
	float clearance = Entity::get(vehicle->id).spec->clearance;
	float cost = a.distance(b);

	if (b == target || b.distance(target) < clearance*1.5f) {
		return cost;
	}

	if (!Entity::isLand(b.box().grow(clearance))) {
		cost *= 1000.0f;
	}

	auto entities = Entity::intersecting(b.box().grow(clearance, 0, clearance));

	for (auto hit: entities) {
		if (!collide(hit->spec)) {
			continue;
		}
		if (vehicle->id != hit->id) {
			cost *= 5.0f;
		}
	}

	return cost;
}

double Vehicle::Route::calcHeuristic(Point p) {
	return p.distance(target) * Entity::get(vehicle->id).spec->costGreedy;
}

bool Vehicle::Route::rayCast(Point a, Point b) {
	float clearance = Entity::get(vehicle->id).spec->clearance;

	float d = a.distance(b);
	if (d > 32.0f) return false;

	Point n = (b-a).normalize();

	for (Point c = a; c.distance(b) > 1.0f; c += n) {
		if (!Entity::isLand(c.box().grow(clearance))) {
			return false;
		}
		for (auto hit: Entity::intersecting(c.box().grow(1))) {
			if (!collide(hit->spec)) continue;
			if (vehicle->id != hit->id) {
				return false;
			}
		}
	}

	return true;
}

Vehicle::Waypoint::Waypoint(Point pos) {
	position = pos;
	stopId = 0;
}

Vehicle::Waypoint::Waypoint(uint eid) {
	Entity& en = Entity::get(eid);
	position = en.ground();
	stopId = en.id;
}

Vehicle::Waypoint::~Waypoint() {
	for (auto c: conditions) {
		delete c;
	}
	conditions.clear();
}

Vehicle::DepartCondition::~DepartCondition() {
}

Vehicle::DepartInactivity::~DepartInactivity() {
}

bool Vehicle::DepartInactivity::ready(Waypoint* waypoint, Vehicle *vehicle) {
	Entity& en = Entity::get(vehicle->id);
	return !en.spec->store || en.store().activity < Sim::tick-(uint)(seconds*60);
}

Vehicle::DepartItem::~DepartItem() {
}

bool Vehicle::DepartItem::ready(Waypoint* waypoint, Vehicle *vehicle) {
	Entity& en = Entity::get(vehicle->id);

	if (en.spec->store) {
		switch (op) {
			case Eq: {
				return en.store().count(iid) == count;
			}
			case Ne: {
				return en.store().count(iid) != count;
			}
			case Lt: {
				return en.store().count(iid) < count;
			}
			case Lte: {
				return en.store().count(iid) <= count;
			}
			case Gt: {
				return en.store().count(iid) > count;
			}
			case Gte: {
				return en.store().count(iid) >= count;
			}
			default: {
				notef("bad op %u", op);
			}
		}
	}

	return true;
}