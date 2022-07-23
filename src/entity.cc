#include "common.h"
#include "item.h"
#include "entity.h"

#include <map>
#include <stdio.h>
#include <algorithm>

// The core of the Entity Component System. All entities have an Entity struct
// with a unique uint ID. Component structs store the ID to associate with an
// entity.

void Entity::reset() {
	for (Entity& en: all) en.destroy();
	all.clear();
	grid.clear();
	gridStores.clear();
	gridStoresLogistic.clear();
	gridGhosts.clear();
	gridDamaged.clear();
	gridConveyors.clear();
	gridPipes.clear();
	gridPipesChange.clear();
	gridEnemyTargets.clear();
	gridCartWaypoints.clear();
	enemyTargets.clear();
	repairActions.clear();
	removing.clear();
	exploding.clear();
	electricityGenerators.clear();
	names.clear();
	sequence = 0;
	Burner::reset();
	Generator::reset();
	Ghost::reset();
	Store::reset();
	Crafter::reset();
	Venter::reset();
	Effector::reset();
	Launcher::reset();
	Vehicle::reset();
	Cart::reset();
	CartStop::reset();
	CartWaypoint::reset();
	Arm::reset();
	Conveyor::reset();
	Unveyor::reset();
	Loader::reset();
	Balancer::reset();
	Pipe::reset();
	Drone::reset();
	Missile::reset();
	Explosion::reset();
	Depot::reset();
	Turret::reset();
	Computer::reset();
	Router::reset();
	Pile::reset();
	Explosive::reset();
	Networker::reset();
	Zeppelin::reset();
	FlightPad::reset();
	FlightPath::reset();
	FlightLogistic::reset();
	Tube::reset();
	Teleporter::reset();
	Monorail::reset();
	Monocar::reset();
	Source::reset();
	PowerPole::reset();
}

std::size_t Entity::memory() {
	return grid.memory()
		+ gridStores.memory()
		+ gridStoresLogistic.memory()
		+ gridGhosts.memory()
		+ gridDamaged.memory()
		+ gridConveyors.memory()
		+ gridPipes.memory()
		+ gridPipesChange.memory()
		+ gridEnemyTargets.memory()
		+ gridCartWaypoints.memory()
		+ gridFlightPaths.memory()
		+ all.memory()
	;
}

// First handler of every tick. At this stage it's safe to remove entities without
// considering other references or pointers to them.
void Entity::preTick() {
	ensure(mutating);

	for (auto id: exploding) {
		Entity& en = Entity::get(id);
		ensuref(en.spec->explodes, "%s cannot explode", en.spec->name);

		Entity& ex = Entity::create(Entity::next(), en.spec->explosionSpec);
		ex.explosion().define(ex.spec->explosionDamage, ex.spec->explosionRadius, ex.spec->explosionRate);
		ex.move(en.pos());
		ex.materialize();

		removing.insert(id);
	}
	exploding.clear();

	for (auto id: removing) {
		Entity::get(id).destroy();
	}
	removing.clear();

	// check drone repair flights valid
	minivec<uint> purge;
	for (auto [eid,did]: repairActions) {
		if (!exists(eid) || !exists(did)) {
			purge.push_back(eid);
		}
	}
	for (auto eid: purge) {
		repairActions.erase(eid);
	}

	// Updated by ::generate()
	electricitySupply = 0;
	electricityCapacity = 0;
	electricityCapacityBuffered = 0;
	electricityCapacityReady = 0;
	electricityCapacityBufferedReady = 0;
	electricityBufferedLevel = 0;
	electricityBufferedLimit = 0;

	for (auto& [_,spec]: Spec::all) {
		spec->statsGroup->energyConsumption.set(Sim::tick, 0);
		spec->statsGroup->energyGeneration.set(Sim::tick, 0);
	}

	// Generators adjust output based on the last tick's electricityLoad so there's
	// a slight delay in response to changes in consumption. This isn't unrealistic;
	// in fact it should probably lag for longer
	for (auto en: electricityGenerators) {
		if (en->isGhost()) continue;
		en->generate();
	}

	Charger::tickDischarge();

	electricityLoad = electricityDemand.portion(electricityCapacityReady);
	electricitySatisfaction = electricitySupply.portion(electricityDemand);
	electricityDemand = 0;
}

void Entity::postTick() {
	ensure(mutating);

	Charger::tickCharge();

	for (auto& [_,spec]: Spec::all) {
		spec->statsGroup->energyConsumption.update(Sim::tick);
		spec->statsGroup->energyGeneration.update(Sim::tick);
	}
}

uint Entity::next() {
	while (all.has(++sequence));
	return sequence;
}

