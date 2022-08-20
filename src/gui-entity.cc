#include "common.h"
#include "gui-entity.h"

// Entities visible on screen are not exposed directly to the rendering thread,
// but loaded into a GuiEntity when a frame starts. This allows the rendering
// thread to mostly proceed without locking the Sim, decoupling UPS and FPS.

void GuiEntity::prepareCaches() {
	auto itemScaleBelt = Mat4::scale(0.5f, 0.5f, 0.5f);

	// materialize transformations for belt items
	for (auto [_,spec]: Spec::all) {
		if (spec->conveyor) {
			for (auto& mat: spec->conveyorTransformsLeft) {
				spec->conveyorTransformsLeftCache[0].push_back(itemScaleBelt * mat * Point::South.rotation());
				spec->conveyorTransformsLeftCache[1].push_back(itemScaleBelt * mat * Point::West.rotation());
				spec->conveyorTransformsLeftCache[2].push_back(itemScaleBelt * mat * Point::North.rotation());
				spec->conveyorTransformsLeftCache[3].push_back(itemScaleBelt * mat * Point::East.rotation());
			}
			for (auto& mat: spec->conveyorTransformsRight) {
				spec->conveyorTransformsRightCache[0].push_back(itemScaleBelt * mat * Point::South.rotation());
				spec->conveyorTransformsRightCache[1].push_back(itemScaleBelt * mat * Point::West.rotation());
				spec->conveyorTransformsRightCache[2].push_back(itemScaleBelt * mat * Point::North.rotation());
				spec->conveyorTransformsRightCache[3].push_back(itemScaleBelt * mat * Point::East.rotation());
			}

			// generate multi-item meshes for backed-up belts
			if (spec->conveyorSlotsLeft == 2 && spec->conveyorSlotsLeft == spec->conveyorSlotsRight
				&& spec->conveyorTransformsLeft.size() == spec->conveyorTransformsRight.size()
			){
				uint slots = spec->conveyorSlotsLeft;
				uint steps = spec->conveyorTransformsLeft.size()/slots;

				// extract the North/South component of item transforms
				for (auto& mat: spec->conveyorTransformsLeft) {
					Point pos = {0,0,Point::Zero.transform(mat).z-0.5f};
					Mat4 trx = pos.translation();
					spec->conveyorTransformsMultiCache[0].push_back(itemScaleBelt * trx * Point::South.rotation());
					spec->conveyorTransformsMultiCache[1].push_back(itemScaleBelt * trx * Point::West.rotation());
					spec->conveyorTransformsMultiCache[2].push_back(itemScaleBelt * trx * Point::North.rotation());
					spec->conveyorTransformsMultiCache[3].push_back(itemScaleBelt * trx * Point::East.rotation());
				}

				for (auto [_,item]: Item::names) {
					for (auto part: item->parts) {
						std::vector<Point> bumps;
						for (int i = 0, l = slots; i < l; i++) {
							float compensateItemScaleBelt = 2.0f;
							float compensatePartPreScaled = 1.0/part->scale.x;
							float scale = compensateItemScaleBelt * compensatePartPreScaled;
							bumps.push_back(spec->conveyorCentroid.transform(spec->conveyorTransformsLeft[i*steps]) * scale);
							bumps.push_back(spec->conveyorCentroid.transform(spec->conveyorTransformsRight[i*steps]) * scale);
						}
						spec->conveyorPartMultiCache[item->id].push_back(part->multi(bumps));
					}
				}
			}
		}
	}
}

GuiEntity::GuiEntity() {
	id = 0;
	spec = nullptr;
	pos(Point::Zero);
	dir(Point::South);
	state = 0;
	health = 1.0;
	flags = 0;

	ghost = false;
	aim = Point::South;
	iid = 0;
	fid = 0;
	radius = 0;
	connected = false;
	color = 0xffffffff;

	crafter.recipe = nullptr;

	tube = nullptr;
	monorail = nullptr;
	shipyard = nullptr;
	powerpole = nullptr;

	cartWaypoint.relative[CartWaypoint::Red] = Point::Zero;
	cartWaypoint.relative[CartWaypoint::Blue] = Point::Zero;
	cartWaypoint.relative[CartWaypoint::Green] = Point::Zero;
}

GuiEntity::GuiEntity(uint eid) : GuiEntity() {
	load(Entity::get(eid));
	updateTransform();
}

GuiEntity::GuiEntity(Entity* en) : GuiEntity() {
	load(*en);
	updateTransform();
}

GuiEntity::GuiEntity(const GuiEntity& other) : GuiEntity() {
	id = other.id;
	spec = other.spec;
	pos(other.pos());
	dir(other.dir());
	state = other.state;
	health = other.health;
	flags = other.flags;
	ghost = other.ghost;
	aim = other.aim;
	iid = other.iid;
	fid = other.fid;
	radius = other.radius;
	connected = other.connected;
	color = other.color;
	cartWaypointLine = other.cartWaypointLine;
	conveyor.left[0] = other.conveyor.left[0];
	conveyor.left[1] = other.conveyor.left[1];
	conveyor.left[2] = other.conveyor.left[2];
	conveyor.right[0] = other.conveyor.right[0];
	conveyor.right[1] = other.conveyor.right[1];
	conveyor.right[2] = other.conveyor.right[2];
	cartWaypoint = other.cartWaypoint;

	if (other.tube) {
		tube = new tubeState;
		tube->length = other.tube->length;
		tube->stuff = other.tube->stuff;
		tube->origin = other.tube->origin;
		tube->target = other.tube->target;
	}

	if (other.shipyard) {
		shipyard = new shipyardState;
		shipyard->stage = other.shipyard->stage;
		shipyard->pos = other.shipyard->pos;
		shipyard->spec = other.shipyard->spec;
		shipyard->ghost = other.shipyard->ghost;
	}

	if (other.monorail) {
		monorail = new monorailState;
		monorail->railsOut = other.monorail->railsOut;
	}

	if (other.powerpole) {
		powerpole = new powerpoleState;
		powerpole->wires = other.powerpole->wires;
		powerpole->point = other.powerpole->point;
	}
}

GuiEntity::~GuiEntity() {
	delete tube;
	tube = nullptr;
	delete monorail;
	monorail = nullptr;
	delete powerpole;
	powerpole = nullptr;
	delete shipyard;
	shipyard = nullptr;
}

void GuiEntity::load(const Entity& en) {
	id = en.id;
	spec = en.spec;
	pos(en.pos());
	dir(en.dir());
	state = en.state;
	health = en.health;
	flags = en.flags;
	ghost = en.isGhost();

	if (spec->status) {
		if (!isEnabled()) status = Status::Warning;
	}

	if (spec->coloredCustom) {
		color = en.color();
	}

	loadArm();
	loadLoader();
	loadCrafter();
	loadConveyor(en);
	loadNetworker();
	loadDrone();
	loadCart();
	loadCartWaypoint();
	loadExplosion();
	loadTurret();
	loadTube();
	loadMonorail();
	loadMonocar();
	loadPipe();
	loadComputer();
	loadRouter();
	loadPowerPole();
	loadShipyard();

	if (spec->coloredAuto && !iid && spec->store) {
		iid = en.store().hint.iid;
	}

	if (spec->coloredAuto && iid) {
		Item* item = Item::get(iid);
		color = item->parts.front()->color;
	}

	if (!ghost && spec->generateElectricity && !en.electricityProducer().connected()) flags |= ELECTRICITY;
	if (!ghost && spec->consumeElectricity && !en.electricityConsumer().connected()) flags |= ELECTRICITY;
	if (!ghost && spec->consumeCharge && !en.charger().powered()) flags |= ELECTRICITY;
}

void GuiEntity::loadConveyor(const Entity& en) {
	if (spec->conveyor) {
		auto& c = en.conveyor();
		for (uint i = 0; i < c.left.slots; i++)
			conveyor.left[i] = c.left.items[i];
		for (uint i = 0; i < c.right.slots; i++)
			conveyor.right[i] = c.right.items[i];
	}
}

void GuiEntity::loadArm() {
	if (spec->arm) {
		auto& arm = Arm::get(id);
		iid = arm.iid;
	}
}

void GuiEntity::loadLoader() {
}

