#include "common.h"
#include "ship.h"

void Ship::reset() {
	all.clear();
}

void Ship::tick() {
	for (auto& ship: all) ship.update();
}

Ship& Ship::create(uint id) {
	ensure(!all.has(id));
	Ship& ship = all[id];
	ship.id = id;
	ship.en = &Entity::get(id);
	ship.stage = Stage::Start;
	ship.speed = 0;
	ship.elevate = 0;
	return ship;
}

Ship& Ship::get(uint id) {
	return all.refer(id);
}

void Ship::destroy() {
	all.erase(id);
}

void Ship::update() {
	if (en->isGhost()) return;

	switch (stage) {
		case Stage::Start: {

			auto pos = en->pos();
			for (auto eo: Entity::intersecting(Box(pos, Volume(1.0, 1000.0, 1.0)))) {
				if (eo->spec->ship) continue;
				if (eo->spec->drone) continue;
				if (eo->spec->missile) continue;
				if (eo->spec->zeppelin) continue;
				if (eo->spec->flightPath) continue;
				if (eo->spec->explosion) continue;
				if (eo->spec->starship) continue;
				elevate = std::max(elevate, (float)(eo->pos().y + eo->spec->collision.h/2.0));
			}
			elevate += en->spec->collision.h;

			stage = Stage::Lift;
			break;
		}

		case Stage::Lift: {
			en->move(en->pos() + (Point::Up*0.1), en->dir());

			if (en->pos().y > elevate) {
				auto rail = Rail(
					en->pos(),
					en->dir(),
					en->pos() + en->dir()*1000.0 + Point::Up*1000.0,
					(Point::Up + en->dir()).normalize()
				);
				flight = rail.steps(1.0);
				speed = 0.2;
				stage = Stage::Fly;
			}
			break;
		}

		case Stage::Fly: {
			if (en->spec->states.size() > 1) {
				en->state++;
				if (en->state >= en->spec->states.size()) en->state = 1;
			}

			speed *= 1.01;

			if (flight.size()) {
				auto pos = en->pos();
				while (flight.size() && flight.front().distance(pos) < speed*1.5) flight.shift();
				auto dir = flight.size() ? (flight.front() - pos).normalize(): en->dir();
				en->move(pos + (dir*speed), dir);
			}
			else {
				en->remove();
			}
			break;
		}
	}
}