Entity& Entity::create(uint id, Spec *spec) {
	ensure(mutating);

	ensure(!all.has(id));
	Entity& en = all[id];
	en.id = id;
	en.spec = spec;
	en.pos(Point::Zero);
	en.dir(Point::South);
	en.state = 0;
	en.health = spec->health;

	en.flags = GHOST | ENABLED;
	Ghost::create(id, Entity::next());

	spec->count.ghosts++;

	en.cache.conveyor = nullptr;

	if (spec->store) {
		Store::create(id, Entity::next(), spec->capacity);
	}

	if (spec->crafter) {
		Crafter::create(id);
	}

	if (spec->venter) {
		Venter::create(id);
	}

	if (spec->effector) {
		Effector::create(id);
	}

	if (spec->launcher) {
		Launcher::create(id);
	}

	if (spec->drone) {
		Drone::create(id);
	}

	if (spec->missile) {
		Missile::create(id);
	}

	if (spec->explosion) {
		Explosion::create(id);
	}

	if (spec->depot) {
		Depot::create(id);
	}

	if (spec->vehicle) {
		Vehicle::create(id);
	}

	if (spec->cart) {
		Cart::create(id);
	}

	if (spec->cartStop) {
		CartStop::create(id);
	}

	if (spec->cartWaypoint) {
		CartWaypoint::create(id);
	}

	if (spec->arm) {
		Arm::create(id);
	}

	if (spec->conveyor) {
		Conveyor::create(id);
	}

	if (spec->unveyor) {
		Unveyor::create(id);
	}

	if (spec->loader) {
		Loader::create(id);
	}

	if (spec->balancer) {
		Balancer::create(id);
	}

	if (spec->pipe) {
		Pipe::create(id);
	}

	if (spec->turret) {
		Turret::create(id);
	}

	if (spec->pile) {
		Pile::create(id);
	}

	if (spec->explosive) {
		Explosive::create(id);
	}

	if (spec->networker) {
		Networker::create(id);
	}

	if (spec->computer) {
		Computer::create(id);
	}

	if (spec->router) {
		Router::create(id);
	}

	if (spec->zeppelin) {
		Zeppelin::create(id);
	}

	if (spec->flightPad) {
		FlightPad::create(id);
	}

	if (spec->flightPath) {
		FlightPath::create(id);
	}

	if (spec->flightLogistic) {
		FlightLogistic::create(id);
	}

	if (spec->teleporter) {
		Teleporter::create(id);
	}

	if (spec->monorail) {
		Monorail::create(id);
	}

	if (spec->monocar) {
		Monocar::create(id);
	}

	if (spec->tube) {
		Tube::create(id);
	}

	if (spec->consumeFuel) {
		Burner::create(id, Entity::next(), spec->consumeFuelType);
	}

	if (spec->consumeCharge) {
		Charger::create(id);
	}

	if (spec->consumeThermalFluid) {
		Generator::create(id, Entity::next());
	}

	if (spec->source) {
		Source::create(id);
	}

	if (spec->powerpole) {
		PowerPole::create(id);
	}

	if (spec->generateElectricity) {
		electricityGenerators.insert(&en);
		en.setGenerating(true);
	}

	if (spec->named) {
		names[id] = fmt("%s %u", spec->name, id);
	}

	if (spec->enemyTarget) {
		enemyTargets.insert(id);
	}

	return en;
}

void Entity::destroy() {
	ensure(mutating);

	if (isGhost())
		spec->count.ghosts--;

	if (!isGhost())
		spec->count.extant--;

	unmanage();
	unindex();

	if (isGhost()) {
		ghost().destroy();
	}

	if (spec->store) {
		store().destroy();
	}

	if (spec->crafter) {
		crafter().destroy();
	}

	if (spec->venter) {
		venter().destroy();
	}

	if (spec->effector) {
		effector().destroy();
	}

	if (spec->launcher) {
		launcher().destroy();
	}

	if (spec->drone) {
		drone().destroy();
	}

	if (spec->missile) {
		missile().destroy();
	}

	if (spec->explosion) {
		explosion().destroy();
	}

	if (spec->depot) {
		depot().destroy();
	}

	if (spec->arm) {
		arm().destroy();
	}

	if (spec->vehicle) {
		vehicle().destroy();
	}

	if (spec->cart) {
		cart().destroy();
	}

	if (spec->cartStop) {
		cartStop().destroy();
	}

	if (spec->cartWaypoint) {
		cartWaypoint().destroy();
	}

	if (spec->conveyor) {
		conveyor().destroy();
	}

	if (spec->unveyor) {
		unveyor().destroy();
	}

	if (spec->loader) {
		loader().destroy();
	}

	if (spec->balancer) {
		balancer().destroy();
	}

	if (spec->pipe) {
		pipe().destroy();
	}

	if (spec->turret) {
		turret().destroy();
	}

	if (spec->computer) {
		computer().destroy();
	}

	if (spec->router) {
		router().destroy();
	}

	if (spec->pile) {
		pile().destroy();
	}

	if (spec->explosive) {
		explosive().destroy();
	}

	if (spec->networker) {
		networker().destroy();
	}

	if (spec->zeppelin) {
		zeppelin().destroy();
	}

	if (spec->flightPad) {
		flightPad().destroy();
	}

	if (spec->flightPath) {
		flightPath().destroy();
	}

	if (spec->flightLogistic) {
		flightLogistic().destroy();
	}

	if (spec->teleporter) {
		teleporter().destroy();
	}

	if (spec->monorail) {
		monorail().destroy();
	}

	if (spec->monocar) {
		monocar().destroy();
	}

	if (spec->tube) {
		tube().destroy();
	}

	if (spec->consumeFuel) {
		burner().destroy();
	}

	if (spec->consumeCharge) {
		charger().destroy();
	}

	if (spec->consumeThermalFluid) {
		generator().destroy();
	}

	if (spec->source) {
		source().destroy();
	}

	if (spec->powerpole) {
		powerpole().destroy();
	}

	if (spec->generateElectricity) {
		electricityGenerators.erase(this);
	}

	if (spec->enemyTarget) {
		enemyTargets.erase(id);
	}

	names.erase(id);
	all.erase(id);
}

Entity::Settings::Settings() {
}

