#pragma once

// Recipes tell Crafters how to make something, which resources to consume, how much energy
// to consume and how fast to do it.

struct Recipe;

#include "glm-ex.h"
#include "item.h"
#include "fluid.h"
#include "energy.h"
#include "spec.h"
#include <map>
#include <set>
#include <vector>

struct Recipe {
	static void reset();
	static void save(const char *path);
	static void load(const char *path);
	static void saveAll(const char *path);
	static void loadAll(const char *path);

	static inline uint sequence;
	static uint next();

	static inline std::map<std::string,Recipe*> names;
	static Recipe* byName(std::string name);

	static void tick();

	bool licensed;
	bool delivery;
	std::string name;
	std::string title;
	std::vector<Part*> parts;

	std::set<std::string> tags;

	enum class Tag {
		Unknown = 0,
		None,
		Mining,
		Drilling,
		Crushing,
		Smelting,
		Crafting,
		Refining,
		Centrifuging,
	};

	Tag rateTag = Tag::Unknown;

	std::map<uint,uint> inputItems;
	std::map<uint,uint> outputItems;

	std::map<uint,uint> inputFluids;
	std::map<uint,uint> outputFluids;

	uint mine;
	uint drill;

	Energy energyUsage;
	uint fluid; // fluid color

	Recipe();
	Recipe(std::string name);
	~Recipe();

	static inline float miningRate = 1.0f;
	static inline float drillingRate = 1.0f;
	static inline float crushingRate = 1.0f;
	static inline float smeltingRate = 1.0f;
	static inline float craftingRate = 1.0f;
	static inline float refiningRate = 1.0f;
	static inline float centrifugingRate = 1.0f;

	float rate(Spec* spec);
	bool launch = false;

	Energy totalEnergy(std::vector<Recipe*>* path = nullptr);
	std::vector<Stack> totalRawItems(std::vector<Recipe*>* path = nullptr);
	std::vector<Amount> totalRawFluids(std::vector<Recipe*>* path = nullptr);

	bool manufacturable();
};
