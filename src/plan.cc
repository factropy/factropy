#include "common.h"
#include "plan.h"
#include "scene.h"
#include "glm-ex.h"

// A plan is a blueprint of entities including layout and configuration.
// A single-entity pipette is a blueprint without configuration.

void Plan::reset() {
	for (auto plan: std::vector<Plan*>{all.begin(), all.end()}) delete plan;
	ensure(!all.size());
}

void Plan::gc() {
	miniset<Plan*> drop;

	for (auto plan: all) {
		if (plan->save) continue;
		if (plan == clipboard) continue;
		if (plan == scene.placing) continue;
		drop.insert(plan);
	}

	for (auto plan: drop) delete plan;
}

Plan* Plan::find(uint id) {
	for (auto plan: all) {
		if (plan->id == id) return plan;
	}
	return nullptr;
}

Plan::Plan() {
	all.insert(this);
	id = ++sequence;
	position = Point::Zero;
	config = false;
	save = false;
	clipboard = this;
}

Plan::Plan(Point p) : Plan() {
	position = Point(std::floor(p.x), 0.0f, std::floor(p.z));
}

Plan::~Plan() {
	for (auto te: entities) delete te;
	entities.clear();
	all.erase(this);
}

void Plan::add(GuiFakeEntity* ge) {
	entities.push_back(ge);
	Point offset = ge->pos() - position;
	offsets.push_back(offset);
}

void Plan::move(Point p) {
	if (entities.size() == 1 && !entities[0]->spec->align) {
		position = p;
	}

	if (entities.size() > 1 || entities[0]->spec->align) {
		position = Point(std::floor(p.x), std::floor(p.y), std::floor(p.z));
	}

	if (entities.size() == 1 && entities[0]->spec->placeOnHill) {
		offsets[0] = Point(0, entities[0]->spec->collision.h/2.0f, 0);
	}

	if (entities.size() == 1 && entities[0]->spec->placeOnWaterSurface) {
		// Spec::aligned enforces p.y, so offset must compensate
		offsets[0] = Point(0, 3.0f, 0);
	}

	for (uint i = 0; i < entities.size(); i++) {
		auto te = entities[i];
		te->move(position + offsets[i]);
	}

	for (uint i = 0; i < entities.size(); i++) {
		auto te = entities[i];
		te->update();
	}
}

void Plan::move(Monorail& monorail) {
	position = monorail.en->pos() + (Point::Up * (monorail.en->spec->collision.h/2.0 + entities[0]->spec->collision.h/2.0));
	entities[0]->move(position, monorail.en->dir());
	offsets[0] = Point::Zero;
	entities[0]->update();
}

void Plan::floor(float level) {
	move(position.floor());
}

bool Plan::canRotate() {
	for (uint i = 0; i < entities.size(); i++) {
		auto te = entities[i];
		if (!te->spec->rotateGhost) return false;
	}
	return true;
}

void Plan::rotate() {
	if (!canRotate()) return;
	if (entities.size() == 1) {
		auto ge = entities[0];
		ge->rotate();
		return;
	}
	Mat4 rot = Mat4::rotateY(-glm::radians(90.0));
	for (uint i = 0; i < entities.size(); i++) {
		auto te = entities[i];
		offsets[i] = offsets[i].transform(rot);
		if (te->spec->align) {
			offsets[i].x = std::round(offsets[i].x*2.0f)/2.0f;
			offsets[i].z = std::round(offsets[i].z*2.0f)/2.0f;
		}
		te->rotate();
		te->pos(te->spec->aligned(position + offsets[i], te->dir()));
		te->updateTransform();
	}
}

void Plan::placed(Spec* fromSpec, Point fromPos, Point fromDir) {
	last.spec = fromSpec;
	last.pos = fromPos;
	last.dir = fromDir;
	last.stamp = Sim::tick;
}

bool Plan::canCycle() {
	return entities.size() == 1 && entities[0]->spec->cycle;
}