Entity::Settings::Settings(Entity& en) : Settings() {
	enabled = en.isEnabled();
	color = en.color();

	if (en.spec->store) {
		store = en.store().settings();
	}

	if (en.spec->crafter) {
		crafter = en.crafter().settings();
	}

	if (en.spec->venter) {
	}

	if (en.spec->effector) {
	}

	if (en.spec->launcher) {
	}

	if (en.spec->drone) {
	}

	if (en.spec->missile) {
	}

	if (en.spec->explosion) {
	}

	if (en.spec->depot) {
	}

	if (en.spec->arm) {
		arm = en.arm().settings();
	}

	if (en.spec->vehicle) {
	}

	if (en.spec->cart) {
		cart = en.cart().settings();
	}

	if (en.spec->cartStop) {
		cartStop = en.cartStop().settings();
	}

	if (en.spec->cartWaypoint) {
		cartWaypoint = en.cartWaypoint().settings();
	}

	if (en.spec->conveyor) {
	}

	if (en.spec->unveyor) {
	}

	if (en.spec->loader) {
		loader = en.loader().settings();
	}

	if (en.spec->balancer) {
		balancer = en.balancer().settings();
	}

	if (en.spec->pipe) {
		pipe = en.pipe().settings();
	}

	if (en.spec->turret) {
	}

	if (en.spec->computer) {
	}

	if (en.spec->router) {
		router = en.router().settings();
	}

	if (en.spec->networker) {
		networker = en.networker().settings();
	}

	if (en.spec->zeppelin) {
	}

	if (en.spec->flightPad) {
	}

	if (en.spec->flightPath) {
	}

	if (en.spec->flightLogistic) {
	}

	if (en.spec->teleporter) {
	}

	if (en.spec->monorail) {
		monorail = en.monorail().settings();
	}

	if (en.spec->monocar) {
	}

	if (en.spec->tube) {
		tube = en.tube().settings();
	}

	if (en.spec->monorail) {
		monorail = en.monorail().settings();
	}

	if (en.spec->source) {
	}

	if (en.spec->powerpole) {
	}
}

Entity::Settings::~Settings() {
	if (store) delete store;
	if (crafter) delete crafter;
	if (arm) delete arm;
	if (loader) delete loader;
	if (balancer) delete balancer;
	if (pipe) delete pipe;
	if (cart) delete cart;
	if (cartStop) delete cartStop;
	if (cartWaypoint) delete cartWaypoint;
	if (networker) delete networker;
	if (tube) delete tube;
	if (monorail) delete monorail;
	if (router) delete router;
}

Entity::Settings* Entity::settings() {
	return new Settings(*this);
}

void Entity::setup(Entity::Settings* settings) {
	setEnabled(settings->enabled);
	color(settings->color);

	if (spec->store && settings->store) {
		store().setup(settings->store);
	}

	if (spec->crafter && settings->crafter) {
		crafter().setup(settings->crafter);
	}

	if (spec->venter) {
	}

	if (spec->effector) {
	}

	if (spec->launcher) {
	}

	if (spec->drone) {
	}

	if (spec->missile) {
	}

	if (spec->explosion) {
	}

	if (spec->depot) {
	}

	if (spec->arm && settings->arm) {
		arm().setup(settings->arm);
	}

	if (spec->vehicle) {
	}

	if (spec->cart && settings->cart) {
		cart().setup(settings->cart);
	}

	if (spec->cartStop && settings->cartStop) {
		cartStop().setup(settings->cartStop);
	}

	if (spec->cartWaypoint && settings->cartWaypoint) {
		cartWaypoint().setup(settings->cartWaypoint);
	}

	if (spec->conveyor) {
	}

	if (spec->unveyor) {
	}

	if (spec->loader && settings->loader) {
		loader().setup(settings->loader);
	}

	if (spec->balancer && settings->balancer) {
		balancer().setup(settings->balancer);
	}

	if (spec->pipe && settings->pipe) {
		pipe().setup(settings->pipe);
	}

	if (spec->turret) {
	}

	if (spec->computer) {
	}

	if (spec->router && settings->router) {
		router().setup(settings->router);
	}

	if (spec->networker && settings->networker) {
		networker().setup(settings->networker);
	}

	if (spec->zeppelin) {
	}

	if (spec->flightPad) {
	}

	if (spec->flightPath) {
	}

	if (spec->flightLogistic) {
	}

	if (spec->teleporter) {
	}

	if (spec->monorail && settings->monorail) {
		monorail().setup(settings->monorail);
	}

	if (spec->monocar) {
	}

	if (spec->tube && settings->tube) {
		tube().setup(settings->tube);
	}

	if (spec->monorail && settings->monorail) {
		monorail().setup(settings->monorail);
	}

	if (spec->source) {
	}

	if (spec->powerpole) {
	}
}

bool Entity::exists(uint id) {
	return id > 0 && all.has(id);
}

Entity& Entity::get(uint id) {
	return all.refer(id);
}

Entity* Entity::find(uint id) {
	return id ? all.point(id): nullptr;
}