void GuiEntity::loadCrafter() {
	if (spec->crafter) {
		auto& crafter = Crafter::get(id);
		this->crafter.recipe = crafter.recipe;

		// sync with Popup::crafterNotice()
		if (!isEnabled()) {
			status = Status::Warning;
		}
		else
		if (spec->consumeFuel && !Burner::get(id).fueled()) {
			status = Status::Alert;
		}
		else
		if (crafter.recipe && !crafter.recipe->licensed) {
			status = Status::Alert;
		}
		else
		if (crafter.working && spec->consumeElectricity && crafter.efficiency < 0.1) {
			status = Status::Alert;
		}
		else
		if (crafter.working && spec->consumeElectricity && crafter.efficiency < 0.99) {
			status = Status::Warning;
		}
		else
		if (crafter.working || crafter.interval) {
			status = Status::Ok;
		}
		else
		if (!crafter.working && crafter.insufficientResources()) {
			status = Status::Alert;
		}
		else
		if (!crafter.working && crafter.excessProducts()) {
			status = Status::Alert;
		}
		else {
			status = Status::Warning;
		}
	}
}

void GuiEntity::loadNetworker() {
	if (spec->networker) {
		for (auto& interface: Networker::get(id).interfaces) {
			connected = connected || interface.network;
		}
	}
}

void GuiEntity::loadDrone() {
	if (spec->drone) {
		iid = Drone::get(id).iid;
	}
}

void GuiEntity::loadCart() {
	if (spec->cart) {
		iid = 0;
		for (auto stack: Store::get(id).stacks) {
			iid = stack.iid;
			break;
		}
		auto& cart = Cart::get(id);
		if (!isEnabled()) status = Status::Warning;
		else if (cart.halt) status = Status::Alert;
		else if (cart.lost) status = Status::Warning;
		else if (cart.fueled < 0.99) status = Status::Warning;
		else status = Status::Ok;
		cartLine = cart.line;

		if (cart.blocked) flags |= BLOCKED;
	}
}

void GuiEntity::loadCartWaypoint() {
	if (spec->cartWaypoint) {
		auto& waypoint = CartWaypoint::get(id);
		cartWaypoint.relative[CartWaypoint::Red] = waypoint.relative[CartWaypoint::Red];
		cartWaypoint.relative[CartWaypoint::Blue] = waypoint.relative[CartWaypoint::Blue];
		cartWaypoint.relative[CartWaypoint::Green] = waypoint.relative[CartWaypoint::Green];
	}
}

void GuiEntity::loadExplosion() {
	if (spec->explosion) {
		radius = Explosion::get(id).radius;
	}
}

void GuiEntity::loadTurret() {
	if (spec->turret) {
		auto& t = Turret::get(id);
		aim = t.aim;
		Entity* target = t.tid && t.fire ? Entity::find(t.tid): nullptr;
		turret.target = target ? target->pos(): Point::Zero;
		dir(Point::South);
	}
}

void GuiEntity::loadTube() {
	if (spec->tube) {
		if (!tube)
			tube = new tubeState;
		auto& t = Tube::get(id);
		tube->length = t.length;
		tube->stuff = t.stuff;
		tube->origin = t.origin();
		tube->target = t.target();

		if (!tube->length && t.next && Entity::exists(t.next)) {
			auto& s = Tube::get(t.next);
			tube->length = std::floor(tube->origin.distance(s.origin()) * 1000.0f);
		}
	}
}

void GuiEntity::loadMonorail() {
	if (spec->monorail) {
		if (!monorail)
			monorail = new monorailState;
		auto& m = Monorail::get(id);
		monorail->railsOut = m.railsOut();
	}
}

void GuiEntity::loadMonocar() {
	if (spec->monocar) {
		auto& monocar = Monocar::get(id);
//		if (!isEnabled()) status = Status::Warning;
//		else if (cart.halt) status = Status::Alert;
//		else if (cart.lost) status = Status::Warning;
//		else if (cart.fueled < 0.99) status = Status::Warning;
//		else status = Status::Ok;
		monorailLine = monocar.line;
		iid = monocar.iid;
	}
}

void GuiEntity::loadPipe() {
	if (spec->pipe) {
		auto& pipe = Pipe::get(id);
		fid = pipe.network ? pipe.network->fid: 0;
		level = pipe.network ? pipe.network->level(): 0;
	}
}

void GuiEntity::loadComputer() {
	if (!ghost && spec->computer) {
		auto& computer = Computer::get(id);
		status = computer.err > 0 ? Status::Alert: Status::Ok;
		if (!isEnabled() || computer.debug) status = Status::Warning;
	}
}

void GuiEntity::loadRouter() {
	if (!ghost && spec->router) {
		auto& router = Router::get(id);
		auto& networker = Networker::get(id);

		status = Status::Ok;

		auto check = [&](auto& rule) {
			return rule.condition.evaluate(networker.interfaces[rule.nicSrc].network
				? networker.interfaces[rule.nicSrc].network->signals : Signal::NoSignals);
		};

		for (auto& rule: router.rules) {
			if (rule.mode == Router::Rule::Mode::Alert && rule.alert == Router::Rule::Alert::Warning && check(rule)) {
				status = Status::Warning;
			}
			else
			if (rule.mode == Router::Rule::Mode::Alert && rule.alert == Router::Rule::Alert::Critical && check(rule)) {
				status = Status::Alert;
			}
		}

		if (!isEnabled() || !router.rules.size()) status = Status::Warning;
	}
}

void GuiEntity::loadPowerPole() {
	if (spec->powerpole) {
		if (!powerpole)
			powerpole = new powerpoleState;

		auto& pole = PowerPole::get(id);
		powerpole->wires.clear();
		powerpole->point = pole.point();

		if (spec->status) {
			// should roughly align with Popup::powerpoleNotice()
			status = Status::Ok;
			if (ghost || !pole.network) {
				status = Status::Warning;
			}
			else
			if (pole.network->lowPower()) {
				status = Status::Alert;
			}
			else
			if (pole.network->brownOut()) {
				status = Status::Warning;
			}
			else
			if (pole.network->noCapacity()) {
				status = Status::Warning;
			}
			else {
				status = Status::Ok;
			}
		}

		for (auto& sid: pole.links) {
			auto& sibling = PowerPole::get(sid);
			auto end = sibling.point();
			powerpole->wires.push_back({end, powerpole->point < end});
		}

		if (ghost) {
			for (auto sid: pole.siblings()) {
				auto& sibling = PowerPole::get(sid);
				auto end = sibling.point();
				powerpole->wires.push_back({end, powerpole->point < end});
			}
		}
	}
}

void GuiEntity::loadShipyard() {
	if (spec->shipyard) {
		if (!shipyard)
			shipyard = new shipyardState;
		auto& s = Shipyard::get(id);
		shipyard->stage = s.stage;
		shipyard->pos = s.shipPos();
		shipyard->spec = s.ship;
		shipyard->ghost = s.shipGhost();
	}
}

Point GuiEntity::pos() const {
	return {
		(real)_pos.x/(real)1000,
		(real)_pos.y/(real)1000,
		(real)_pos.z/(real)1000,
	};
}

Point GuiEntity::pos(Point p) {
	_pos.x = (int)(p.x*(real)1000);
	_pos.y = (int)(p.y*(real)1000);
	_pos.z = (int)(p.z*(real)1000);
	return pos();
}

Point GuiEntity::dir() const {
	return { (real)_dir.x, (real)_dir.y, (real)_dir.z};
}

Point GuiEntity::dir(Point p) {
	_dir.x = (float)p.x;
	_dir.y = (float)p.y;
	_dir.z = (float)p.z;
	return dir();
}

Box GuiEntity::box() const {
	return spec->box(pos(), dir(), spec->collision);
}

Box GuiEntity::selectionBox() const {
	return spec->box(pos(), dir(), spec->selection);
}

Box GuiEntity::southBox() const {
	return spec->southBox(pos());
}

Box GuiEntity::miningBox() const {
	return box().grow(0.5f);
}

Box GuiEntity::drillingBox() const {
	return box().grow(0.5f);
}

Point GuiEntity::ground() const {
	return {pos().x, spec->underground ? pos().y + spec->collision.h/2.0f: pos().y - spec->collision.h/2.0f, pos().z};
}

Sphere GuiEntity::sphere() const {
	return box().sphere();
}

Cuboid GuiEntity::cuboid() const {
	return Cuboid(spec->southBox(pos(), spec->collision), dir());
}

Cuboid GuiEntity::selectionCuboid() const {
	return Cuboid(spec->southBox(pos(), spec->selection), dir());
}

void GuiEntity::updateTransform() {
	//transform = dir().rotation() * pos().translation();
}

