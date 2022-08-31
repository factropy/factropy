#include "enemy.h"
#include "sim.h"
#include "scenario.h"

// The Enemy system uses the scenario to spawn enemy entities like missiles.

void Enemy::tick() {
	if (!trigger && !scenario->attack()) return;

	if (enable && (trigger || Sim::tick%(60*60*1) == 0)) {

		localvec<uint> targets = {Entity::enemyTargets.begin(), Entity::enemyTargets.end()};
		discard_if(targets, [&](auto id) { return Entity::get(id).isGhost(); });
		std::shuffle(targets.begin(), targets.end(), *Sim::urng());

		float hours = Sim::tick/(60*60*60);
		float scenarioFactor = scenario->attackFactor();

		float factor = hours/10.0f + scenarioFactor;
		if (trigger) factor = std::max(1.0f, factor);
		notef("attack factor %f (hours %f, scenario %f, trigger %s)", factor, hours, scenarioFactor, trigger ? "true":"false");

		float h = 0.75;
		float v = 1.25;
		auto dir = Point(Sim::random()*h, Sim::random()*v, Sim::random()*h).normalize();

		for (int i = 0, l = Sim::choose(std::ceil(factor))+1; i < l; i++) spawn(targets, dir);
	}

	trigger = false;
}

void Enemy::spawn(localvec<uint>& targets, Point dir) {
	for (auto [_,spec]: Spec::all) {
		if (!spec->enemy || !spec->missile) continue;
		if (!targets.size()) break;

		auto tid = targets.back();
		targets.pop_back();

		auto& te = Entity::get(tid);
		auto altitude = (Sim::random()*0.25)*100.0f+300.0f;

		Entity& me = Entity::create(Entity::next(), spec);
		me.missile().attack(tid);
		me.move(te.pos() + (dir * altitude));
		me.materialize();
		return;
	}
}
