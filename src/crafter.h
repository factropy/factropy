#pragma once

// Crafter components take input materials, do some work and output different
// materials. Almost everything that assembles or mines or pumps or teleports
// is at heart a Crafter. Recipes it what to do. Specs tell it how to behave.

struct Crafter;
struct CrafterSettings;

#include "slabmap.h"
#include "item.h"
#include "fluid.h"
#include "entity.h"
#include "recipe.h"
#include "hashset.h"
#include "activeset.h"
#include <map>
#include <list>
#include <vector>
#include <deque>

struct Crafter {
	uint id;
	Entity *en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Crafter,&Crafter::id> all;
	static Crafter& create(uint id);
	static Crafter& get(uint id);

	static inline activeset<Crafter*,1> hot;
	static inline activeset<Crafter*,30> cold;

	bool working;
	float progress;
	float efficiency;
	Recipe *recipe, *changeRecipe;
	Energy energyUsed;
	uint completed;
	bool interval;
	bool transmit;
	uint64_t updatedPipes;

	void destroy();
	void update();
	CrafterSettings* settings();
	void setup(CrafterSettings*);

	void craft(Recipe* recipe);
	bool craftable(Recipe* recipe);
	void retool(Recipe* recipe);

	float speed();
	Energy consumption();

	Point output();
	float inputsProgress();
	std::vector<Point> pipeConnections();
	std::vector<Point> pipeInputConnections();
	std::vector<Point> pipeOutputConnections();

	bool exporting();
	std::vector<Stack> exportItems;
	std::vector<Amount> exportFluids;

	std::vector<uint> inputPipes;
	std::vector<uint> outputPipes;
	void updatePipes();

	minimap<Stack,&Stack::iid> inputItemsState();
	minimap<Amount,&Amount::fid> inputFluidsState();
	minimap<Stack,&Stack::iid> outputItemsState();
	minimap<Amount,&Amount::fid> outputFluidsState();

	bool inputItemsReady();
	bool inputFluidsReady();
	bool inputMiningReady();
	bool inputDrillingReady();

	bool outputItemsReady();

	bool insufficientResources();
	bool excessProducts();
};

struct CrafterSettings {
	Recipe* recipe;
	bool transmit;
	CrafterSettings(Crafter& crafter);
};