void Plan::cycle() {
	if (!canCycle()) return;

	auto ge = entities[0];

	auto ce = new GuiFakeEntity(ge->spec->cycle);
	ce->dir(ge->spec->cycleReverseDirection ? -ge->dir(): ge->dir());
	ce->move(Point(ge->pos().x, position.y + ce->spec->collision.h/2.0f, ge->pos().z));

	entities[0] = ce;
	offsets[0] = Point(0, ce->spec->collision.h/2.0f, 0);

	if (ge->spec->conveyor) {
		Point ginput = ge->spec->conveyorInput
			.transform(ge->dir().rotation())
			.transform(ge->pos().translation());
		Box gbox = ginput.box().grow(0.1f);
		for (int i = 0; i < 4; i++) {
			Point cinput = ce->spec->conveyorInput
				.transform(ce->dir().rotation())
				.transform(ce->pos().translation());
			if (gbox.contains(cinput)) break;
			ce->rotate();
		}
	}

	delete ge;
}

Spec* Plan::canFollow() {
	if (entities.size() > 1) return nullptr;
	auto repl = entities[0]->spec->follow;
	while (repl && !repl->licensed) repl = repl->follow;
	return repl;
}

void Plan::follow() {
	if (!canFollow()) return;

	auto ge = entities[0];
	auto repl = canFollow();

	auto ce = new GuiFakeEntity(repl);
	ce->dir(ge->dir());
	ce->move(ge->pos())->floor(ge->ground().y);

	offsets[0] = ce->pos()-position;

	if (ge->spec->raise) {
		offsets[0].y = ge->pos().y;
	}

	if (!ge->spec->raise) {
		offsets[0].y = ge->ground().y + ce->spec->collision.h/2.0f;
	}

	entities[0] = ce;
	delete ge;
}

// PageUp + unlocked
Spec* Plan::canUpward() {
	if (entities.size() > 1) return nullptr;
	auto repl = entities[0]->spec->upward;
	while (repl && !repl->licensed) repl = repl->upward;
	return repl;
}

// PageUp
void Plan::upward() {
	if (!canUpward()) return;

	auto ge = entities[0];
	auto repl = canUpward();

	auto ce = new GuiFakeEntity(repl);
	ce->dir(ge->dir());
	ce->move(ge->pos())->floor(ge->ground().y);

	offsets[0] = ce->pos()-position;

	if (ge->spec->raise) {
		offsets[0].y = ge->pos().y;
	}

	if (!ge->spec->raise) {
		offsets[0].y = ge->ground().y + ce->spec->collision.h/2.0f;
	}

	entities[0] = ce;
	delete ge;
}

// PageDown + unlocked
Spec* Plan::canDownward() {
	if (entities.size() > 1) return nullptr;
	auto repl = entities[0]->spec->downward;
	while (repl && !repl->licensed) repl = repl->downward;
	return repl;
}

// PageDown
void Plan::downward() {
	if (!canDownward()) return;

	auto ge = entities[0];
	auto repl = canDownward();

	auto ce = new GuiFakeEntity(repl);
	ce->dir(ge->dir());
	ce->move(ge->pos())->floor(ge->ground().y);

	offsets[0] = ce->pos()-position;

	if (ge->spec->raise) {
		offsets[0].y = ge->pos().y;
	}

	if (!ge->spec->raise) {
		offsets[0].y = ge->ground().y + ce->spec->collision.h/2.0f;
	}

	entities[0] = ce;
	delete ge;
}

// Similar to Entity::fits() but looser to allow plans to be pasted
// over existing ghosts and entities, or partially force-placed over terrain
bool Plan::fits() {
	for (uint i = 0; i < entities.size(); i++) {
		auto te = entities[i];
		if (!Entity::fits(te->spec, te->pos(), te->dir()) && !entityFits(te->spec, te->pos(), te->dir())) {
			return false;
		}
	}
	return true;
}

