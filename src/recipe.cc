#include "common.h"
#include "recipe.h"
#include "goal.h"
#include "miniset.h"
#include "minimap.h"

// Recipes tell Crafters how to make something, which resources to consume, how much energy
// to consume and how fast to do it.

void Recipe::reset() {
	for (auto& [_,recipe]: names) {
		delete recipe;
	}
	names.clear();
}

uint Recipe::next() {
	return sequence++;
}

void Recipe::tick() {
}

Recipe::Recipe(std::string name) {
	ensuref(name.length() < 100, "names must be less than 100 characters");
	ensuref(names.count(name) == 0, "duplicate recipe %s", name.c_str());
	this->name = name;
	names[name] = this;

	mine = 0;
	drill = 0;
	fluid = 0;
	delivery = false;
	energyUsage = 0;
	launch = false;
	licensed = false;
 }

Recipe::~Recipe() {
}

Recipe* Recipe::byName(std::string name) {
	ensuref(names.count(name) == 1, "unknown recipe %s", name.c_str());
	return names[name];
}

float Recipe::rate(Spec* spec) {
	if (rateTag == Tag::Unknown) {
		if (spec->recipeTags.count("mining")) {
			//return miningRate;
			rateTag = Tag::Mining;
		}
		else
		if (spec->recipeTags.count("drilling")) {
			//return drillingRate;
			rateTag = Tag::Drilling;
		}
		else
		if (spec->recipeTags.count("crushing")) {
			//return crushingRate;
			rateTag = Tag::Crushing;
		}
		else
		if (spec->recipeTags.count("smelting")) {
			//return smeltingRate;
			rateTag = Tag::Smelting;
		}
		else
		if (spec->recipeTags.count("crafting")) {
			//return craftingRate;
			rateTag = Tag::Crafting;
		}
		else
		if (spec->recipeTags.count("crafting-with-fluid")) {
			//return craftingRate;
			rateTag = Tag::Crafting;
		}
		else
		if (spec->recipeTags.count("refining")) {
			//return refiningRate;
			rateTag = Tag::Refining;
		}
		else
		if (spec->recipeTags.count("centrifuging")) {
			//return refiningRate;
			rateTag = Tag::Centrifuging;
		}
		else {
			rateTag = Tag::None;
		}
	}

	switch (rateTag) {
		case Tag::Mining: return miningRate;
		case Tag::Drilling: return drillingRate;
		case Tag::Crushing: return crushingRate;
		case Tag::Smelting: return smeltingRate;
		case Tag::Crafting: return craftingRate;
		case Tag::Refining: return refiningRate;
		case Tag::Centrifuging: return centrifugingRate;
		default: return 1.0f;
	}
}

Energy Recipe::totalEnergy(std::vector<Recipe*>* path) {
	Energy total;

	Recipe* foundRecipe = nullptr;
	uint foundCount = 0;

	auto findItemRecipe = [&](uint iid) {
		foundRecipe = nullptr;
		foundCount = 0;

		for (auto [_,matchRecipe]: names) {
			bool recursion = false;
			for (auto pathRecipe: *path) {
				recursion = pathRecipe == matchRecipe;
				if (recursion) break;
			}
			if (recursion) continue;

			for (auto [riid,rcount]: matchRecipe->outputItems) {
				if (riid == iid && rcount > 0) {
					foundRecipe = matchRecipe;
					foundCount = rcount;
					return true;
				}
			}
		}
		return false;
	};

	auto findFluidRecipe = [&](uint fid) {
		foundRecipe = nullptr;
		foundCount = 0;

		for (auto [_,matchRecipe]: names) {
			bool recursion = false;
			for (auto pathRecipe: *path) {
				recursion = pathRecipe == matchRecipe;
				if (recursion) break;
			}
			if (recursion) continue;

			for (auto [rfid,rcount]: matchRecipe->outputFluids) {
				if (rfid == fid && rcount > 0) {
					foundRecipe = matchRecipe;
					foundCount = rcount;
					return true;
				}
			}
		}
		return false;
	};

	std::vector<Recipe*> lpath;
	if (!path) path = &lpath;

	for (auto [iid,count]: inputItems) {
		if (findItemRecipe(iid)) {
			path->push_back(foundRecipe);
			total += foundRecipe->totalEnergy(path) * (float)(1.0f/(double)foundCount*(double)count);
			path->pop_back();
		}
	}

	for (auto [fid,count]: inputFluids) {
		if (findFluidRecipe(fid)) {
			path->push_back(foundRecipe);
			total += foundRecipe->totalEnergy(path) * (float)(1.0f/(double)foundCount*(double)count);
			path->pop_back();
		}
	}

	return total + energyUsage;
}