// can other connect to this
bool GuiEntity::connectable(GuiEntity* other) const {
	if (spec->tube) {
		return id != other->id && other->spec->tube && pos().distance(other->pos()) < ((real)spec->tubeSpan / 1000.0);
	}
	return false;
}

void GuiEntity::instance() {
	Point pos = this->pos();
	Point dir = this->dir();
	if (ghost) pos += Point::Up*0.01;

	float distance = pos.distance(scene.position);
	auto trx = dir.rotationAltAz() * (pos + scene.offset).translation();

	for (uint i = 0, l = spec->parts.size(); i < l; i++) {
		auto part = spec->parts[i];
		if (part->autoColor || part->customColor) continue;
		if (!part->show(pos, dir)) continue;
		GLuint group = (ghost || part->ghost) ? scene.shader.ghost.id(): (part->glow ? scene.shader.glow.id(): scene.shader.part.id());
		part->instanceSpec(group, distance, trx, spec, i, state, aim, part->color, crafter.recipe);
	}

	if (spec->coloredAuto || spec->coloredCustom) {
		for (uint i = 0, l = spec->parts.size(); i < l; i++) {
			auto part = spec->parts[i];
			if (!(part->autoColor || part->customColor)) continue;
			if (!part->show(pos, dir)) continue;
			GLuint group = (ghost || part->ghost) ? scene.shader.ghost.id(): (part->glow ? scene.shader.glow.id(): scene.shader.part.id());
			part->instanceSpec(group, distance, trx, spec, i, state, aim, color, crafter.recipe);
		}
	}

	if (spec->explosion) {
		scene.sphere(Sphere(pos, radius), 0xff0000ff);
		scene.sphere(Sphere(pos, radius*0.75), 0xffff00ff);
	}

	if (spec->tube && tube && tube->length) {
		Point move = tube->origin + scene.offset;
		Point path = tube->target - tube->origin;
		Point mid = path * 0.5f;
		Point step = path.normalize();

		auto rot = step.rotationAltAz();
		auto az = step.rotationAltAz();
		auto end = Mat4::scale(1,1,8) * rot;

		Mat4 glass = Mat4::scale(1,1,path.length()) * rot * Mat4::translate(mid + move);
		spec->tubeGlass->instance(scene.shader.ghost.id(), distance, glass);

		Mat4 end1 = end * Mat4::translate((step*0.5) + move);
		spec->tubeRing->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), distance, end1);

		Mat4 end2 = end * Mat4::translate(path - (step*0.5) + move);
		spec->tubeRing->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), distance, end2);

		for (int i = 2, l = std::floor(path.length()); i < l; i+=2) {
			Mat4 ring = rot * Mat4::translate(step*i + move);
			spec->tubeRing->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), distance, ring);
			if (spec->tubeChevron->distanceLOD(distance)) {
				Mat4 chevron = az * Mat4::translate(step*i + move);
				spec->tubeChevron->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), distance, chevron);
			}
		}
	}

	if (spec->tube && tube && spec->conveyor) {
		float height = spec->collision.h-1.5f;
		Point origin = tube->origin;
		Point target = tube->origin + (Point::Down*height);

		Point path = target - origin;
		Point from = origin + scene.offset;
		Point mid = (target - origin) * 0.5f;

		auto rot = path.normalize().rotation();

		auto mat = Mat4::scale(1,1,path.length()*10.0f) * rot * Mat4::translate(from + mid);
		spec->tubeRing->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), distance, mat);
	}

	if (spec->cart && spec->cartRoute) {
		auto color = cartRouteColor(cartLine);
		spec->cartRoute->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), distance, trx, color);
	}

	if (spec->cartWaypoint) {
		if (cartWaypoint.relative[CartWaypoint::Red] != Point::Zero)
			waypointLine(CartWaypoint::Red, pos, pos + cartWaypoint.relative[CartWaypoint::Red]);
		if (cartWaypoint.relative[CartWaypoint::Blue] != Point::Zero)
			waypointLine(CartWaypoint::Blue, pos, pos + cartWaypoint.relative[CartWaypoint::Blue]);
		if (cartWaypoint.relative[CartWaypoint::Green] != Point::Zero)
			waypointLine(CartWaypoint::Green, pos, pos + cartWaypoint.relative[CartWaypoint::Green]);
	}

	if (spec->monocar && spec->monocarRoute) {
		auto color = monorailRouteColor(monorailLine);
		spec->monocarRoute->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), distance, trx, color);
	}

	if (spec->monorail && monorail) {
		miniset<Rail> seen;
		for (auto& rl: monorail->railsOut) {
			if (seen.has(rl.rail) && distance > Config::window.levelsOfDetail[Part::MD]) continue;
			bool routeOnly = seen.has(rl.rail);
			monorailLink(rl.line, rl.rail, routeOnly, false);
			seen.insert(rl.rail);
		}
	}

	if (spec->turret && turret.target != Point::Zero) {
		Mat4 rot = Point(aim.x, 0, aim.z).rotation();
		Point fire = spec->turretPivotFirePoint.transform(rot) + pos;
		scene.line(fire, turret.target, spec->turretTrail, scene.pen()/2.0f);
	}

	if (!ghost && spec->networker && connected && distance < Config::window.levelsOfDetail[Part::HD]) {
		auto trxA = Mat4::translate(spec->networkWifi) * trx;
		scene.bits.wifi->instance(scene.shader.part.id(), trxA, Color(0x444444ff), 0.0f, Part::SHADOW);
		float radius1 = 0.005*(Sim::tick%30);
		float alpha1 = (0.8-(radius1*2.5));
		float radius2 = 0.15 + 0.005*(Sim::tick%30);
		float alpha2 = (0.8-(radius2*2.5));
		Point center = pos + spec->networkWifi.transform(dir.rotation()) + (Point::Up*0.25);
		scene.circle(center, radius1, Color(0.5,0.5,0.5,alpha1), 1, 30);
		scene.circle(center, radius2, Color(0.5,0.5,0.5,alpha2), 1, 15);
	}

	if (spec->status && distance < Config::window.levelsOfDetail[Part::HD] && spec->beacon != Point::Zero) {
		bool light = false;
		Color color = 0x00000ff;
		if (!ghost && flags & Entity::RULED && isEnabled()) {
			light = true;
			color = (flags & Entity::BLOCKED) ? 0xff0000ff: 0x00ff00ff;
		}
		else {
			switch (status) {
				case Status::None: break;
				case Status::Ok: light = true; color = 0x00ff00ff; break;
				case Status::Alert: light = true; color = 0xff0000ff; break;
				case Status::Warning: light = true; color = 0xffa500ff; break;
			}
		}
		if (light) {
			auto trxA = Mat4::translate(spec->beacon) * trx;
			auto frame = ghost ? scene.shader.ghost.id(): scene.shader.part.id();
			auto shine = ghost ? scene.shader.ghost.id(): scene.shader.glow.id();
			scene.bits.beaconFrame->instance(frame, trxA, Color(0x444444ff), 0.0f, Part::NOSHADOW);
			scene.bits.beaconGlass->instance(shine, trxA, color, 0.0f, Part::NOSHADOW);
		}
	}

	if (spec->health && health < spec->health && !spec->junk && !spec->enemy) {
		scene.cuboid(selectionCuboid(), 0xff0000ff, scene.pen());
	}

	if (scene.directing && id == scene.directing->id) {
		if (spec->zeppelin) {
			Sim::locked([&]() {
				if (!Entity::exists(id)) return;
				auto& en = Entity::get(id);
				auto& zeppelin = en.zeppelin();
				if (!zeppelin.moving) return;

				auto elevation = world.elevation(zeppelin.target);
				Point target = zeppelin.target.floor(std::max(0.0f, elevation));

				scene.line(pos, target, 0xff9900ff, scene.pen());
			});
		}
		else
		if (spec->cart) {
			Sim::locked([&]() {
				if (!Entity::exists(id)) return;
				auto& en = Entity::get(id);
				auto& cart = en.cart();

				Point target = cart.target != Point::Zero ? cart.target: en.pos();
				target = target.floor(0.0f);

				if (target.distance(pos) > 1.0)
					scene.line(pos, target, 0xff9900ff, scene.pen());
			});
		}
	}

	if (spec->pipe && spec->pipeLevels.size() && fid) {
		Fluid* fluid = Fluid::get(fid);
		for (auto& pl: spec->pipeLevels) {
			float height = pl.box.h*level;
			float down = (pl.box.h-height)/2.0;
			auto scale = Mat4::scale(pl.box.w,pl.box.h*level,pl.box.d);
			Mat4 m0 = scale * pl.trx * (Point::Down*down).translation() * trx;
			scene.unit.mesh.cube->instance(scene.shader.ghost.id(), m0, fluid->color);
			if (distance < Config::window.levelsOfDetail[Part::HD]) {
				auto scale = Mat4::scale(0.03, 0.03, 0.03);
				float y = std::max(0.015, std::min(pl.box.h-0.03, pl.box.h*level));
				Mat4 m1 = scale * Point(0,y-down-(height/2.0),0.05).translation() * pl.trx * trx;
				scene.unit.mesh.sphere->instance(scene.shader.part.id(), m1, Color(0xffa500ff));
			}
		}
	}

	if (spec->shipyard) {
		if (shipyard && shipyard->spec && shipyard->pos != Point::Zero) {
			auto gs = GuiFakeEntity(shipyard->spec);
			gs.health = spec->health;
			gs.ghost = shipyard->ghost;
			gs.move(pos + shipyard->pos.transform(dir.rotation()), dir);
			gs.instance();
		}
	}
}