bool Entity::fits(Spec *spec, Point pos, Point dir) {
	Box bounds = spec->box(pos, dir, spec->collision).shrink(0.01);
	bounds.h = std::max(bounds.h, 0.1);

	for (auto es: intersecting(bounds)) {
		if (es->spec->junk) continue;
		if (es->isDeconstruction()) continue;
		if (es->spec->slab && !spec->slab) continue;
		if (!es->spec->slab && spec->slab) continue;
		if (es->spec->vehicleStop && spec->vehicle) continue;
		if (es->spec->cartWaypoint && spec->cart) continue;
		if (es->spec->flightPad && spec->flightPath) continue;
		if (es->spec->monorail && spec->monocar) continue;
		if (es->spec->monocar && spec->monocar) return false;
		if (es->spec->collideBuild) return false;
	}

	if (spec->place & Spec::Monorail) {
		for (auto em: intersecting((pos+(Point::Down*spec->collision.h)).box().grow(0.1))) {
			if (em->spec->monorail) return true;
		}
	}

	if (spec->place != Spec::Footings) {

		if (spec->place & Spec::Hill && spec->placeOnHill && world.isHill(pos)) {
			Box hbox = spec->placeOnHillBox(pos);
			if ((pos.y-(spec->collision.h/2.0)+1.1) < world.hillPlatform(hbox)) return false;
		}

		for (auto at: world.walk(bounds)) {
			bool ok = false;
			Point p = Point((float)at.x+0.5f, 0, (float)at.y+0.5f);
			ok = ok || ((spec->place & Spec::Land) && isLand(p));
			ok = ok || ((spec->place & Spec::Hill) && world.isHill(at));
			ok = ok || ((spec->place & Spec::Water) && (world.isLake(at) && !isLand(p)));
			if (!ok) return false;
		}
		return true;
	}

	for (auto& footing: spec->footings) {
		Point point = footing.point.transform(dir.rotation()) + pos;
		World::XY at = {(int)std::floor(point.x), (int)std::floor(point.z)};
		bool ok = false;
		ok = ok || ((footing.place & Spec::Land) && isLand(point));
		ok = ok || ((footing.place & Spec::Hill) && world.isHill(at));
		ok = ok || ((footing.place & Spec::Water) && (world.isLake(at) && !isLand(point)));
		if (!ok) return false;
	}

	return true;
}

std::vector<Entity*> Entity::intersecting(const Cuboid& cuboid) {
	return intersecting(cuboid, grid);
}

std::vector<Entity*> Entity::intersecting(const Box& box) {
	return intersecting(box, grid);
}

std::vector<Entity*> Entity::intersecting(const Sphere& sphere) {
	return intersecting(sphere, grid);
}

std::vector<Entity*> Entity::intersecting(const Cylinder& cylinder) {
	return intersecting(cylinder, grid);
}

std::vector<Entity*> Entity::intersecting(Point pos, float radius) {
	return intersecting(Sphere(pos, radius));
}

Entity* Entity::at(Point p) {
	return at(p, grid);
}

Entity* Entity::at(Point p, gridmap<GRID,Entity*>& gm) {
	auto hits = intersecting(p.box(), gm);
	return hits.size() ? hits.front(): 0;
}

bool Entity::isLand(Point p) {
	return isLand(p.box().grow(0.1f));
}

bool Entity::isLand(Box b) {
	if (world.isLand(b)) return true;

	std::vector<Entity*> piles;
	for (auto en: intersecting(b)) {
		if (en->spec->pile && !en->isGhost()) {
			piles.push_back(en);
		}
	}
	for (auto [x,y]: world.walk(b)) {
		Box b2 = Point((float)x+0.5f,-0.5f,(float)y+0.5f).box().grow(0.1f);
		for (auto en: piles) {
			if (en->box().intersects(b2)) {
				return true;
			}
		}
	}
	return false;
}

void Entity::upgradeCascade(uint from) {
	if (!exists(from)) return;
	auto& en = get(from);
	if (en.spec->upgradeCascade.empty()) return;

	if (en.spec->tube) {
		for (auto cid: Tube::upgradableGroup(en.id)) {
			get(cid).upgrade();
		}
	}
	else
	if (en.spec->conveyor) {
		for (auto cid: Conveyor::upgradableGroup(en.id)) {
			get(cid).upgrade();
		}
	}
}

std::vector<Entity*> Entity::enemiesInRange(Point pos, float radius) {
	float radiusSquared = radius*radius;
	std::vector<Entity*>hits;
	for (auto& missile: Missile::all) {
		Entity& me = Entity::get(missile.id);
		float de = me.pos().distanceSquared(pos);
		if (de < radiusSquared) hits.push_back(&me);
	}
	std::sort(hits.begin(), hits.end(), [&](auto a, auto b) {
		return a->pos().distanceSquared(pos) < b->pos().distanceSquared(pos);
	});
	return hits;
}

void Entity::remove() {
	removing.insert(id);
}

void Entity::explode() {
	exploding.insert(id);
}

Point Entity::pos() const {
	return {
		(real)(((double)_pos.x) / 1000.0),
		(real)(((double)_pos.y) / 1000.0),
		(real)(((double)_pos.z) / 1000.0),
	};
}

Point Entity::pos(Point p) {
	_pos.x = (int)std::round(((double)p.x) * 1000.0);
	_pos.y = (int)std::round(((double)p.y) * 1000.0);
	_pos.z = (int)std::round(((double)p.z) * 1000.0);
	return pos();
}

Point Entity::dir() const {
	return {_dir.x, _dir.y, _dir.z};
}

Point Entity::dir(Point p) {
	_dir.x = (float)p.x;
	_dir.y = (float)p.y;
	_dir.z = (float)p.z;
	return dir();
}

bool Entity::isGhost() const {
	return (flags & GHOST) != 0;
}

Entity& Entity::setGhost(bool state) {
	ensure(mutating);
	flags = state ? (flags | GHOST) : (flags & ~GHOST);
	return *this;
}

bool Entity::isConstruction() const {
	return (flags & CONSTRUCTION) != 0;
}

Entity& Entity::setConstruction(bool state) {
	ensure(mutating);
	flags = state ? (flags | CONSTRUCTION) : (flags & ~CONSTRUCTION);
	return *this;
}