std::vector<Stack> Recipe::totalRawItems(std::vector<Recipe*>* path) {
	minimap<Stack,&Stack::iid> total;
	for (auto [iid,count]: inputItems) if (Item::get(iid)->raw) total[iid].size = count;

	minivec<Recipe*> foundRecipes;

	Recipe* foundRecipe = nullptr;
	uint foundCount = 0;

	auto findItemRecipe = [&](uint iid) {
		foundRecipes.clear();
		foundRecipe = nullptr;
		foundCount = 0;

		for (auto [_,matchRecipe]: names) {
			bool recursion = false;
			for (auto pathRecipe: *path) {
				recursion = pathRecipe == matchRecipe;
				if (recursion) break;
			}
			if (recursion) continue;

			for (auto [riid,rcount]: matchRecipe->outputItems) {
				if (riid == iid && rcount > 0) {
					foundRecipes.push_back(matchRecipe);
				}
			}
		}

		std::sort(foundRecipes.begin(), foundRecipes.end(), [](auto a, auto b) {
			return a->outputItems.size() < b->outputItems.size();
		});

		if (foundRecipes.size()) {
			foundRecipe = foundRecipes.front();
			foundCount = foundRecipe->outputItems[iid];
			return true;
		}

		return false;
	};

	std::vector<Recipe*> lpath;
	if (!path) path = &lpath;

	for (auto [iid,count]: inputItems) {
		if (findItemRecipe(iid)) {
			path->push_back(foundRecipe);
			for (auto stack: foundRecipe->totalRawItems(path)) {
				total[stack.iid].size += std::max(1U, (uint)((double)stack.size/(double)foundCount*(double)count));
			}
			path->pop_back();
		}
	}

	return {total.begin(), total.end()};
}

std::vector<Amount> Recipe::totalRawFluids(std::vector<Recipe*>* path) {
	minimap<Amount,&Amount::fid> total;
	for (auto [fid,count]: inputFluids) if (Fluid::get(fid)->raw) total[fid].size = count;

	minivec<Recipe*> foundRecipes;

	Recipe* foundRecipe = nullptr;
	uint foundCount = 0;

	auto findFluidRecipe = [&](uint fid) {
		foundRecipes.clear();
		foundRecipe = nullptr;
		foundCount = 0;

		for (auto [_,matchRecipe]: names) {
			bool recursion = false;
			for (auto pathRecipe: *path) {
				recursion = pathRecipe == matchRecipe;
				if (recursion) break;
			}
			if (recursion) continue;

			for (auto [rfid,rcount]: matchRecipe->outputFluids) {
				if (rfid == fid && rcount > 0) {
					foundRecipes.push_back(matchRecipe);
				}
			}
		}

		std::sort(foundRecipes.begin(), foundRecipes.end(), [](auto a, auto b) {
			return a->outputFluids.size() < b->outputFluids.size();
		});

		if (foundRecipes.size()) {
			foundRecipe = foundRecipes.front();
			foundCount = foundRecipe->outputFluids[fid];
			return true;
		}

		return false;
	};

	std::vector<Recipe*> lpath;
	if (!path) path = &lpath;

	for (auto [fid,count]: inputFluids) {
		if (findFluidRecipe(fid)) {
			path->push_back(foundRecipe);
			for (auto amount: foundRecipe->totalRawFluids(path)) {
				total[amount.fid].size += std::max(1U, (uint)((double)amount.size/(double)foundCount*(double)count));
			}
			path->pop_back();
		}
	}

	return {total.begin(), total.end()};
}

bool Recipe::manufacturable() {
	for (auto& [iid,_]: inputItems) {
		auto item = Item::get(iid);
		if (!item->manufacturable()) return false;
	}
	for (auto& [fid,_]: inputFluids) {
		auto fluid = Fluid::get(fid);
		if (!fluid->manufacturable()) return false;
	}
	for (auto& [_,spec]: Spec::all) {
		if (!spec->licensed) continue;
		for (auto& tag: tags) {
			if (spec->recipeTags.count(tag)) {
				return true;
			}
		}
	}
	return false;
}