void GuiEntity::instanceItems() {
	if (ghost) return;
	if (!(spec->drone || spec->arm || spec->conveyor || spec->tube || spec->cart || (spec->showItem && iid))) return;

	Point pos = this->pos();
	Point dir = this->dir();

	float distance = pos.distance(scene.position);
	auto trx = dir.rotation() * (pos + scene.offset).translation();

	if (spec->drone && iid) {
		auto itemScaleDrone = Mat4::scale(0.7f, 0.7f, 0.7f);
		Item* item = Item::get(iid);
		if (item->hasLOD(distance)) {
			Point p = pos - (Point::Up*0.6f) + scene.offset;
			Mat4 t = itemScaleDrone * Mat4::translate(p.x, p.y + item->droneV, p.z);
			for (Part* part: item->parts) {
				part->instance(scene.shader.part.id(), distance, t, part->color, Config::mode.itemShadows);
			}
		}
	}

	if (spec->arm && iid) {
		auto itemScaleArm = Mat4::scale(0.6f, 0.6f, 0.6f);
		Item* item = Item::get(iid);
		if (item->hasLOD(distance)) {
			uint grip = spec->states[0].size()-1;
			Point p = Point::Zero.transform(spec->states[state][grip]);
			Mat4 t = itemScaleArm * Mat4::translate(p.x,p.y+item->armV,p.z) * trx;
			for (Part* part: item->parts) {
				part->instance(scene.shader.part.id(), distance, t, part->color, Config::mode.itemShadows);
			}
		}
	}

	if (spec->conveyor) {
		for (;;) {
			auto ipos = pos - (Point::Up*0.25f) + scene.offset;

			int cache = 0;
			if (dir == Point::West) cache = 1;
			if (dir == Point::North) cache = 2;
			if (dir == Point::East) cache = 3;
			Item* item = nullptr;

			int slotsLeft = spec->conveyorSlotsLeft;
			int stepsLeft = spec->conveyorTransformsLeftCache[cache].size()/slotsLeft;
			int slotsRight = spec->conveyorSlotsRight;
			int stepsRight = spec->conveyorTransformsRightCache[cache].size()/slotsRight;

			// backed-up belts of a single item type
			if (Config::mode.meshMerging && slotsLeft == slotsRight && stepsLeft == stepsRight && conveyor.left[0].iid) {
				uint iid = conveyor.left[0].iid;
				uint offset = conveyor.left[0].offset;

				bool uniform = true;
				for (int i = 0; uniform && i < slotsLeft; i++) {
					uniform = uniform && conveyor.left[i].iid == iid && conveyor.left[i].offset == offset;
					uniform = uniform && conveyor.right[i].iid == iid && conveyor.right[i].offset == offset;
				}

				if (uniform && spec->conveyorPartMultiCache.count(iid)) {
					item = Item::get(iid);
					Mat4 move = Mat4::translate(ipos + Point(0, item->beltV, 0));
					Mat4 bump = spec->conveyorTransformsMultiCache[cache][offset];
					Mat4 m = bump * move;
					for (Part* part: spec->conveyorPartMultiCache[iid]) {
						part->instance(scene.shader.part.id(), distance, m, part->color, Config::mode.itemShadows);
					}
					break;
				}
			}

			for (int i = 0; i < slotsLeft; i++) {
				auto& slot = conveyor.left[i];
				if (!slot.iid) continue;
				item = !item || item->id != slot.iid ? Item::get(slot.iid): item;
				if (item->hasLOD(distance)) {
					Mat4 move = Mat4::translate(ipos + Point(0, item->beltV, 0));
					Mat4 bump = spec->conveyorTransformsLeftCache[cache][slot.offset + stepsLeft*i];
					Mat4 m = bump * move;
					for (Part* part: item->parts) {
						part->instance(scene.shader.part.id(), distance, m, part->color, Config::mode.itemShadows);
					}
				}
			}

			for (int i = 0; i < slotsRight; i++) {
				auto& slot = conveyor.right[i];
				if (!slot.iid) continue;
				item = !item || item->id != slot.iid ? Item::get(slot.iid): item;
				if (item->hasLOD(distance)) {
					Mat4 move = Mat4::translate(ipos + Point(0, item->beltV, 0));
					Mat4 bump = spec->conveyorTransformsRightCache[cache][slot.offset + stepsRight*i];
					Mat4 m = bump * move;
					for (Part* part: item->parts) {
						part->instance(scene.shader.part.id(), distance, m, part->color, Config::mode.itemShadows);
					}
				}
			}
			break;
		}
	}

	if (spec->tube && tube && tube->length) {
		auto itemScaleTube = Mat4::scale(0.5f, 0.5f, 0.5f);
		Point path = tube->target - tube->origin;
		Point from = tube->origin + scene.offset;

		auto rot = path.normalize().rotationAltAz();
		Item* item = nullptr;

		for (auto& thing: tube->stuff) {
			float offset = (float)thing.offset/(float)tube->length;
			item = !item || item->id != thing.iid ? Item::get(thing.iid): item;
			if (item->hasLOD(distance)) {
				Mat4 m = Mat4::translate(from + (path * offset) + Point(0, item->tubeV-0.1, 0));
				Mat4 t = itemScaleTube * rot * m;
				for (Part* part: item->parts) {
					part->instance(scene.shader.part.id(), distance, t, part->color, Config::mode.itemShadows);
				}
			}
		}
	}

	if (spec->cart && iid) {
		Item* item = Item::get(iid);
		if (item->hasLOD(distance)) {
			Mat4 m = spec->cartItem.translation() * trx;
			for (Part* part: item->parts) {
				part->instance(scene.shader.part.id(), distance, m, part->color, Config::mode.itemShadows);
			}
		}
	}

	if (spec->showItem && iid) {
		Item* item = Item::get(iid);
		if (item->hasLOD(distance)) {
			Mat4 m = spec->showItemPos.translation() * trx;
			for (Part* part: item->parts) {
				part->instance(scene.shader.part.id(), distance, m, part->color, Config::mode.itemShadows);
			}
		}
	}
}

void GuiEntity::instanceCables() {

	auto line = [&](const Point& a, const Point& b) {
		auto c = b-a;
		auto pen = scene.pen()/2.0;
		auto scale = Mat4::scale(pen,pen,c.length());
		auto rotate = c.rotation();
		auto translate = (a + (c * 0.5) + scene.offset).translation();
		scene.unit.mesh.line->instance(scene.shader.part.id(), scale * rotate * translate, Color(0x444444ff));
	};

	if (spec->powerpole && powerpole) {
		for (auto& wire: powerpole->wires) {
			// The electricity grid is a doubly-linked loom, so render each wire only once
			// Ghosts that are fake entities being placed have no real wires, so force those
			if (!ghost && !wire.render) continue;
			Point posA = powerpole->point;
			Point posB = wire.target;
			Point dirAB = (posB-posA).normalize();
			float distance = posA.distance(posB);
			Point middle = posA + (dirAB*(distance/2)) + Point::Down;
			Point dirA = (middle-posA).normalize();
			Point dirB = (posB-middle).normalize();
			auto curve = Rail(posA, dirA, posB, dirB);
			auto last = posA;
			for (auto next: curve.steps(1.0f)) {
				line(last, next);
				last = next;
			}
			line(last, posB);
		}
	}
}

