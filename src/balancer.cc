#include "common.h"
#include "sim.h"
#include "balancer.h"

// Balancer components link up with Conveyors to balance items.

void Balancer::reset() {
	for (auto group: BalancerGroup::all) delete group;
	BalancerGroup::all.clear();
	BalancerGroup::changed.clear();
	all.clear();
}

void Balancer::tick() {
	if (changed.size() || BalancerGroup::changed.size()) {
		hashset<Balancer*> affected;

		// When one or more balancers have changed (placed, rotated, removed)
		// flood-fill from that position to find the affected groups, and
		// rebuild only as required.
		std::function<void(uint id)> floodAffected;

		floodAffected = [&](uint id) {
			if (!all.has(id)) return;
			Balancer* balancer = &get(id);

			if (!balancer->managed) return;
			if (affected.has(balancer)) return;

			affected.insert(balancer);

			auto eb = &Entity::get(id);
			for (auto eo: Entity::intersecting(eb->box().grow(0.5f))) {
				if (eb->id != eo->id && eo->spec->balancer) floodAffected(eo->id);
			}
		};

		for (uint id: changed) {
			floodAffected(id);
		}

		for (auto group: BalancerGroup::changed) {
			for (auto& member: group->members) {
				floodAffected(member.balancer->id);
			}
		}

		hashset<BalancerGroup*> affectedGroups;

		for (auto group: BalancerGroup::changed) {
			affectedGroups.insert(group);
		}

		for (auto balancer: affected) {
			if (balancer->group) {
				affectedGroups.insert(balancer->group);
			}
		}

		for (auto group: affectedGroups) {
			for (auto& member: group->members) {
				ensure(affected.has(&get(member.balancer->id)));
			}
			delete group;
			BalancerGroup::all.erase(group);
		}

		for (auto balancer: affected) {
			balancer->group = nullptr;
		}

		for (auto balancer: affected) {
			if (balancer->group) continue;

			auto group = new BalancerGroup;
			BalancerGroup::all.insert(group);

			std::function<void(Balancer& sibling)> floodGroup;

			floodGroup = [&](Balancer& sibling) {
				if (!sibling.managed) return;
				if (!affected.has(&sibling)) return;
				for (auto& member: group->members) {
					if (member.balancer->id == sibling.id) return;
				}
				auto es = &Entity::get(sibling.id);

				ensure(!sibling.group);
				sibling.group = group;
				BalancerGroup::Member member;
				member.balancer = &sibling;
				group->members.push(member);

				for (auto eo: Entity::intersecting(sibling.left().box().grow(0.1))) {
					if (eo->id == es->id) continue;
					if (eo->dir() != es->dir()) continue;
					if (!eo->spec->balancer) continue;
					floodGroup(eo->balancer());
				}

				for (auto eo: Entity::intersecting(sibling.right().box().grow(0.1))) {
					if (eo->id == es->id) continue;
					if (eo->dir() != es->dir()) continue;
					if (!eo->spec->balancer) continue;
					floodGroup(eo->balancer());
				}
			};

			floodGroup(*balancer);
		}

		for (auto balancer: affected) {
			ensure(balancer->group);
		}

		for (auto group: BalancerGroup::all) {
			ensure(group->members.size());
			for (auto& member: group->members) {
				ensure(member.balancer->managed);
				ensure(member.balancer->group == group);
				ensure(!Entity::get(member.balancer->id).isGhost());
			}
		}

		notef("balancer rebuild: changed: %llu, affected: %llu, changedGroups: %llu",
			changed.size(), affected.size(), BalancerGroup::changed.size()
		);

		changed.clear();
		BalancerGroup::changed.clear();
	}

	for (auto group: BalancerGroup::all) {

		minivec<BalancerGroup::Member*> priorityIn;
		minivec<BalancerGroup::Member*> priorityOut;
		minivec<BalancerGroup::Member*> filtered;
		minivec<BalancerGroup::Member*> all;

		for (auto& member: group->members) {
			member.conveyor = &member.balancer->en->conveyor();

			if (member.balancer->priority.input) priorityIn.push(&member);
			if (member.balancer->priority.output) priorityOut.push(&member);
			if (member.balancer->filter.size()) filtered.push(&member);
			all.push(&member);

			auto conveyor = member.conveyor;
			if (conveyor->left.items[1].iid) conveyor->left.items[1].offset = std::max(conveyor->left.items[1].offset, (uint16_t)1);
			if (conveyor->right.items[1].iid) conveyor->right.items[1].offset = std::max(conveyor->right.items[1].offset, (uint16_t)1);
		}

		auto balanceLeft = [&](BalancerGroup::Member* out, minivec<BalancerGroup::Member*>& ins) {
			if (!out->balancer->en->isEnabled()) return;
			if (out->conveyor->left.items[0].iid) return;
			std::sort(ins.begin(), ins.end(), [](const auto a, const auto b) { return a->inLeft < b->inLeft; });
			for (auto in: ins) {
				if (!in->balancer->en->isEnabled()) continue;
				uint iid = in->conveyor->left.items[1].iid;
				if (!iid) continue;
				if (in->conveyor->left.items[1].offset > in->conveyor->left.steps/2) continue;
				if (!out->balancer->filter.size() || out->balancer->filter.has(iid)) {
					in->conveyor->removeLeftBack(iid);
					in->inLeft = Sim::tick;
					out->conveyor->insertLeft(0,iid);
					out->outLeft = Sim::tick;
					break;
				}
			}
		};

		auto balanceRight = [&](BalancerGroup::Member* out, minivec<BalancerGroup::Member*>& ins) {
			if (!out->balancer->en->isEnabled()) return;
			if (out->conveyor->right.items[0].iid) return;
			std::sort(ins.begin(), ins.end(), [](const auto a, const auto b) { return a->inRight < b->inRight; });
			for (auto in: ins) {
				if (!in->balancer->en->isEnabled()) continue;
				uint iid = in->conveyor->right.items[1].iid;
				if (!iid) continue;
				if (in->conveyor->right.items[1].offset > in->conveyor->right.steps/2) continue;
				if (!out->balancer->filter.size() || out->balancer->filter.has(iid)) {
					in->conveyor->removeRightBack(iid);
					in->inRight = Sim::tick;
					out->conveyor->insertRight(0,iid);
					out->outRight = Sim::tick;
					break;
				}
			}
		};

		auto balance = [&](minivec<BalancerGroup::Member*>& outs, minivec<BalancerGroup::Member*>& ins) {
			std::sort(outs.begin(), outs.end(), [](const auto a, const auto b) { return a->outRight < b->outRight; });
			for (auto out: outs) balanceRight(out, ins);
			std::sort(outs.begin(), outs.end(), [](const auto a, const auto b) { return a->outLeft < b->outLeft; });
			for (auto out: outs) balanceLeft(out, ins);
		};

		balance(priorityOut, priorityIn);
		balance(priorityOut, all);
		balance(filtered, priorityIn);
		balance(filtered, all);
		balance(all, priorityIn);
		balance(all, all);
	}
}

