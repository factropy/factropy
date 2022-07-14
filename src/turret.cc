#include "common.h"
#include "entity.h"
#include "turret.h"
#include "missile.h"

// Turret components scan for enemies and fire Missiles at them.

void Turret::reset() {
	all.clear();
}

void Turret::tick() {
	for (auto& turret: all) {
		turret.update();
	}
}

Turret& Turret::create(uint id) {
	ensure(!all.has(id));
	Turret& turret = all[id];
	turret.id = id;
	turret.tid = 0;
	turret.aim = Point::South;
	turret.fire = false;
	turret.cool = 0;
	turret.ammoId = 0;
	turret.ammoRounds = 0;
	turret.pause = 0;
	turret.attack = 0;

	Entity& en = Entity::get(id);
	if (!en.spec->turretLaser) {
		minivec<Item*> ammo;
		for (auto [_,item]: Item::names) {
			if (item->ammoDamage) ammo.push_back(item);
		}
		std::sort(ammo.begin(), ammo.end(), [&](auto a, auto b) {
			return a->ammoDamage < b->ammoDamage;
		});
		for (auto item: ammo) {
			en.store().levelSet(item->id, 2, 2);
		}
	}

	return turret;
}

Turret& Turret::get(uint id) {
	return all.refer(id);
}

void Turret::destroy() {
	all.erase(id);
}

void Turret::update() {
	Entity& en = Entity::get(id);
	if (en.isGhost()) return;

	if (cool > 0) {
		cool--;
		fire = false;
	}

	if (en.spec->turretStateAlternate) {
		en.state = en.state%60;
		// barrels spin when targetting
		if (tid) {
			en.state++;
			if (en.state >= 60) {
				en.state = 0;
			}
		}
	}

	if (!en.spec->turretStateAlternate) {
		// cooling rings flash when targetting
		if (tid) {
			en.state++;
			if (en.state >= en.spec->states.size()) {
				en.state = 0;
			}
		}
	}

	if (pause > Sim::tick) return;

	if (!tid) {
		for (auto et: Entity::enemiesInRange(en.pos(), en.spec->turretRange*1.25f)) {
			auto blocker = [&](Entity* eb) {
				return false;
			};

			float step = 2.0f;
			float stepSquared = step*step;
			Point a = en.pos() + en.spec->turretPivotPoint;
			Point b = et->pos();
			Point n = (b-a).normalize() * step;

			auto lineOfSight = [&]() {
				for (Point c = a; c.distanceSquared(b) > stepSquared; c += n) {
					auto elevation = world.elevation(c);
					if (elevation > c.y) return false;

					for (auto ei: Entity::intersecting(c.box().grow(0.25))) {
						if (blocker(ei)) return false;
					}
				}
				return true;
			};

			if (lineOfSight()) {
				tid = et->id;
				break;
			}
		}
	}

	if (!tid || !Entity::exists(tid)) {
		if (en.spec->turretStateRevert)
			en.state = 0;
		tid = 0;
		pause = Sim::tick + 10;
		return;
	}

	Entity& te = Entity::get(tid);

	if (!aimAt(te.pos())) {
		return;
	}

	if (!en.spec->turretLaser) {
		if (!ammoId || !ammoRounds) {
			auto& store = en.store();
			for (auto stack: store.stacks) {
				Item* item = Item::get(stack.iid);
				if (item->ammoDamage && store.remove({stack.iid,1}).size == 1) {
					ammoId = stack.iid;
					ammoRounds = item->ammoRounds;
					break;
				}
			}
		}

		if (!ammoId || !ammoRounds) {
			pause = Sim::tick + 30;
			return;
		}
	}

	if (te.pos().distance(en.pos()) > en.spec->turretRange) {
		pause = Sim::tick + 5;
		return;
	}

	if (!cool) {
		attack = damage();

		if (en.spec->turretLaser) {
			attack *= en.consumeRate(en.spec->energyConsume);
		} else {
			ammoRounds--;
		}

		if (attack > 0) {
			te.damage(attack);
			fire = true;
			cool = en.spec->turretCooldown;
		}

		if (en.spec->turretStateAlternate)
			en.state = (en.state%60)+60;
	}
}

// this doesn't account for entity rotation, so GuiEntity::loadTurret does. fix?
bool Turret::aimAt(Point p) {
	Entity& en = Entity::get(id);
	p = (p-en.pos()).normalize();
	aim = aim.pivot(p, en.spec->turretPivotSpeed);
	return aim == p;
}

uint Turret::ammo() {
	Entity& en = Entity::get(id);
	if (en.spec->turretLaser) return 0;
	if (ammoId) return ammoId;
	for (auto& stack: en.store().stacks) {
		Item* item = Item::get(stack.iid);
		if (item->ammoDamage) return stack.iid;
	}
	return 0;
}

Health Turret::damage() {
	Entity& en = Entity::get(id);
	if (en.spec->turretLaser) return en.spec->turretDamage;
	return en.spec->turretDamage * en.spec->damage(ammo());
}