void GuiEntity::overlayHovering(bool full) {
	Color input = 0xffffffff;
	Color output = 0xffff00ff;

	bool detail = scene.keyDown(SDLK_SPACE) && (
		(scene.hovering && scene.hovering->id == id) || (scene.placing && scene.placing->entities.size() == 1u && scene.placing->entities.front()->id == id)
	);

	enum class Direction {
		In = 0,
		Out,
		Both
	};

	auto indicator = [&](Point point, Direction type, Point idir = Point::Zero) {
		auto face = idir.rotation();
		auto trx = point.translation() * dir().rotation() * (pos() + scene.offset).translation();

		switch (type) {
			case Direction::In: {
				auto m = face * Mat4::scale(0.6) * trx;
				scene.bits.chevron->instance(scene.shader.glow.id(), m, input, 0.0f, Part::NOSHADOW);
				break;
			}
			case Direction::Out: {
				auto m = face * Mat4::scale(0.6) * trx;
				scene.bits.chevron->instance(scene.shader.glow.id(), m, output, 0.0f, Part::NOSHADOW);
				break;
			}
			case Direction::Both: {
				auto mA = face * Mat4::scale(0.6) * Mat4::translate(0.175,0,0) * trx;
				auto mB = face * Mat4::scale(0.6) * Mat4::translate(0.175,0,0) * trx;
				scene.bits.chevron->instance(scene.shader.glow.id(), mA, input, 0.0f, Part::NOSHADOW);
				scene.bits.chevron->instance(scene.shader.glow.id(), mB, output, 0.0f, Part::NOSHADOW);
				break;
			}
		}
	};

	auto indicatorPipe = [&](Point point, Direction type) {
		auto rot = Point::South.rotation();
		auto color = Color(0x26f7fdff);

		if (spec->collision.side(Point::West,0.2).intersects(point.box().grow(0.1))) {
			rot = Point::West.rotation();
		}
		if (spec->collision.side(Point::North,0.2).intersects(point.box().grow(0.1))) {
			rot = Point::North.rotation();
		}
		if (spec->collision.side(Point::East,0.2).intersects(point.box().grow(0.1))) {
			rot = Point::East.rotation();
		}

		auto flip = Point::North.rotation();
		auto trx = point.translation() * dir().rotation() * (pos() + scene.offset).translation();

		switch (type) {
			case Direction::In: {
				auto m = Mat4::scale(0.6) * Mat4::translate(0,0,-0.25) * rot * flip * trx;
				scene.bits.chevron->instance(scene.shader.glow.id(), m, input, 0.0f, Part::NOSHADOW);
				break;
			}
			case Direction::Out: {
				auto m = Mat4::scale(0.6) * Mat4::translate(0,0,0.25) * rot * trx;
				scene.bits.chevron->instance(scene.shader.glow.id(), m, output, 0.0f, Part::NOSHADOW);
				break;
			}
			case Direction::Both: {
				auto mA = Mat4::scale(0.6) * Mat4::translate(0.165,0,0.25) * rot * trx;
				auto mB = Mat4::scale(0.6) * Mat4::translate(0.165,0,-0.25) * rot * flip * trx;
				scene.bits.chevron->instance(scene.shader.glow.id(), mA, color, 0.0f, Part::NOSHADOW);
				scene.bits.chevron->instance(scene.shader.glow.id(), mB, color, 0.0f, Part::NOSHADOW);
				break;
			}
		}

		if (!spec->pipeHints) return;

		auto trxB = rot * point.translation() * dir().rotation() * (pos() + scene.offset).translation();
		scene.bits.pipeConnector->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), trxB, Color(0xffa500ff), 0.0f, Part::NOSHADOW);
	};

	if (spec->pipe && !spec->pipeValve) {
		for (auto point: spec->pipeConnections) {
			indicatorPipe(point, Direction::Both);
		}
	}

	if (spec->pipeValve && spec->pipeConnections.size() == 2) {
		indicatorPipe(spec->pipeConnections[0], Direction::In);
		indicatorPipe(spec->pipeConnections[1], Direction::Out);
	}

	bool iopipes = spec->pipeHints && (spec->pipeInputConnections.size() || spec->pipeOutputConnections.size());
	if (spec->crafter && !ghost && crafter.recipe && !crafter.recipe->inputFluids.size() && !crafter.recipe->outputFluids.size()) iopipes = false;

	if (iopipes) {
		for (auto point: spec->pipeInputConnections) {
			indicatorPipe(point, Direction::In);
		}
		for (auto point: spec->pipeOutputConnections) {
			indicatorPipe(point, Direction::Out);
		}
	}

	if (spec->arm) {
		indicator(spec->armInput + (Point::Up*0.25), Direction::In);
		indicator(spec->armOutput + (Point::Up*0.25), Direction::Out);
	}

	if (full && spec->conveyor) {
		Point in = spec->conveyorInput.floor(0.0).normalize();
		Point out = spec->conveyorOutput.floor(0.0).normalize();
		if (in != Point::Zero) indicator((in*(spec->collision.d*0.5)) + (in*0.5) + (Point::Up*0.25), Direction::In, -in);
		if (out != Point::Zero) indicator((out*(spec->collision.d*0.5)) + (out*0.5) + (Point::Up*0.25), Direction::Out, out);
	}

	if (spec->effector) {
		indicator(Point::South, Direction::Out);
	}

	if (spec->depot) {
		scene.circle(pos().floor(1.0), spec->depotRange, 0xffff00ff, scene.pen());
		Sim::locked([&]() {
			auto en = Entity::find(id);
			if (!en) return;
			auto& depot = en->depot();
			for (auto did: depot.drones) {
				auto ed = Entity::find(did);
				if (!ed) continue;
				scene.line(pos(), ed->pos(), 0xff9900ff, scene.pen());
			}
		});
	}

	if (spec->turret) {
		scene.circle(pos(), spec->turretRange, 0xff0000ff, scene.pen());
	}

	auto packets = [&](Point origin, Point target, Color color) {
		Point packet = origin;
		Point step = (target-origin).normalize() * 0.25;
		int cursor = Sim::tick % scene.packets.size(), steps = 0;
		std::vector<glm::mat4> batch;
		auto scale = Mat4::scale(0.01 * scene.pen());
		while (packet.distanceSquared(origin) < target.distanceSquared(origin)) {
			if (scene.packets[cursor++ % scene.packets.size()])
				batch.push_back(scale * (packet + scene.offset).translation());
			packet = origin + (step * (float)steps++);
		}
		scene.unit.mesh.cube->instances(scene.shader.ghost.id(), batch, color, 0.0f, Part::NOSHADOW);
	};

	if (spec->networker && !spec->networkHub) {
		Sim::locked([&]() {
			if (!Entity::exists(id)) return;
			auto& en = Entity::get(id);
			auto& networker = en.networker();
			for (auto eid: networker.links()) {
				if (!Entity::exists(eid)) continue;
				auto& el = Entity::get(eid);
				packets(networker.aerial(), el.networker().aerial(), 0x0000ffff);
				packets(el.networker().aerial(), networker.aerial(), 0x0000ffff);
			}
		});
	}

	if (spec->networker && spec->networkHub) {
		scene.circle(pos(), spec->networkRange, 0x0000ffff, scene.pen());

		Sim::locked([&]() {
			if (!Entity::exists(id)) return;
			auto& en = Entity::get(id);
			if (en.isGhost()) return;
			auto& networker = en.networker();

			for (auto& nid: networker.links()) {
				auto& node = Networker::get(nid);
				if (node.isHub()) continue;
				packets(networker.aerial(), node.aerial(), 0x0000ffff);
				packets(node.aerial(), networker.aerial(), 0x0000ffff);
			}

			std::map<uint,std::set<uint>> web;

			miniset<uint> done;
			miniset<uint> nest = {en.id};

			while (nest.size()) {
				auto nid = nest.shift();
				done.insert(nid);

				auto& eh = Entity::get(nid);
				if (eh.pos().distance(scene.position) > Config::window.levelsOfDetail[Part::VLD]) continue;

				auto& hub = Networker::get(nid);
				for (auto& nid: hub.links()) {
					auto& node = Networker::get(nid);
					if (!node.isHub()) continue;
					if (done.has(nid)) continue;

					web[hub.id].insert(node.id);
					web[node.id].insert(hub.id);
					nest.insert(nid);
				}
			}

			for (auto& [nid,links]: web) {
				for (auto& lid: links) {
					packets(Networker::get(nid).aerial(), Networker::get(lid).aerial(), 0xffff00ff);
				}
			}
		});
	}

	if (full && spec->unveyor) {
		Sim::locked([&]() {
			if (Entity::exists(id)) {
				auto& en = Entity::get(id);
				auto& unveyor = en.unveyor();
				if (unveyor.partner && Entity::exists(unveyor.partner)) {
					Entity& sib = Entity::get(unveyor.partner);
					scene.cuboid(en.selectionCuboid(), 0x00ff00ff, scene.pen());
					scene.cuboid(sib.selectionCuboid(), 0x00ff00ff, scene.pen());
					scene.line(en.pos(), sib.pos(), 0x00ff00ff, scene.pen());
					return;
				}
			}
			auto link = spec->unveyorEntry ? dir(): -dir();
			scene.line(pos(), pos() + (link * spec->unveyorRange), 0x00ff00ff, scene.pen());
		});
	}

	if (spec->pipe && detail) {
		Sim::locked([&]() {
			if (!Entity::exists(id)) return;
			auto& en = Entity::get(id);
			auto& pipe = en.pipe();
			if (!pipe.network) return;

			for (auto id: pipe.network->pipes) {
				Entity& sib = Entity::get(id);
				scene.cuboid(sib.selectionCuboid(), 0x00ff00ff, scene.pen());
			}
		});
	}

	if (spec->pipe && spec->pipeUnderground) {
		Sim::locked([&]() {
			if (Entity::exists(id)) {
				auto& en = Entity::get(id);
				auto& pipe = en.pipe();
				if (pipe.partner && Entity::exists(pipe.partner)) {
					Entity& sib = Entity::get(pipe.partner);
					scene.cuboid(en.selectionCuboid(), 0x00ff00ff, scene.pen());
					scene.cuboid(sib.selectionCuboid(), 0x00ff00ff, scene.pen());
					scene.line(en.pos(), sib.pos(), 0x00ff00ff, scene.pen());
					return;
				}
			}
			scene.line(pos(), pos() + (dir() * spec->pipeUndergroundRange), 0x00ff00ff, scene.pen());
		});
	}

	if (spec->pipe && spec->pipeUnderwater) {
		Sim::locked([&]() {
			if (Entity::exists(id)) {
				auto& en = Entity::get(id);
				auto& pipe = en.pipe();
				if (pipe.partner && Entity::exists(pipe.partner)) {
					Entity& sib = Entity::get(pipe.partner);
					scene.cuboid(en.selectionCuboid(), 0x00ff00ff, scene.pen());
					scene.cuboid(sib.selectionCuboid(), 0x00ff00ff, scene.pen());
					scene.line(en.pos(), sib.pos(), 0x00ff00ff, scene.pen());
					return;
				}
			}
			scene.line(pos(), pos() + (dir() * spec->pipeUnderwaterRange), 0x00ff00ff, scene.pen());
		});
	}

	if (spec->effector && detail) {
		Sim::locked([&]() {
			auto en = Entity::find(id);
			if (!en) return;
			auto& effector = en->effector();
			auto et = Entity::find(effector.tid);
			if (!et) return;
			scene.cuboid(et->selectionCuboid(), 0xffffffff, scene.pen());
		});
	}

	if (spec->cartWaypoint) {
		Sim::locked([&]() {
			auto en = Entity::find(id);
			if (!en) return;

			real dist = 0.0;
			if (cartWaypoint.relative[CartWaypoint::Red] != Point::Zero)
				dist = std::max(dist, cartWaypoint.relative[CartWaypoint::Red].length());
			if (cartWaypoint.relative[CartWaypoint::Blue] != Point::Zero)
				dist = std::max(dist, cartWaypoint.relative[CartWaypoint::Blue].length());
			if (cartWaypoint.relative[CartWaypoint::Green] != Point::Zero)
				dist = std::max(dist, cartWaypoint.relative[CartWaypoint::Green].length());

			auto touches = [&](Entity* ew, Point rel) {
				bool aligned = ew->box().intersectsRay(en->pos(), rel.normalize());
				bool samedir = (ew->pos() - en->pos()).normalize().distance(rel.normalize()) < 0.5;
				bool closer = rel.length() > en->pos().distance(ew->pos());
				return aligned && samedir && closer;
			};

			auto targets = [&](Entity* ew, Point rel) {
				return ew->box().intersects((en->pos() + rel).box().grow(0.1));
			};

			auto nearby = Entity::intersecting(en->pos().sphere().grow(dist), Entity::gridCartWaypoints);

			for (auto ew: nearby) {
				auto check = [&](Entity* ew, Point rel) {
					if (rel == Point::Zero) return;
					if (targets(ew, rel)) {
						scene.cuboid(ew->selectionCuboid(), 0x00ff00ff);
					}
					else
					if (touches(ew, rel)) {
						scene.cuboid(ew->selectionCuboid(), 0xff0000ff);
					}
				};
				check(ew, cartWaypoint.relative[CartWaypoint::Red]);
				check(ew, cartWaypoint.relative[CartWaypoint::Blue]);
				check(ew, cartWaypoint.relative[CartWaypoint::Green]);
			}

			auto pos = this->pos();
			if (cartWaypoint.relative[CartWaypoint::Red] != Point::Zero)
				waypointLine(CartWaypoint::Red, pos, pos + cartWaypoint.relative[CartWaypoint::Red], true);
			if (cartWaypoint.relative[CartWaypoint::Blue] != Point::Zero)
				waypointLine(CartWaypoint::Blue, pos, pos + cartWaypoint.relative[CartWaypoint::Blue], true);
			if (cartWaypoint.relative[CartWaypoint::Green] != Point::Zero)
				waypointLine(CartWaypoint::Green, pos, pos + cartWaypoint.relative[CartWaypoint::Green], true);
		});
	}

	if (spec->cart) {
		Sim::locked([&]() {
			auto en = Entity::find(id);
			if (!en) return;
			auto& cart = en->cart();
			for (auto& point: cart.sensors()) {
				scene.sphere(Sphere(point, 0.1), 0xff0000ff);
			}
		});
	}

	if (spec->balancer && detail) {
		Sim::locked([&]() {
			auto en = Entity::find(id);
			if (!en) return;
			auto& balancer = en->balancer();
			if (!balancer.group) return;
			for (auto& member: balancer.group->members) {
				auto eb = Entity::find(member.balancer->id);
				if (!eb) continue;
				scene.cuboid(eb->selectionCuboid(), 0x00ff00ff, scene.pen());
			}
		});
	}

	if (spec->monorail) {
		auto trx = dir().rotation() * (pos() + scene.offset).translation();

		if (spec->monorailStop) {
			auto m0 = Point::West.rotation() * (spec->monorailStopFill + Point::East*2.0 + Point::Down).translation() * trx;
			scene.bits.chevron->instance(scene.shader.glow.id(), m0, input, 0.0f, Part::NOSHADOW);

			auto m1 = Point::West.rotation() * (spec->monorailStopEmpty + Point::West*2.0 + Point::Down).translation() * trx;
			scene.bits.chevron->instance(scene.shader.glow.id(), m1, output, 0.0f, Part::NOSHADOW);

			Point p0 = spec->monorailStopFill.transform(dir().rotation()) + pos();
			scene.cuboid(Cuboid(Box(p0, Volume(3,3,7)), dir()), 0xffffffff);

			Point p1 = spec->monorailStopEmpty.transform(dir().rotation()) + pos();
			scene.cuboid(Cuboid(Box(p1, Volume(3,3,7)), dir()), 0xffffffff);
		}

		auto m2 = (spec->monorailArrive + Point::Up*0.3).translation() * trx;
		scene.bits.chevron->instance(scene.shader.glow.id(), m2, input, 0.0f, Part::NOSHADOW);

		auto m3 = (spec->monorailDepart + Point::Up*0.3).translation() * trx;
		scene.bits.chevron->instance(scene.shader.glow.id(), m3, output, 0.0f, Part::NOSHADOW);

		Sim::locked([&]() {
			auto en = Entity::find(id);
			if (!en) return;

			for (auto oid: en->monorail().out) {
				auto eo = Entity::find(oid);
				if (!eo) continue;
				scene.cuboid(eo->selectionCuboid(), 0x00ff00ff);
			}
		});
	}

	if (spec->explosive) {
		scene.circle(pos().floor(0.1), spec->explosionSpec->explosionRadius, 0xff0000ff, scene.pen());
	}

	if (spec->powerpole) {
		Point p = ground() + (Point::Up * 0.05);
		scene.square(p, spec->powerpoleCoverage, 0xffffffff, scene.pen());
		scene.circle(p, spec->powerpoleRange, Config::window.grid, scene.pen());

		Sim::locked([&]() {
			auto en = Entity::find(id);
			if (!en) return;

			for (auto es: Entity::intersecting(en->powerpole().coverage())) {
				if (en->id == es->id) continue;
				if (es->spec->consumeElectricity || es->spec->generateElectricity || es->spec->consumeCharge)
					scene.cuboid(es->cuboid(), 0xffffffff, scene.pen());
			}
		});
	}
}