bool Entity::isDeconstruction() const {
	return (flags & DECONSTRUCTION) != 0;
}

Entity& Entity::setDeconstruction(bool state) {
	ensure(mutating);
	flags = state ? (flags | DECONSTRUCTION) : (flags & ~DECONSTRUCTION);
	return *this;
}

bool Entity::isEnabled() const {
	if (!spec->enable) return true;
	return (flags & ENABLED) != 0;
}

Entity& Entity::setEnabled(bool state) {
	ensure(mutating);
	if (spec->enable)
		flags = state ? (flags | ENABLED) : (flags & ~ENABLED);
	return *this;
}

bool Entity::isBlocked() const {
	return (flags & BLOCKED) != 0;
}

Entity& Entity::setBlocked(bool state) {
	ensure(mutating);
	flags = state ? (flags | BLOCKED) : (flags & ~BLOCKED);
	return *this;
}

bool Entity::isGenerating() const {
	return (flags & GENERATING) != 0;
}

Entity& Entity::setGenerating(bool state) {
	flags = state ? (flags | GENERATING) : (flags & ~GENERATING);
	return *this;
}

bool Entity::isRuled() const {
	return (flags & RULED) != 0;
}

Entity& Entity::setRuled(bool state) {
	flags = state ? (flags | RULED) : (flags & ~RULED);
	return *this;
}

bool Entity::isMarked1() const {
	return (flags & MARKED1) != 0;
}

Entity& Entity::setMarked1(bool state) {
	flags = state ? (flags | MARKED1) : (flags & ~MARKED1);
	return *this;
}

bool Entity::isMarked2() const {
	return (flags & MARKED2) != 0;
}

Entity& Entity::setMarked2(bool state) {
	flags = state ? (flags | MARKED2) : (flags & ~MARKED2);
	return *this;
}

Entity& Entity::clearMarks() {
	flags &= ~(MARKED1|MARKED2);
	return *this;
}

std::string Entity::name() const {
	return spec->named ? names[id]: fmt("%s %u", spec->name, id);
}

std::string Entity::title() const {
	return spec->named ? names[id]: spec->title;
}

bool Entity::rename(std::string name) {
	ensure(mutating);
	if (spec->named) {
		names[id] = name;
		return true;
	}
	return false;
}

Color Entity::color() const {
	return spec->coloredCustom && colors.count(id) ? colors[id]: spec->color;
}

Color Entity::color(Color c) {
	if (spec->coloredCustom) colors[id] = c;
	return color();
}

Box Entity::box() const {
	return spec->box(pos(), dir(), spec->collision);
}

Sphere Entity::sphere() const {
	return box().sphere();
}

Box Entity::miningBox() const {
	return box().grow(0.5f);
}

Box Entity::drillingBox() const {
	return box().grow(0.5f);
}

Cuboid Entity::cuboid() const {
	return Cuboid(spec->southBox(pos(), spec->collision), dir());
}

Cuboid Entity::selectionCuboid() const {
	return Cuboid(spec->southBox(pos(), spec->selection), dir());
}

Entity& Entity::look(Point p) {
	ensure(mutating);
	move(pos(), p);
	return *this;
}

Entity& Entity::lookAt(Point p) {
	look(p-pos());
	return *this;
}

bool Entity::lookAtPivot(Point o, float speed) {
	if (o == pos()) return true;
	o = (o-pos()).normalize();
	Point d = dir().pivot(o, speed);
	look(d);
	return d == o;
}

bool Entity::lookingAt(Point o) {
	if (o == pos()) return true;
	o = (o-pos()).normalize();
	return o == dir();
}

bool Entity::specialIndexable() {
	return spec->store
		|| spec->consumeFuel
		|| spec->health
		|| spec->conveyor
		|| spec->pipe
		|| spec->enemyTarget
		|| spec->cartWaypoint
		|| spec->flightPath
		|| spec->tube;
}

Entity& Entity::index() {
	ensure(mutating);

	Box aabb = box();
	if (!spec->align) {
		// AABB of collision box (and cuboid) in any rotation
		aabb = aabb.sphere().box();
	}

	grid.insert(aabb, this);
	gridRender.insert(aabb, this);

	if (spec->store) {
		gridStores.insert(aabb, this);
	}

	if (spec->store && spec->logistic) {
		gridStoresLogistic.insert(aabb, this);
	}

	if (spec->consumeFuel) {
		gridStoresFuel.insert(aabb, this);
	}

	if (isGhost()) {
		gridGhosts.insert(aabb, this);
	}

	if (spec->health && health < spec->health) {
		gridDamaged.insert(aabb, this);
	}

	if (spec->conveyor) {
		gridConveyors.insert(aabb, this);
	}

	if (spec->pipe) {
		gridPipes.insert(aabb, this);
		gridPipesChange.set(aabb, Sim::tick);
	}

	if (spec->enemyTarget) {
		gridEnemyTargets.insert(aabb, this);
	}

	if (spec->cartWaypoint) {
		gridCartWaypoints.insert(aabb, this);
	}

	if (spec->flightPath) {
		gridFlightPaths.insert(aabb, this);
	}

	if (spec->tube) {
		gridTubes.insert(aabb, this);
		gridRenderTubes.insert(aabb, this);
	}

	if (spec->monorail) {
		gridRenderMonorails.insert(aabb, this);
	}

	if (spec->slab) {
		gridSlabs.insert(aabb, this);
	}

	if (!isGhost() && !spec->junk && !spec->enemy && spec->health && health < spec->health) {
		damaged.insert(id);
	}

	return *this;
}

