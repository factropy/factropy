#include "common.h"
#include "sim.h"
#include "item.h"
#include "recipe.h"

Fuel::Fuel() {
	energy = 0;
}

Fuel::Fuel(std::string cat, Energy e) {
	category = cat;
	energy = e;
}

Stack::Stack() {
	iid = 0;
	size = 0;
}

Stack::Stack(std::initializer_list<uint> l) {
	auto i = l.begin();
	iid = *i++;
	size = *i++;
}

void Item::reset() {
	all.clear();
	names.clear();
}

uint Item::next() {
	return ++sequence;
}

Item* Item::create(uint id, std::string name) {
	ensuref(id < 65536, "max 65536 items");
	ensuref(name.length() < 100, "names must be less than 100 characters");
	ensuref(names.count(name) == 0, "duplicate item %s", name.c_str());
	ensuref(!all.has(id), "duplicate item id %u %s", id, name.c_str());
	Item& item = all[id];
	item.id = id;
	names[name] = &item;
	item.name = name;
	item.mass = Mass::kg(1);
	item.repair = 0;
	item.beltV = 0;
	item.armV = 0;
	item.droneV = 0;
	item.tubeV = 0;
	item.ammoRounds = 0;
	item.ammoDamage = 0;
	item.raw = false;
	item.show = false;
	return &item;
}

Item::Item() {
	beltV = 0;
	armV = 0;
}

Item::~Item() {
}

Item* Item::byName(std::string name) {
	ensuref(names.count(name) == 1, "unknown item %s", name.c_str());
	return names[name];
}

Item* Item::get(uint id) {
	return &all.refer(id);
}

uint Item::bestAmmo() {
	uint ammoId = 0;
	Health health = 0;
	for (auto [_,item]: names) {
		if (item->ammoRounds && item->ammoDamage > health && item->manufacturable()) {
			ammoId = item->id;
			health = item->ammoDamage;
		}
	}
	return ammoId;
}

bool Item::manufacturable() {
	if (free) return true;
	if (mining.count(id)) return true;
	for (auto& [_,recipe]: Recipe::names) {
		if (!recipe->licensed) continue;
		if (recipe->mine == id) return true;
		if (recipe->outputItems.count(id)) return true;
	}
	for (auto& [_,spec]: Spec::all) {
		if (spec->licensed && spec->source && spec->sourceItem == this) return true;
	}
	return false;
}

void Item::produce(int count) {
	produced += count;
	production.add(Sim::tick, count);
}

void Item::consume(int count) {
	consumed += count;
	consumption.add(Sim::tick, count);
}

void Item::supply(int count) {
	supplies.add(Sim::tick, count);
	supplied[id] += std::max(0,count);
	shipments.push_back({Sim::tick, (uint)count});
}

bool Item::hasLOD(float distance) {
	for (auto part: parts) if (part->distanceLOD(distance)) return true;
	return false;
}