Balancer& Balancer::create(uint id) {
	ensure(!all.has(id));
	Balancer& balancer = all[id];
	balancer.id = id;
	balancer.en = &Entity::get(id);
	balancer.group = nullptr;
	balancer.managed = false;
	balancer.priority.input = false;
	balancer.priority.output = false;
	return balancer;
}

Balancer& Balancer::get(uint id) {
	return all.refer(id);
}

void Balancer::destroy() {
	all.erase(id);
}

BalancerSettings* Balancer::settings() {
	return new BalancerSettings(*this);
}

BalancerSettings::BalancerSettings(Balancer& balancer) {
	filter = balancer.filter;
	priority.input = balancer.priority.input;
	priority.output = balancer.priority.output;
}

void Balancer::setup(BalancerSettings* settings) {
	filter = settings->filter;
	priority.input = settings->priority.input;
	priority.output = settings->priority.output;
}

void Balancer::update() {
	if (en->isGhost()) return;
	if (!en->isEnabled()) return;
}

Point Balancer::left() {
	return en->spec->balancerLeft
		.transform(en->dir().rotation())
		.transform(en->pos().translation())
	;
}

Point Balancer::right() {
	return en->spec->balancerRight
		.transform(en->dir().rotation())
		.transform(en->pos().translation())
	;
}

Balancer& Balancer::manage() {
	ensure(!managed);
	ensure(!group);
	managed = true;
	changed.insert(id);
	return *this;
}

Balancer& Balancer::unmanage() {
	ensure(managed);
	managed = false;
	changed.insert(id);

	// previous rebuild may not have run yet
	if (group) {
		BalancerGroup::changed.insert(group);
		for (int i = 0, l = group->members.size(); i < l; i++) {
			if (group->members[i].balancer == this) {
				group->members.erase(i);
				break;
			}
		}
		group = nullptr;
	}

	return *this;
}