Entity& Entity::unindex() {
	ensure(mutating);

	Box aabb = box();
	if (!spec->align) {
		// AABB of collision box (and cuboid) in any rotation
		aabb = aabb.sphere().box();
	}

	grid.remove(aabb, this);
	gridRender.remove(aabb, this);

	if (spec->store) {
		gridStores.remove(aabb, this);
	}

	if (spec->store && spec->logistic) {
		gridStoresLogistic.remove(aabb, this);
	}

	if (spec->consumeFuel) {
		gridStoresFuel.remove(aabb, this);
	}

	if (isGhost()) {
		gridGhosts.remove(aabb, this);
	}

	if (spec->health) {
		gridDamaged.remove(aabb, this);
	}

	if (spec->conveyor) {
		gridConveyors.remove(aabb, this);
	}

	if (spec->pipe) {
		gridPipes.remove(aabb, this);
		gridPipesChange.set(aabb, Sim::tick);
	}

	if (spec->enemyTarget) {
		gridEnemyTargets.remove(aabb, this);
	}

	if (spec->cartWaypoint) {
		gridCartWaypoints.remove(aabb, this);
	}

	if (spec->flightPath) {
		gridFlightPaths.remove(aabb, this);
	}

	if (spec->tube) {
		gridTubes.remove(aabb, this);
		gridRenderTubes.remove(aabb, this);
	}

	if (spec->monorail) {
		gridRenderMonorails.remove(aabb, this);
	}

	if (spec->slab) {
		gridSlabs.remove(aabb, this);
	}

	damaged.erase(id);

	return *this;
}

Entity& Entity::manage() {
	ensure(mutating);

	if (!isGhost()) {
		if (spec->conveyor) {
			conveyor().manage();
		}
		if (spec->unveyor) {
			unveyor().manage();
		}
		if (spec->balancer) {
			balancer().manage();
		}
		if (spec->pipe) {
			pipe().manage();
		}
		if (spec->networker) {
			networker().manage();
		}
		if (spec->powerpole) {
			powerpole().manage();
		}
	}
	return *this;
}

Entity& Entity::unmanage() {
	ensure(mutating);
	if (!isGhost()) {
		if (spec->conveyor) {
			conveyor().unmanage();
		}
		if (spec->unveyor) {
			unveyor().unmanage();
		}
		if (spec->balancer) {
			balancer().unmanage();
		}
		if (spec->pipe) {
			pipe().unmanage();
		}
		if (spec->networker) {
			networker().unmanage();
		}
		if (spec->powerpole) {
			powerpole().unmanage();
		}
	}
	return *this;
}

Entity& Entity::upgrade() {
	ensure(mutating);
	if (isGhost()) return *this;
	if (!spec->upgrade) return *this;
	if (!spec->upgrade->licensed) return *this;

	auto cfg = settings();

	Entity& eu = create(next(), spec->upgrade);
	eu.move(pos(), dir());
	eu.construct();

	float height = std::max(0.0f, world.elevation(pos()));

	Store& gstore = eu.ghost().store;

	for (Stack stack: spec->constructionMaterials(height)) {
		auto level = gstore.level(stack.iid);
		// upgrade spec may not be a superset of this spec's construction materials
		if (level) gstore.insert({stack.iid, std::min(stack.size, level->lower)});
	}

	eu.setup(cfg);
	delete cfg;

	if (spec->store && eu.spec->store) {
		// preserving levels across upgrade seems to be always preferred?
		// containers will already do this via setup() but crafters won't
		// as they only preserve the recipe
		eu.store().levels = store().levels;
	}

	if (spec->conveyor) {
		eu.conveyor().upgrade(id);
	}

	if (spec->tube) {
		eu.tube().upgrade(id);
	}

	if (spec->store && spec->storeUpgradePreserve) {
		eu.store().stacks = store().stacks;
	}

	remove();
	return *this;
}

Entity& Entity::construct() {
	ensure(mutating);
	unindex();
	unmanage();

	bool wasGhost = isGhost();

	if (!wasGhost) {
		Ghost::create(id, Entity::next());
		spec->count.extant--;
		spec->count.ghosts++;
	}

	setGhost(true);
	setConstruction(true);
	setDeconstruction(false);

	float height = std::max(0.0f, world.elevation(pos()));

	Store& gstore = ghost().store;
	gstore.levels.clear();

	for (auto& stack: spec->constructionMaterials(height)) {
		gstore.levelSet(stack.iid, stack.size, stack.size);
	}

	index();
	return *this;
}

Entity& Entity::deconstruct(bool items) {
	ensure(mutating);
	unindex();
	unmanage();

	bool wasGhost = isGhost();

	if (!wasGhost) {
		Ghost::create(id, Entity::next());
		spec->count.extant--;
		spec->count.ghosts++;
	}

	setGhost(true);
	setConstruction(false);
	setDeconstruction(true);

	float height = std::max(0.0f, world.elevation(pos()));

	Store& gstore = ghost().store;

	if (!wasGhost) {
		for (auto& stack: spec->constructionMaterials(height)) {
			gstore.insert(stack);
		}
	}

	if (items) {
		if (spec->store) {
			for (auto& stack: store().stacks) {
				gstore.insert(stack);
			}
			store().stacks.clear();
		}

		if (spec->conveyor) {
			for (auto iid: conveyor().items()) {
				gstore.insert({iid,1});
			}
		}
	}

	// When a game is loaded, the old ghost contents may differ from an
	// updated spec's material. So apply all levels for all known contents

	for (auto& stack: gstore.stacks) {
		gstore.levelSet(stack.iid, 0, 0);
	}

	for (auto& stack: spec->constructionMaterials(height)) {
		gstore.levelSet(stack.iid, 0, 0);
	}

	index();
	return *this;
}

