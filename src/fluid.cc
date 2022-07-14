#include "common.h"
#include "sim.h"
#include "fluid.h"

void Fluid::reset() {
	names.clear();
	all.clear();
}

uint Fluid::next() {
	return ++sequence;
}

Fluid* Fluid::create(uint id, std::string name) {
	ensuref(name.length() < 100, "names must be less than 100 characters");
	ensuref(names.count(name) == 0, "duplicate fluid %s", name.c_str());
	ensuref(!all.has(id), "duplicate fluid id %u %s", id, name.c_str());
	Fluid& fluid = all[id];
	fluid.id = id;
	names[name] = &fluid;
	fluid.name = name;
	fluid.raw = false;
	return &fluid;
}

Fluid::Fluid() {
	liquid = Liquid::l(1);
	thermal = 0;
}

Fluid::~Fluid() {
}

Fluid* Fluid::byName(std::string name) {
	ensuref(names.count(name) == 1, "unknown fluid %s", name.c_str());
	return names[name];
}

Fluid* Fluid::get(uint id) {
	return &all.refer(id);
}

bool Fluid::manufacturable() {
	for (auto& [_,recipe]: Recipe::names) {
		if (!recipe->licensed) continue;
		for (auto [fid,_]: recipe->outputFluids) {
			if (fid == id) {
				return true;
			}
		}
	}
	return false;
}

void Fluid::produce(int count) {
	produced += count;
	production.add(Sim::tick, count);
}

void Fluid::consume(int count) {
	consumed += count;
	consumption.add(Sim::tick, count);
}

Amount::Amount() {
	fid = 0;
	size = 0;
}

Amount::Amount(std::initializer_list<uint> l) {
	auto i = l.begin();
	fid = *i++;
	size = *i++;
}