void GuiEntity::overlayDirecting() {

	auto groundMarker = [&](Point target) {
		auto elevation = world.elevation(target);
		target = target.floor(std::max(0.0f, elevation));

		scene.circle(target, 0.5f, 0xff9900ff, scene.pen());
		scene.circle(target, 1.0f, 0xff9900ff, scene.pen());

		float offset = (float)(Sim::tick%60)/60.0f;
		if (offset > 0.5f) offset = 0.5f-(offset-0.5f);

		Point n = target + Point::North * offset;
		Point s = target + Point::South * offset;
		Point e = target + Point::East  * offset;
		Point w = target + Point::West  * offset;

		scene.line(n, n+Point::North, 0xff9900ff, scene.pen());
		scene.line(s, s+Point::South, 0xff9900ff, scene.pen());
		scene.line(e, e+Point::East,  0xff9900ff, scene.pen());
		scene.line(w, w+Point::West,  0xff9900ff, scene.pen());
	};

	if (scene.directing && id == scene.directing->id) {
		Sim::locked([&]() {
			if (!Entity::exists(id)) return;
			auto& en = Entity::get(id);
			if (en.spec->zeppelin) {
				auto& zeppelin = en.zeppelin();
				if (zeppelin.moving) {
					groundMarker(zeppelin.target);
					return;
				}
			}
			if (en.spec->flightPath) {
				auto& flight = en.flightPath();
				if (flight.destination.distance(pos()) > 1.0) {
					groundMarker(flight.destination);
					return;
				}
			}
			if (en.spec->cart) {
				auto& cart = en.cart();
				if (cart.target.distance(pos()) > 1.0) {
					groundMarker(cart.target);
					return;
				}
			}
			groundMarker(en.pos());
		});
	}
}