Entity& Entity::materialize() {
	ensure(mutating);
	unindex();
	ghost().destroy();
	spec->count.extant++;
	spec->count.ghosts--;
	setGhost(false);
	setConstruction(false);
	setDeconstruction(false);
	index();
	manage();
	return *this;
}

Entity& Entity::complete() {
	ensure(mutating);
	ensure(isGhost());
	auto& gstore = ghost().store;

	// if this is an upgrade the net construction materials required may
	// not match the spec if some materials were carried over from the old
	// entity, so assume the ghost levels are authoritative
	for (auto& level: gstore.levels) {
		Item::get(level.iid)->consume(level.lower);
		gstore.remove({level.iid, level.lower});
	}

	if (spec->store) {
		// anything left in the ghost store after construction is due to
		// and upgrade preserving the contents of the old entity
		for (auto& stack: gstore.stacks) {
			store().insert(stack);
		}
	}

	if (spec->store && spec->supplies.size()) {
		for (auto& stack: spec->supplies) {
			store().insert(stack);
		}
	}

	gstore.stacks.clear();
	gstore.levels.clear();
	return *this;
}

Entity& Entity::move(Point p) {
	move(p, dir());
//	ensure(mutating);
//	unmanage();
//	unindex();
//	pos(spec->aligned(p, dir()));
//	index();
//	manage();
	return *this;
}

Entity& Entity::move(Point p, Point d) {
	ensure(mutating);

	if (spec->drone) {
		ensure(!specialIndexable());
		auto oldBox = box().sphere().box();
		pos(spec->aligned(p, d));
		dir(d.normalize());
		auto newBox = box().sphere().box();
		grid.update(oldBox, newBox, this);
		gridRender.update(oldBox, newBox, this);
		if (isGhost())
			gridGhosts.update(oldBox, newBox, this);
		return *this;
	}

	unmanage();
	unindex();
	pos(spec->aligned(p, d));
	dir(d.normalize());
	index();
	manage();
	return *this;
}

Entity& Entity::move(float x, float y, float z) {
	return move(Point(x, y, z));
}

Entity& Entity::bump(Point p, Point d) {
	ensure(mutating);

	if (!spec->monorailContainer) return move(p, d);

	unmanage();
	unindex();
	pos(p);
	dir(d.normalize());
	index();
	manage();
	return *this;
}

Point Entity::ground() const {
	return {pos().x, pos().y + (spec->underground ? spec->collision.h/2.0f: -spec->collision.h/2.0f), pos().z};
}

Sphere Entity::groundSphere() const {
	return ground().sphere().grow(std::max(spec->collision.w, spec->collision.d)/2.0f);
}

Entity& Entity::rotate() {
	if (spec->rotateExtant) {
		for (int i = 0, l = spec->rotations.size()-1; i < l; i++) {
			if (spec->rotations[i] == dir()) {
				move(pos(), spec->rotations[i+1]);
				return *this;
			}
		}
		move(pos(), spec->rotations[0]);
	}
	return *this;
}

Energy Entity::consume(Energy e) {
	ensure(mutating);
	Energy c = 0;
	if (!isEnabled()) return c;

	if (spec->consumeElectricity) {
		electricityDemand += e;
		c = e * electricitySatisfaction;
	}
	if (spec->consumeFuel) {
		c = burner().consume(e);
	}
	if (spec->consumeCharge) {
		c = charger().consume(e);
	}
	if (spec->consumeThermalFluid) {
		c = generator().consume(e);
	}
	if (spec->consumeMagic) {
		c = e;
	}
	// chargers add a level of indirection to electricity
	// consumption and manage consumption tracking directly
	if (!spec->consumeCharge) {
		spec->statsGroup->energyConsumption.add(Sim::tick, c);
	}
	return c;
}

float Entity::consumeRate(Energy e) {
	return consume(e).portion(e);
}

// For numerous simple components like conveyors it's faster to consume energy once per
// tick rather than hammer the postTick() energy consumption/drain checks
void Entity::bulkConsumeElectricity(Spec* spec, Energy e, int count) {
	e = (e * (float)count);
	if (spec->consumeElectricity) {
		electricityDemand += e;
		spec->statsGroup->energyConsumption.add(Sim::tick, e);
	}
}

