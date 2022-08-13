 #pragma once

#include <string>
#include <map>
#include <set>
#include "sim.h"
#include "item.h"

struct Goal {
	static void reset();
	static void saveAll(const char *path);
	static void loadAll(const char *path);
	static void tick();
	static Goal* current();
	static float overallProgress();

	static inline std::map<std::string,Goal*> all;
	static Goal* byName(std::string name);

	static inline int chits = 0;

	std::string name;
	std::string title;
	std::vector<std::string> hints;
	bool met = false;

	int reward = 0;

	struct {
		std::set<Spec*> specs;
		std::set<Recipe*> recipes;
	} license;

	std::vector<Stack> supplies;
	std::map<Spec*,uint> construction;
	std::map<uint,uint> productionItem;
	std::map<uint,uint> productionFluid;
	std::set<Goal*> dependencies;

	static const uint hour = 60*60*60;
	static const uint interval = hour;
	uint period = 0;

	struct Rate {
		uint iid = 0;
		uint count = 0;
		std::vector<uint> intervalSums(uint period, uint interval);
	};

	std::vector<Rate> rates;

	Goal(std::string name);
	void check();
	bool active();
	void complete();
	void load();
	bool depends(Goal* other, std::set<Goal*>& checked);
	bool depends(Goal* other);
	float progress();
};

