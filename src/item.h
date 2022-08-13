#pragma once

// Items are materials used in construction, minable resources and/or burnable fuel.

struct Item;
struct Fuel;
struct Stack;

#include "spec.h"
#include "part.h"
#include "mass.h"
#include "energy.h"
#include "recipe.h"
#include "slabarray.h"
#include "time-series.h"
#include <map>
#include <array>
#include <vector>

struct Fuel {
	std::string category;
	Energy energy;

	Fuel();
	Fuel(std::string, Energy);
};

struct Stack {
	uint iid;
	uint size;

	Stack();
	Stack(std::initializer_list<uint>);
};

struct Item {
	uint id;
	static void saveAll(const char *path);
	static void loadAll(const char *path);
	static void reset();

	static inline uint sequence;
	static uint next();

	static inline std::map<std::string,Item*> names;
	static inline slabarray<Item,1024> all;
	static Item* create(uint id, std::string name);
	static Item* byName(std::string name);
	static Item* get(uint id);
	static uint bestAmmo();

	static inline std::map<uint,float> mining;
	static inline std::map<uint,uint> supplied;

	std::string name;
	std::string title;
	std::string ore;
	Mass mass;
	Fuel fuel;
	Health repair;
	std::vector<Part*> parts;
	float beltV;
	float armV;
	float droneV;
	float tubeV;
	uint64_t produced;
	uint64_t consumed;
	TimeSeriesSum production;
	TimeSeriesSum consumption;
	TimeSeriesSum supplies;

	struct Group {
		std::string order;
		std::vector<Item*> display;
	};

	struct Category {
		std::string title;
		std::string order;
		std::map<std::string,Group> groups;
		std::vector<Group*> display;
	};

	static inline std::map<std::string,Category> categories;
	static inline std::vector<Category*> display;

	Category* category = nullptr;
	Group* group = nullptr;
	std::string order;

	static inline bool sort(const Item* a, const Item* b) {
		return (a->category->order < b->category->order)
			|| (a->category->order == b->category->order && a->group->order < b->group->order)
			|| (a->category->order == b->category->order && a->group->order == b->group->order && a->order < b->order)
			|| (a->category->order == b->category->order && a->group->order == b->group->order && a->order == b->order && a->title < b->title)
			|| (a->category->order == b->category->order && a->group->order == b->group->order && a->order == b->order && a->title == b->title && a->name < b->name);
	}

	struct Shipment {
		uint64_t tick = 0;
		uint count = 0;
	};

	std::vector<Shipment> shipments;

	uint ammoRounds;
	Health ammoDamage;
	bool raw;
	bool free;
	bool show;

	Item();
	~Item();

	bool manufacturable();
	void produce(int count);
	void consume(int count);
	void supply(int count);
	bool hasLOD(float distance);
	miniset<Recipe*> recipes();
};
