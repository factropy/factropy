#include "goal.h"
#include "scene.h"

void Goal::reset() {
	for (auto& [_,goal]: all) delete goal;
	all.clear();
}

void Goal::tick() {
	for (auto [_,goal]: all) goal->check();
}

Goal* Goal::byName(std::string name) {
	ensure(all.count(name));
	return all[name];
}

Goal::Goal(std::string nname) {
	ensuref(!all.count(name), "duplicate goal %s", name);
	name = nname;
	all[name] = this;
	title = name;
}

Goal* Goal::current() {
	for (auto [_,goal]: all) {
		if (goal->active()) return goal;
	}
	return nullptr;
}

float Goal::overallProgress() {
	int done = 0;
	int total = all.size();
	for (auto [_,goal]: all) if (goal->met) done++;
	return std::max(0.0f, std::min(1.0f, (float)done/(float)total));
}

std::vector<uint> Goal::Rate::intervalSums(uint period, uint interval) {
	ensure(period > 0 && interval > 0 && period%interval == 0);

	auto item = Item::get(iid);

	std::vector<uint> intervals;
	intervals.resize(period/interval);

	if (Sim::tick < period) {
		uint64_t adjust = period-Sim::tick;
		for (auto& shipment: item->shipments) {
			if (shipment.tick >= Sim::tick) continue; // bounds
			ensure((shipment.tick+adjust)/interval < intervals.size());
			intervals[(shipment.tick+adjust)/interval] += shipment.count;
		}
	}
	else {
		uint64_t begin = Sim::tick-period;
		for (auto& shipment: item->shipments) {
			if (shipment.tick >= Sim::tick) continue; // bounds
			if (shipment.tick >= begin) {
				ensure((shipment.tick-begin)/interval < intervals.size());
				intervals[(shipment.tick-begin)/interval] += shipment.count;
			}
		}
	}

	return intervals;
}

float Goal::progress() {
	float sum = 0.0f;
	float div = 0;

	if (supplies.size()) {
		float suppliesSum = 0.0f;
		for (auto stack: supplies) {
			suppliesSum += std::min(1.0f, (float)Item::supplied[stack.iid] / (float)stack.size);
		}
		sum += suppliesSum / (float)supplies.size();
		div++;
	}

	if (construction.size()) {
		float constructionSum = 0.0f;
		for (auto [spec,count]: construction) {
			constructionSum += std::min(1.0f, (float)spec->count.extant / (float)count);
		}
		sum += constructionSum / (float)construction.size();
		div++;
	}

	if (productionItem.size()) {
		float productionItemSum = 0.0f;
		for (auto [iid,count]: productionItem) {
			productionItemSum += std::min(1.0f, (float)Item::get(iid)->produced / (float)count);
		}
		sum += productionItemSum / (float)productionItem.size();
		div++;
	}

	if (productionFluid.size()) {
		float productionFluidSum = 0.0f;
		for (auto [fid,count]: productionFluid) {
			productionFluidSum += std::min(1.0f, (float)Fluid::get(fid)->produced / (float)count);
		}
		sum += productionFluidSum / (float)productionFluid.size();
		div++;
	}

	for (auto rate: rates) {
		float rateSum = 0.0f;
		for (auto count: rate.intervalSums(period, interval)) {
			rateSum += std::min(1.0f, (float)count / (float)rate.count);
		}
		sum += rateSum / (float)(period / interval);
		div++;
	}

	return sum / (float)div;
}

void Goal::check() {
	if (!active()) return;

	for (auto [spec,count]: construction) {
		if (spec->count.extant < (int)count) return;
	}

	for (auto [iid,count]: productionItem) {
		if (Item::get(iid)->produced < count) return;
	}

	for (auto [fid,count]: productionFluid) {
		if (Fluid::get(fid)->produced < count) return;
	}

	for (auto stack: supplies) {
		if (Item::supplied[stack.iid] < stack.size) return;
	}

	for (auto rate: rates) {
		for (auto count: rate.intervalSums(period, interval)) {
			if (count < rate.count) return;
		}
	}

	complete();
}

bool Goal::active() {
	if (met) return false;

	for (auto goal: dependencies) {
		if (!goal->met) return false;
	}

	return true;
}

void Goal::complete() {
	if (met) return;

	met = true;
	chits += reward;

	for (auto spec: license.specs) spec->licensed = true;
	for (auto recipe: license.recipes) recipe->licensed = true;
}

void Goal::load() {
	if (!met) return;
	for (auto spec: license.specs) spec->licensed = true;
	for (auto recipe: license.recipes) recipe->licensed = true;
}

bool Goal::depends(Goal* other, std::set<Goal*>& checked) {
	// catch recursion
	if (checked.count(this)) return false;
	checked.insert(this);

	if (dependencies.count(other)) return true;
	for (auto dep: dependencies) if (dep->depends(other, checked)) return true;

	return false;
}

bool Goal::depends(Goal* other) {
	std::set<Goal*> checked;
	return depends(other, checked);
}