bool Plan::entityFits(Spec *spec, Point pos, Point dir) {
	Box bounds = spec->box(pos, dir, spec->collision).shrink(0.01);
	bounds.h = std::max(bounds.h, 0.1);

	for (auto es: Entity::intersecting(bounds)) {
		if (Entity::removing.has(es->id)) continue;
		if (Entity::exploding.has(es->id)) continue;
		if (es->spec->junk) continue;
		if (es->isDeconstruction()) continue;
		if (es->spec->slab && !spec->slab) continue;
		if (!es->spec->slab && spec->slab) continue;
		if (es->spec->vehicleStop && spec->vehicle) continue;
		if (es->spec->cartWaypoint && spec->cart) continue;
		if (es->spec->flightPad && spec->flightPath) continue;
		if (es->spec->monorail && spec->monocar) continue;
		if (es->spec->monocar && spec->monocar) return false;
		if (es->spec->drone) continue;
		if (es->spec != spec) return false;
		if (es->pos() != pos) return false;
		if (es->dir() != dir) return false;
	}

	if (spec->place & Spec::Monorail) {
		for (auto em: Entity::intersecting((pos+(Point::Down*spec->collision.h)).box().grow(0.1))) {
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
			ok = ok || ((spec->place & Spec::Land) && Entity::isLand(p));
			ok = ok || ((spec->place & Spec::Hill) && world.isHill(at));
			ok = ok || ((spec->place & Spec::Water) && (world.isLake(at) && !Entity::isLand(p)));
			if (!ok) return false;
		}
		return true;
	}

	for (auto& footing: spec->footings) {
		Point point = footing.point.transform(dir.rotation()) + pos;
		World::XY at = {(int)std::floor(point.x), (int)std::floor(point.z)};
		bool ok = false;
		ok = ok || ((footing.place & Spec::Land) && Entity::isLand(point));
		ok = ok || ((footing.place & Spec::Hill) && world.isHill(at));
		ok = ok || ((footing.place & Spec::Water) && (world.isLake(at) && !Entity::isLand(point)));
		if (!ok) return false;
	}

	return true;
}

// Used when placing a single entity in series, when the next placement
// depends in some way on the last placement. Eg belts in line
bool Plan::conforms() {
	if (entities.size() > 1) return true;
	if (last.stamp < Sim::tick-30U) return true;

	auto ge = entities[0];

	// Spec is placed rapidly aligned in series, like straight belts.
	// Plans will prevent accidentally placing an identical entity
	// out of line for a short while after the last placement
	if (last.spec->snapAlign && last.spec == ge->spec) {
		if (last.dir != ge->dir()) return false;

		auto eq = [](float a, float b) {
			return std::abs(a-b) < 0.001;
		};

		if (last.dir == Point::North && !eq(last.pos.x, ge->pos().x)) return false;
		if (last.dir == Point::South && !eq(last.pos.x, ge->pos().x)) return false;
		if (last.dir == Point::East  && !eq(last.pos.z, ge->pos().z)) return false;
		if (last.dir == Point::West  && !eq(last.pos.z, ge->pos().z)) return false;
	}
	return true;
}

// Choose a roughly central entity.
// Used for rendering alignment and direction overlay hints.
GuiFakeEntity* Plan::central() {
	GuiFakeEntity* ce = nullptr;
	real dist = 0.0;
	for (uint i = 0; i < entities.size(); i++) {
		auto te = entities[i];
		if (!ce || offsets[i].length() < dist) {
			ce = te;
			dist = offsets[i].length();
		}
	}
	ensure(ce);
	return ce;
}

minimap<Stack,&Stack::iid> Plan::materials() {
	minimap<Stack,&Stack::iid> stacks;
	for (auto ge: entities) {
		for (auto stack: ge->spec->materials) {
			stacks[stack.iid].size += stack.size;
		}
	}
	return stacks;
}