void GuiEntity::overlayRouting() {
	if (spec->cartWaypoint && scene.placing && scene.placing->entities.size() == 1 && scene.placing->entities.front()->spec->cartWaypoint) {
		waypointLine(cartWaypointLine, pos(), scene.placing->entities.front()->pos());
	}

	if (spec->monorail && scene.placing && scene.placing->entities.size() == 1 && scene.placing->entities.front()->spec->monorail) {
		auto other = scene.placing->entities.front();
		auto rail = spec->railTo(pos(), dir(), other->spec, other->pos(), other->dir());
		monorailLink(monorailLine, rail);
	}
}

void GuiEntity::overlayAlignment() {
	if (!Config::mode.alignment) return;
	if (spec->pipeUnderground || spec->pipeUnderwater) return;

	float far = 1000.0;
	float near = 10.0;

	Color color = Config::window.grid;

	auto rot = dir().rotation();
	Point south = Point::South.transform(rot);
	Point north = Point::North.transform(rot);
	Point west = Point::West.transform(rot);
	Point east = Point::East.transform(rot);
	Point origin = pos() + Point::Down*(spec->collision.h/2.0) + Point::Up*0.05;

	if (spec->conveyor || spec->monorail || spec->cart || spec->monocar || spec->arm) {
		Color in = 0xddddddff;
		Color out = 0xdddd00ff;

		scene.line(origin + south*near, origin + south*far, color);
		scene.line(origin + north*near, origin + north*far, color);
		scene.line(origin + east*near, origin + east*far, color);
		scene.line(origin + west*near, origin + west*far, color);

		enum { S = 0, N, E, W };
		Color cardinals[4] = {out, in, color, color};

		if (spec->conveyor) {
			cardinals[S] = color;
			cardinals[N] = color;
			cardinals[E] = color;
			cardinals[W] = color;
			Point input = spec->conveyorInput.floor(0.0).normalize();
			Point output = spec->conveyorOutput.floor(0.0).normalize();
			if (input == Point::South) cardinals[S] = in;
			if (input == Point::North) cardinals[N] = in;
			if (input == Point::East) cardinals[E] = in;
			if (input == Point::West) cardinals[W] = in;
			if (output == Point::South) cardinals[S] = out;
			if (output == Point::North) cardinals[N] = out;
			if (output == Point::East) cardinals[E] = out;
			if (output == Point::West) cardinals[W] = out;
		}

		scene.line(origin, origin + south*near, cardinals[S]);
		scene.line(origin, origin + north*near, cardinals[N]);
		scene.line(origin, origin + east*near, cardinals[E]);
		scene.line(origin, origin + west*near, cardinals[W]);
	}
	else {
		scene.line(origin, origin + south*far, color);
		scene.line(origin + north*far, origin, color);
		scene.line(origin + west*far, origin + east*far, color);
	}

	scene.line(origin + north*far + west*far, origin + south*far + east*far, color);
	scene.line(origin + south*far + west*far, origin + north*far + east*far, color);
}

void GuiEntity::icon() {
	if (flags & ELECTRICITY) {
		scene.alert(scene.icon.electricity, spec->iconPoint(pos(), dir()));
		return;
	}

	if (flags & BLOCKED) {
		scene.warning(scene.icon.exclaim, spec->iconPoint(pos(), dir()));
		return;
	}
}

Color GuiEntity::cartRouteColor(int line) {
	Color color = 0xaa0000ff;
	if (line == CartWaypoint::Blue) color = 0x0000ddff;
	if (line == CartWaypoint::Green) color = 0x006600ff;
	return color;
}

Color GuiEntity::monorailRouteColor(int line) {
	Color color = 0xaa0000ff;
	if (line == Monorail::Blue) color = 0x0000ddff;
	if (line == Monorail::Green) color = 0x006600ff;
	return color;
}

void GuiEntity::waypointLine(int line, Point a, Point b, bool wide) {
	if (a == b) return;
	Color color = cartRouteColor(line);
	Point bump = Point::Up*0.02;
	if (line == CartWaypoint::Blue) bump = bump + Point::East*0.15;
	if (line == CartWaypoint::Green) bump = bump + Point::West*0.15;
	Mat4 rotate = (b-a).normalize().rotation();
	bump = bump.transform(rotate);
	float width = std::max(wide ? 16.0f: 8.0f, scene.pen());
	scene.line(a + bump, b + bump, color, width, 0.005f);
}

void GuiEntity::monorailLink(int line, Rail rail, bool routeOnly, bool check) {
	Point pA = rail.p0 - (rail.p1-rail.p0).normalize();
	Point pB = rail.p3 + (rail.p3-rail.p2).normalize();

	Point last = pA;
	Color base = check && !spec->railOk(rail) ? 0xFF0000FF : 0x90A4BEff;
	Color color = monorailRouteColor(line);
	float width = std::max(16.0f, scene.pen());

	// Girder blocks are offset below the rail's bezier curve, so when the
	// curve deflects vertically they need to rotate their up vector so they
	// offset evenly away from the curve.

	auto girder = [&](Point a, Point b) {
		if (routeOnly) return;
		auto c = b-a;
		auto scale = Mat4::scale(0.99,0.99,c.length()*1.1);
		auto rotate = c.rotation();

		auto bump = Point::Down.transform(rotate)*0.5;
		a = a + bump;
		b = b + bump;

		auto translate = (a + (c * 0.5) + scene.offset).translation();
		bool shadow = a.distance(scene.position) < Config::window.levelsOfDetail[Part::MD];

		scene.unit.mesh.cube->instance(ghost ? scene.shader.ghost.id(): scene.shader.part.id(), scale * rotate * translate, base, 16.0, shadow);
	};

	auto u = Point::Up*0.1;
	if (line == Monorail::Blue) u = Point::Up*0.12;
	if (line == Monorail::Green) u = Point::Up*0.14;

	auto s = Point::Zero;
	if (line == Monorail::Blue) s = Point::West*0.25;
	if (line == Monorail::Green) s = Point::East*0.25;

	// Route line are offset above the rail's bezier curve, and horizontally
	// depending on colour. When the curve deflects vertically they need to
	// rotate their up vector so they offset evenly away from the rail, and
	// their horizontal vector (east or west) when the rail turns a corner.

	auto route = [&](Point a, Point b) {
		float penW = width;
		float penH = 4.0f;
		auto c = b-a;
		auto scale = Mat4::scale(penW,penH,c.length());
		auto rotate = c.rotation();

		auto bump = u.transform(rotate) + s.transform(rotate);
		a = a + bump;
		b = b + bump;

		auto translate = (a + (c * 0.5) + scene.offset).translation();
		scene.unit.mesh.line->instance(scene.shader.ghost.id(), scale * rotate * translate, color);
	};

	if (rail.straight()) {
		girder(pA, pB);
		route(pA, pB);
		return;
	}

	int inc = 25;
	real distance = pos().distance(scene.position);

	if (distance < Config::window.levelsOfDetail[Part::MD]) inc = 10;
	if (distance < Config::window.levelsOfDetail[Part::HD]) inc = 4;

	for (int i = 0; i <= 100; i += inc) {
		Point p = rail.point((float)i/100.0);
		girder(last, p);
		route(last, p);
		last = p;
	}

	girder(last, pB);
	route(last, pB);
}