void Entity::generate() {
	ensure(mutating);
	if (!isEnabled() || !isGenerating()) return;

	// At the start of the game when a single generator exists, or when the electricity network
	// fuel supply crashes and generators need to restart, load must always be slightly > 0.
	Energy energy = std::max(Energy::J(1), spec->energyGenerate * electricityLoad);

	if (spec->generateElectricity && spec->consumeFuel) {
		auto& burn = burner();

		Energy supplied = burn.consume(energy);
		spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
		electricitySupply += supplied;

		if (burn.energy) electricityCapacityReady += spec->energyGenerate;
		electricityCapacity += spec->energyGenerate;

		if (spec->burnerState) {
			// Burner state is a smooth progression between 0 and #spec->states
			state = std::floor((float)burn.energy.value/(float)burn.buffer.value * (float)spec->states.size());
			state = std::min((uint16_t)spec->states.size(), std::max((uint16_t)1, state)) - 1;
		}
		return;
	}

	if (spec->generateElectricity && spec->consumeThermalFluid) {
		auto& gen = generator();

		Energy supplied = gen.consume(energy);
		spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
		electricitySupply += supplied;

		if (gen.supplying) electricityCapacityReady += spec->energyGenerate;
		electricityCapacity += spec->energyGenerate;

		// The state for generators is currently stopped, slow or fast. The steam-engine entity
		// uses state to spin its flywheel albeit jerkily. This should be done in a smoother
		// fashion someday
		if (spec->generatorState && supplied) {
			state += supplied > (spec->energyGenerate * 0.5f) ? 2: 1;
			if (state >= spec->states.size()) state -= spec->states.size();
		}

		return;
	}

	if (spec->windTurbine) {
		float wind = Sim::windSpeed({pos().x, pos().y-(spec->collision.h/2.0f), pos().z});

		Energy supplied = spec->energyGenerate * wind;
		spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
		electricitySupply += supplied;

		if (wind > 0.0f) {
			state += std::ceil(wind/10.0f);
			if (state >= spec->states.size()) state -= spec->states.size();

			electricityCapacityReady += supplied;
			electricityCapacity += supplied;
		}

		return;
	}

	if (spec->generateElectricity && spec->consumeMagic) {
		Energy supplied = energy;
		spec->statsGroup->energyGeneration.add(Sim::tick, supplied);
		electricitySupply += supplied;
		electricityCapacityReady += spec->energyGenerate;
		electricityCapacity += spec->energyGenerate;
		return;
	}
}

void Entity::damage(Health hits) {
	ensure(mutating);
	if (isGhost()) return;
	if (!spec->health) return;
	if (spec->explosion) return;
	if (exploding.has(id)) return;
	if (removing.has(id)) return;

	unindex();

	health = std::max(0, health-hits);

	if (!health) {
		if (!spec->junk && !spec->enemy) {
			auto cfg = settings();
			auto& replace = create(next(), spec);
			replace.move(pos(), dir());
			replace.setup(cfg);
			replace.construct();
			delete cfg;
		}
		if (spec->explodes) explode();
		else remove();
	}

	index();
}

void Entity::repair(Health hits) {
	ensure(mutating);
	if (isGhost()) return;
	if (!spec->health) return;
	if (spec->explosion) return;
	if (exploding.has(id)) return;
	if (removing.has(id)) return;

	unindex();

	health = std::min(spec->health, health+hits);

	index();
}

Ghost& Entity::ghost() const {
	return Ghost::get(id);
}

Store& Entity::store() const {
	return Store::get(id);
}

std::vector<Store*> Entity::stores() const {
	std::vector<Store*> stores;
	if (isGhost()) {
		stores.push_back(&ghost().store);
	}
	if (spec->consumeFuel) {
		stores.push_back(&burner().store);
	}
	if (spec->store) {
		stores.push_back(&store());
	}
	return stores;
}

Crafter& Entity::crafter() const {
	return Crafter::get(id);
}

Venter& Entity::venter() const {
	return Venter::get(id);
}

Effector& Entity::effector() const {
	return Effector::get(id);
}

Launcher& Entity::launcher() const {
	return Launcher::get(id);
}

Vehicle& Entity::vehicle() const {
	return Vehicle::get(id);
}

Cart& Entity::cart() const {
	return Cart::get(id);
}

CartStop& Entity::cartStop() const {
	return CartStop::get(id);
}

CartWaypoint& Entity::cartWaypoint() const {
	return CartWaypoint::get(id);
}

Arm& Entity::arm() const {
	return Arm::get(id);
}

Conveyor& Entity::conveyor() const {
	ensure(cache.conveyor);
	return *cache.conveyor;
}

Unveyor& Entity::unveyor() const {
	return Unveyor::get(id);
}

Loader& Entity::loader() const {
	return Loader::get(id);
}

Balancer& Entity::balancer() const {
	return Balancer::get(id);
}

Pipe& Entity::pipe() const {
	return Pipe::get(id);
}

Drone& Entity::drone() const {
	return Drone::get(id);
}

Missile& Entity::missile() const {
	return Missile::get(id);
}

Explosion& Entity::explosion() const {
	return Explosion::get(id);
}

Depot& Entity::depot() const {
	return Depot::get(id);
}

Burner& Entity::burner() const {
	return Burner::get(id);
}

Charger& Entity::charger() const {
	return Charger::get(id);
}

Generator& Entity::generator() const {
	return Generator::get(id);
}

Turret& Entity::turret() const {
	return Turret::get(id);
}

Computer& Entity::computer() const {
	return Computer::get(id);
}

Router& Entity::router() const {
	return Router::get(id);
}

Pile& Entity::pile() const {
	return Pile::get(id);
}

Explosive& Entity::explosive() const {
	return Explosive::get(id);
}

Networker& Entity::networker() const {
	return Networker::get(id);
}

Zeppelin& Entity::zeppelin() const {
	return Zeppelin::get(id);
}

FlightPad& Entity::flightPad() const {
	return FlightPad::get(id);
}

FlightPath& Entity::flightPath() const {
	return FlightPath::get(id);
}

FlightLogistic& Entity::flightLogistic() const {
	return FlightLogistic::get(id);
}

Teleporter& Entity::teleporter() const {
	return Teleporter::get(id);
}

Monorail& Entity::monorail() const {
	return Monorail::get(id);
}

Monocar& Entity::monocar() const {
	return Monocar::get(id);
}

Tube& Entity::tube() const {
	return Tube::get(id);
}

Source& Entity::source() const {
	return Source::get(id);
}

PowerPole& Entity::powerpole() const {
	return PowerPole::get(id);
}