bool GuiEntity::isEnabled() const {
	if (!spec->enable) return true;
	return flags & Entity::ENABLED ? true:false;
}

// GuiFakeEntity

GuiFakeEntity::GuiFakeEntity(Spec* spec) : GuiEntity() {
	id = 0;
	this->spec = spec;
	dir(Point::South);
	state = 0;
	ghost = true;
	move((Point){0,0,0});
	settings = nullptr;
}

GuiFakeEntity::~GuiFakeEntity() {
	delete settings;
	settings = nullptr;
}

GuiFakeEntity* GuiFakeEntity::getConfig(Entity& en, bool plan) {
	if (en.spec != spec) {
		scene.exclaim(en.spec->iconPoint(en.pos(), en.dir()));
		return this;
	}
	delete settings;
	settings = en.settings();
	status = settings->enabled ? Status::None: Status::Warning;
	color = settings->color;
	if (!plan && !settings->applicable) {
		scene.exclaim(en.spec->iconPoint(en.pos(), en.dir()));
		return this;
	}
	if (settings->applicable) {
		scene.tick(en.spec->iconPoint(en.pos(), en.dir()));
	}
	return this;
}

GuiFakeEntity* GuiFakeEntity::setConfig(Entity& en, bool plan) {
	if (!plan && (en.spec != spec || !settings)) {
		scene.exclaim(en.spec->iconPoint(en.pos(), en.dir()));
		return this;
	}
	if (en.setup(settings)) {
		scene.tick(en.spec->iconPoint(en.pos(), en.dir()));
	}
	return this;
}

GuiFakeEntity* GuiFakeEntity::move(Point p) {
	pos(spec->aligned(p, dir()));
	updateTransform();
	return this;
}

GuiFakeEntity* GuiFakeEntity::move(Point p, Point d) {
	pos(spec->aligned(p, d));
	dir(d);
	updateTransform();
	return this;
}

GuiFakeEntity* GuiFakeEntity::move(float x, float y, float z) {
	return move(Point(x, y, z));
}

GuiFakeEntity* GuiFakeEntity::floor(float level) {
	return move(Point(pos().x, spec->underground ? level - spec->collision.h/2.0f: level + spec->collision.h/2.0f, pos().z));
}

GuiFakeEntity* GuiFakeEntity::rotate() {
	if (spec->rotateGhost) {
		bool done = false;
		for (int i = 0, l = spec->rotations.size()-1; i < l; i++) {
			if (spec->rotations[i] == dir()) {
				dir(spec->rotations[i+1]);
				done = true;
				break;
			}
		}
		if (!done) dir(spec->rotations[0]);
		pos(spec->aligned(pos(), dir()));
		updateTransform();

		if (spec->cartWaypoint && settings) {
			auto rot = Mat4::rotateY(glm::radians(90.0f));
			settings->cartWaypoint->dir = settings->cartWaypoint->dir.transform(rot);
		}

		if (spec->tube && settings) {
			auto rot = Mat4::rotateY(glm::radians(-90.0f));
			settings->tube->target = settings->tube->target.transform(rot);
		}
	}
	return this;
}

GuiFakeEntity* GuiFakeEntity::update() {
	// placing a series of linked tubes
	if (spec->tube && scene.connecting && scene.connecting->spec->tube && scene.placing && scene.placing->entities.size() == 1 && scene.placing->entities[0] == this) {
		if (!tube)
			tube = new tubeState;
		tube->origin = Point::Zero;
		tube->target = Point::Zero;
		tube->length = 0;
		tube->stuff.clear();
		auto target = pos() + (Point::Up * spec->tubeOrigin);
		auto origin = scene.connecting->pos() + (Point::Up * scene.connecting->spec->tubeOrigin);
		uint length = std::floor(origin.distance(target) * 1000.0f); // mm
		if (length < spec->tubeSpan) {
			tube->origin = origin;
			tube->target = target;
			tube->length = length;
			tube->stuff.clear();
		}
	}

	// placing a plan with linked tubes
	if (spec->tube && scene.placing && settings && settings->tube) {
		if (!tube)
			tube = new tubeState;
		tube->origin = pos() + (Point::Up * spec->tubeOrigin);
		tube->target = tube->origin + settings->tube->target;
		tube->length = std::floor(tube->origin.distance(tube->target) * 1000.0f);
		tube->stuff.clear();
	}

	// placing a plan with linked waypoints
	if (spec->cartWaypoint && scene.placing && settings && settings->cartWaypoint) {
		auto rotA = settings->cartWaypoint->dir.rotation();
		auto rotB = dir().rotation();

		cartWaypoint.relative[CartWaypoint::Red] = settings->cartWaypoint->relative[CartWaypoint::Red];
		cartWaypoint.relative[CartWaypoint::Blue] = settings->cartWaypoint->relative[CartWaypoint::Blue];
		cartWaypoint.relative[CartWaypoint::Green] = settings->cartWaypoint->relative[CartWaypoint::Green];

		auto rot = [&](Point p) {
			if (p == Point::Zero) return p;
			return p.transform(rotA.invert()).transform(rotB);
		};

		cartWaypoint.relative[CartWaypoint::Red] = rot(cartWaypoint.relative[CartWaypoint::Red]);
		cartWaypoint.relative[CartWaypoint::Blue] = rot(cartWaypoint.relative[CartWaypoint::Blue]);
		cartWaypoint.relative[CartWaypoint::Green] = rot(cartWaypoint.relative[CartWaypoint::Green]);
	}

	// placing a series of linked monorail towers
	if (spec->monorail && scene.connecting && scene.connecting->spec->monorail && scene.placing && scene.placing->entities.size() == 1 && scene.placing->entities[0] == this) {
		if (!monorail) monorail = new monorailState;
		monorail->railsOut = {{
			.rail = scene.connecting->spec->railTo(scene.connecting->pos(), scene.connecting->dir(), spec, pos(), dir()),
			.line = Monorail::Red,
		}};
	}

	// placing linked power poles
	if (spec->powerpole) {
		Sim::locked([&]() {
			if (!powerpole)
				powerpole = new powerpoleState;

			powerpole->wires.clear();
			powerpole->point = pos() + Point::Up*(spec->collision.h/2);

			auto range = Cylinder(powerpole->point, spec->powerpoleRange+0.001, 1000);
			for (auto se: Entity::intersecting(range)) {
				if (se->id == id) continue;
				if (!se->spec->powerpole) continue;
				auto& sibling = PowerPole::get(se->id);
				if (!sibling.range().contains(powerpole->point)) continue;
				if (!range.contains(sibling.point())) continue;
				powerpole->wires.push_back({sibling.point(), powerpole->point < sibling.point()});
			}
		});
	}

	return this;
}

bool GuiFakeEntity::isEnabled() const {
	if (!spec->enable) return true;
	return (!settings || settings->enabled) ? true: false;
}

GuiFakeEntity& GuiFakeEntity::setEnabled(bool state) {
	if (!spec->enable) state = true;
	if (!settings) settings = new Entity::Settings();
	settings->enabled = state;
	status = settings->enabled ? Status::None: Status::Warning;
	return *this;
}

