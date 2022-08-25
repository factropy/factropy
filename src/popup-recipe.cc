#include "common.h"
#include "config.h"
#include "popup.h"
#include "gui.h"

#include "../imgui/setup.h"

using namespace ImGui;

namespace {
	void openURL(const std::string& url) {
	#if defined(_WIN32)
		if (system(fmtc("explorer %s", url)) < 0)
			notef("openURL failed");
	#endif
	#if defined(__linux__) || defined(__FreeBSD__)
		// Alternatives: firefox, x-www-browser
		if (system(fmtc("xdg-open %s &", url)) < 0)
			notef("openURL failed");
	#endif
	}
}

RecipePopup::RecipePopup() : Popup() {
}

RecipePopup::~RecipePopup() {
}

void RecipePopup::prepare() {
	sorted.items.clear();
	sorted.fluids.clear();
	sorted.recipes.clear();
	sorted.specs.clear();

	for (auto& [_,item]: Item::names)
		sorted.items.push_back(item);
	for (auto& [_,fluid]: Fluid::names)
		sorted.fluids.push_back(fluid);
	for (auto& [_,recipe]: Recipe::names)
		sorted.recipes.push_back(recipe);
	for (auto& [_,spec]: Spec::all)
		if (spec->build) sorted.specs.push_back(spec);
	for (auto& [_,goal]: Goal::all)
		sorted.goals.push_back(goal);

	std::sort(sorted.items.begin(), sorted.items.end(), Item::sort);
	std::sort(sorted.fluids.begin(), sorted.fluids.end(), Fluid::sort);
	reorder(sorted.recipes, [&](auto a, auto b) { return a->title < b->title; });
	reorder(sorted.specs, [&](auto a, auto b) { return a->title < b->title; });
	reorder(sorted.goals, [&](auto a, auto b) { return b->depends(a); });
}

void RecipePopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		big();
		Begin("Build##recipe", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

		if (IsWindowAppearing()) {
			expanded.goal.clear();
			if (Goal::current()) expanded.goal[Goal::current()] = true;
		}

		float column = GetContentRegionAvail().x/4.0;

		PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));
		if (BeginTable("list", 4, ImGuiTableFlags_BordersInnerV)) {
			TableSetupColumn("##items", ImGuiTableColumnFlags_WidthFixed, column);
			TableSetupColumn("##recipes", ImGuiTableColumnFlags_WidthFixed, column);
			TableSetupColumn("##specs", ImGuiTableColumnFlags_WidthFixed, column);
			TableSetupColumn("##goals", ImGuiTableColumnFlags_WidthFixed, column);

			TableNextColumn();

				Header("Items & Fluids", false);
				if (SmallButtonInlineRight(fmtc(" %s ##%s", ICON_FA_LOCK, "toggle-items-fluids"))) showUnavailableItemsFluids = !showUnavailableItemsFluids;
				if (IsItemHovered()) tip("Toggle locked items & fluids");

				BeginChild("list-items-and-fluids"); {
					for (auto category: Item::display) {
						bool show = false;
						for (auto group: category->display) {
							for (auto item: group->display) show = show || showItem(item);
						}
						if (show) {
							Section(category->title, false);
							for (auto group: category->display) {
								for (auto item: group->display) drawItem(item);
							}
						}
					}
					bool show = false;
					for (auto& fluid: sorted.fluids) {
						show = show || showFluid(fluid);
					}
					if (show) {
						Section("Fluids", false);
						for (auto& fluid: sorted.fluids) drawFluid(fluid);
					}
				}
				EndChild();

			TableNextColumn();

				Header("Recipes", false);
				if (SmallButtonInlineRight(fmtc(" %s ##%s", ICON_FA_LOCK, "toggle-recipes"))) showUnavailableRecipes = !showUnavailableRecipes;
				if (IsItemHovered()) tip("Toggle locked recipes");

				BeginChild("list-recipes");
				for (auto& recipe: sorted.recipes) drawRecipe(recipe);
				EndChild();

			TableNextColumn();

				Header("Structures & Vehicles", false);
				if (SmallButtonInlineRight(fmtc(" %s ##%s", ICON_FA_LOCK, "toggle-specs"))) showUnavailableSpecs = !showUnavailableSpecs;
				if (IsItemHovered()) tip("Toggle locked structures & vehicles");

				BeginChild("list-specs");
				for (auto& spec: sorted.specs) drawSpec(spec);
				EndChild();

			TableNextColumn();

				Header("Goals", false);
				BeginChild("list-goals");
				for (auto& goal: sorted.goals) drawGoal(goal);
				EndChild();

			EndTable();
		}
		PopStyleVar(1);

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

bool RecipePopup::showItem(Item* item) {
	return showUnavailableItemsFluids || item->manufacturable() || item->show || expanded.item[item];
}

void RecipePopup::drawItem(Item* item) {
	if (!showItem(item)) return;

	if (locate.item == item) {
		SetScrollHereY();
		locate.item = nullptr;
	}

	int pop = 0;

	if (highlited.item[item]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if (!item->manufacturable() && !highlited.item[item]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.item[item]);
	PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.item[item] = CollapsingHeader(fmtc("##item-%d", item->id));
	bool faint = !highlited.item[item] && !IsItemHovered();
	PopStyleColor(1);

	SameLine();
	itemIcon(item);

	SameLine();
	Print(item->title.c_str());

	SameLine();
	if (faint) PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	PushFont(Config::sidebar.font.imgui);
	if (item->fuel.energy) {
		PrintRight(fmtc("%s, %s", item->fuel.energy.format(), item->mass.format()), true);
	}
	else {
		PrintRight(item->mass.format().c_str(), true);
	}
	PopFont();
	if (faint) PopStyleColor(1);

	if (expanded.item[item]) {

		miniset<Recipe*> producers;
		for (auto& [_,recipe]: Recipe::names) {
			if (recipe->mine == item->id) {
				producers.insert(recipe);
			}
			if (recipe->outputItems.count(item->id)) {
				producers.insert(recipe);
			}
		}
		if (producers.size()) {
			Section("Produced by");
			for (auto& recipe: producers) drawRecipeButton(recipe);
			SpacingV();
		}

		miniset<Recipe*> recipeConsumers;
		for (auto& [_,recipe]: Recipe::names) {
			if (recipe->inputItems.count(item->id)) {
				recipeConsumers.insert(recipe);
			}
		}

		miniset<Spec*> specConsumers;
		for (auto& [_,spec]: Spec::all) {
			for (auto [iid,_]: spec->materials) {
				if (iid == item->id) {
					specConsumers.insert(spec);
				}
			}
		}

		if (recipeConsumers.size() || specConsumers.size()) {
			Section("Consumed by");
			for (auto& recipe: recipeConsumers) drawRecipeButton(recipe);
			for (auto& spec: specConsumers) drawSpecButton(spec);
			SpacingV();
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.item[item] = false;
}

void RecipePopup::drawItemButton(Item* item) {
	Bullet();
	if (SmallButtonInline(item->title.c_str())) {
		locate.item = item;
		expanded.item[item] = !expanded.item[item];
	}
	highlited.item[item] = highlited.item[item] || IsItemHovered();
}

void RecipePopup::drawItemButtonNoBullet(Item* item) {
	if (SmallButton(item->title.c_str())) {
		locate.item = item;
		expanded.item[item] = !expanded.item[item];
	}
	highlited.item[item] = highlited.item[item] || IsItemHovered();
}

void RecipePopup::drawItemButton(Item* item, int count) {
	Bullet();
	if (SmallButtonInline(fmtc("%s(%d)", item->title, count))) {
		locate.item = item;
		expanded.item[item] = !expanded.item[item];
	}
	highlited.item[item] = highlited.item[item] || IsItemHovered();
}

void RecipePopup::drawItemButton(Item* item, int count, int limit) {
	Bullet();
	if (SmallButtonInline(fmtc("%s(%d/%d)", item->title, count, limit))) {
		locate.item = item;
		expanded.item[item] = !expanded.item[item];
	}
	highlited.item[item] = highlited.item[item] || IsItemHovered();
}

bool RecipePopup::showFluid(Fluid* fluid) {
	return showUnavailableItemsFluids || fluid->manufacturable() || expanded.fluid[fluid];
}

void RecipePopup::drawFluid(Fluid* fluid) {
	if (!showFluid(fluid)) return;

	if (locate.fluid == fluid) {
		SetScrollHereY();
		locate.fluid = nullptr;
	}

	int pop = 0;

	if (highlited.fluid[fluid]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if (!fluid->manufacturable() && !highlited.fluid[fluid]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.fluid[fluid]);
	PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.fluid[fluid] = CollapsingHeader(fmtc("##fluid-%d", fluid->id));
	PopStyleColor(1);

	SameLine();
	fluidIcon(fluid);

	SameLine();
	Print(fluid->title.c_str());

	if (expanded.fluid[fluid]) {

		miniset<Recipe*> producers;
		for (auto& [_,recipe]: Recipe::names) {
			if (recipe->outputFluids.count(fluid->id)) producers.insert(recipe);
		}

		if (producers.size()) {
			Section("Produced by");
			for (auto& recipe: producers) drawRecipeButton(recipe);
			SpacingV();
		}

		miniset<Recipe*> consumers;
		for (auto& [_,recipe]: Recipe::names) {
			if (recipe->inputFluids.count(fluid->id)) consumers.insert(recipe);
		}

		if (consumers.size()) {
			Section("Consumed by");
			for (auto& recipe: consumers) drawRecipeButton(recipe);
			SpacingV();
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.fluid[fluid] = false;
}

void RecipePopup::drawFluidButton(Fluid* fluid) {
	Bullet();
	if (SmallButtonInline(fluid->title.c_str())) {
		locate.fluid = fluid;
		expanded.fluid[fluid] = !expanded.fluid[fluid];
	}
	highlited.fluid[fluid] = highlited.fluid[fluid] || IsItemHovered();
}

void RecipePopup::drawFluidButton(Fluid* fluid, int count) {
	Bullet();
	if (SmallButtonInline(fmtc("%s(%d)", fluid->title, count))) {
		locate.fluid = fluid;
		expanded.fluid[fluid] = !expanded.fluid[fluid];
	}
	highlited.fluid[fluid] = highlited.fluid[fluid] || IsItemHovered();
}

void RecipePopup::drawRecipe(Recipe* recipe) {

	if (!showUnavailableRecipes && !recipe->licensed && !expanded.recipe[recipe]) return;

	if (locate.recipe == recipe) {
		SetScrollHereY();
		locate.recipe = nullptr;
	}

	int pop = 0;

	if (highlited.recipe[recipe]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if ((!recipe->licensed || !recipe->manufacturable()) && !highlited.recipe[recipe]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.recipe[recipe]);
	PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.recipe[recipe] = CollapsingHeader(fmtc("##recipe-%s", recipe->name));
	bool faint = !highlited.recipe[recipe] && !IsItemHovered();
	PopStyleColor(1);

	SameLine();
	recipeIcon(recipe);

	SameLine();
	Print(recipe->title.c_str());

	SameLine();
	if (faint) PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	PushFont(Config::sidebar.font.imgui);
	PrintRight(recipe->energyUsage.format(), true);
	PopFont();
	if (faint) PopStyleColor(1);

	if (expanded.recipe[recipe]) {

		if (recipe->inputItems.size() || recipe->inputFluids.size()) {
			Section("Inputs");
			for (auto [iid,count]: recipe->inputItems) drawItemButton(Item::get(iid), count);
			SpacingV();
			for (auto [fid,count]: recipe->inputFluids) drawFluidButton(Fluid::get(fid), count);
			SpacingV();
		}

		if (recipe->outputItems.size() || recipe->outputFluids.size()) {
			Section("Outputs");
			for (auto [iid,count]: recipe->outputItems) drawItemButton(Item::get(iid), count);
			SpacingV();
			for (auto [fid,count]: recipe->outputFluids) drawFluidButton(Fluid::get(fid), count);
			SpacingV();
		}

		miniset<Spec*> specs;
		for (auto& [_,spec]: Spec::all) {
			for (auto& tag: spec->recipeTags) {
				if (recipe->tags.count(tag)) specs.insert(spec);
			}
		}
		if (specs.size()) {
			Section("Made in");
			for (auto& spec: specs) drawSpecButton(spec);
			SpacingV();
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.recipe[recipe] = false;
}

void RecipePopup::drawRecipeButton(Recipe* recipe) {
	Bullet();
	if (SmallButtonInline(recipe->title.c_str())) {
		locate.recipe = recipe;
		expanded.recipe[recipe] = !expanded.recipe[recipe];
	}
	highlited.recipe[recipe] = highlited.recipe[recipe] || IsItemHovered();
}

void RecipePopup::drawSpec(Spec* spec) {

	if (!showUnavailableSpecs && !spec->licensed && !expanded.spec[spec]) return;

	if (locate.spec == spec) {
		SetScrollHereY();
		locate.spec = nullptr;
	}

	int pop = 0;

	if (highlited.spec[spec]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if (!spec->licensed && !highlited.spec[spec]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.spec[spec]);
	PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.spec[spec] = CollapsingHeader(fmtc("##build-spec-%s", spec->name));
	bool faint = !highlited.spec[spec] && !IsItemHovered();
	PopStyleColor(1);

	SameLine();
	specIcon(spec);

	SameLine();
	Print(spec->title.c_str());

	if (faint) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	}

	PushFont(Config::sidebar.font.imgui);

	Spec* dspec = spec->statsGroup;

	if (spec->unveyor) {
		SameLine();
		PrintRight(dspec->conveyorEnergyDrain.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity consumer scaled by length, reported as a belt", (dspec->energyConsume + dspec->conveyorEnergyDrain).formatRate()));
	} else
	if (spec->balancer) {
		SameLine();
		PrintRight(dspec->conveyorEnergyDrain.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity consumer, reported as a belt", (dspec->energyConsume + dspec->conveyorEnergyDrain).formatRate()));
	} else
	if ((dspec->consumeElectricity || dspec->tube) && dspec->energyConsume) {
		SameLine();
		PrintRight(dspec->energyConsume.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity consumer", (dspec->energyConsume + dspec->conveyorEnergyDrain).formatRate()));
	} else
	if (dspec->consumeFuel && dspec->energyConsume) {
		SameLine();
		PrintRight(dspec->energyConsume.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s fuel consumer", dspec->energyConsume.formatRate()));
	} else
	if (dspec->conveyorEnergyDrain) {
		SameLine();
		PrintRight(dspec->conveyorEnergyDrain.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity consumer", dspec->conveyorEnergyDrain.formatRate()));
	} else
	if (dspec->energyGenerate) {
		SameLine();
		PrintRight(dspec->energyGenerate.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity generator", dspec->energyGenerate.formatRate()));
	} else
	if (spec->depot) {
		SameLine();
		PrintRight(fmtc("x%d", spec->depotDrones), true);
		if (IsItemHovered()) tip(fmt("%d drones", spec->depotDrones));
	} else
	if (spec->tipStorage) {
		SameLine();
		PrintRight(spec->capacity.format().c_str(), true);
	} else
	if (spec->source && spec->sourceFluid) {
		SameLine();
		PrintRight(fmtc("%s/s", Liquid(spec->sourceFluidRate*60).format()), true);
	} else
	if (spec->pipe) {
		SameLine();
		PrintRight(spec->pipeCapacity.format().c_str(), true);
	} else
	if (spec->effector && spec->effectorElectricity > 0.0) {
		SameLine();
		PrintRight(fmtc("+%d%%", (int)std::floor(spec->effectorElectricity*100)), true);
		if (IsItemHovered()) tip(fmt(
			"+%d%% boost for electrical structures."
			" For crafters, higher speed."
			" For chargers, faster recharge.",
			(int)std::floor(spec->effectorElectricity*100)
		));
	} else
	if (spec->effector && spec->effectorFuel > 0.0) {
		SameLine();
		PrintRight(fmtc("+%d%%", (int)std::floor(spec->effectorFuel*100)), true);
		if (IsItemHovered()) tip(fmt(
			"+%d%% speed boost for fuel-burning structures.",
			(int)std::floor(spec->effectorFuel*100)
		));
	} else
	if (spec->bufferElectricity) {
		SameLine();
		PrintRight(spec->consumeChargeBuffer.format().c_str(), true);
	} else
	if (spec->turret && spec->turretLaser) {
		SameLine();
		PrintRight(fmtc("%dd", spec->damage()), true);
		if (IsItemHovered()) tip(
			"Damage per tick until stored energy is depleted."
			" Recharge rate can be boosted with battery packs."
		);
	} else
	if (spec->turret) {
		uint ammoId = Item::bestAmmo();
		// may not be unlocked
		if (ammoId) {
			SameLine();
			PrintRight(fmtc("%dd", spec->damage(ammoId)), true);
			if (IsItemHovered()) tip(fmt(
				"Damage per shot with %u tick cooldown using: %s",
				spec->turretCooldown,
				Item::get(ammoId)->title
			));
		}
	} else
	if (spec->consumeCharge && spec->energyConsume) {
		SameLine();
		PrintRight(spec->energyConsume.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity consumer", spec->energyConsume.formatRate()));
	}

	PopFont();

	if (faint) {
		PopStyleColor(1);
	}

	if (expanded.spec[spec]) {

		if (spec->licensed) {
			if (BeginTable(fmtc("##buttons-%s", spec->name), spec->wiki.size() ? 3: 2)) {
				TableNextColumn();
				if (Button(fmtc("build##%s", spec->name), ImVec2(-1,0))) {
					scene.build(spec);
					show(false);
				}

				TableNextColumn();
				if (gui.toolbar->has(spec) && Button(fmtc("-toolbar##d%s", spec->name), ImVec2(-1,0))) {
					gui.toolbar->drop(spec);
				}
				else
				if (!gui.toolbar->has(spec) && Button(fmtc("+toolbar##a%s", spec->name), ImVec2(-1,0))) {
					gui.toolbar->add(spec);
				}

				TableNextColumn();
				if (spec->wiki.size() && Button("wiki", ImVec2(-1,0))) {
					openURL(spec->wiki);
				}

				EndTable();
			}
		}

		minivec<Spec*> iconSpecs;
		iconSpecs.push(spec);
		for (Spec* s = spec->cycle; s && s != spec; s = s->cycle) {
			if (!s->build) iconSpecs.push(s);
		}
		if (iconSpecs.size() > 1) {
			for (auto s: iconSpecs) {
				specIcon(s);
				if (IsItemHovered() && iconSpecs.size() > 1) tip("Use [C] to cycle ghost variants");
				SameLine();
			}
			NewLine();
		}

		if (spec->materials.size()) {
			Section("Construction Materials");
			for (auto& [iid,count]: spec->materials) drawItemButton(Item::get(iid), count);
			SpacingV();
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.spec[spec] = false;
}

void RecipePopup::drawSpecButton(Spec* spec) {
	if (!spec->build && !spec->ship) return;
	Bullet();
	if (SmallButtonInline(spec->title.c_str())) {
		if (spec->ship) {
			for (auto [_,recipe]: Recipe::names) {
				if (recipe->outputSpec == spec) {
					locate.recipe = recipe;
					expanded.recipe[recipe] = !expanded.recipe[recipe];
					break;
				}
			}
		}
		else {
			locate.spec = spec;
			expanded.spec[spec] = !expanded.spec[spec];
		}
	}
	highlited.spec[spec] = highlited.spec[spec] || IsItemHovered();
}

void RecipePopup::drawGoal(Goal* goal) {

//	if (!showUnavailable && !goal->licensed && !expanded.goal[goal]) return;

	if (locate.goal == goal) {
		SetScrollHereY();
		locate.goal = nullptr;
	}

	int pop = 0;

	if (highlited.goal[goal]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if (!goal->active() && !highlited.goal[goal] && !expanded.goal[goal]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.goal[goal]);
	if (!goal->active()) PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.goal[goal] = CollapsingHeader(fmtc("%s##build-goal-%s", goal->title, goal->name));
	if (!goal->active()) PopStyleColor(1);

	if (goal->met) {
		SameLine();
		PushFont(Config::sidebar.font.imgui);
		PrintRight("met", true);
		PopFont();
	}

	if (expanded.goal[goal]) {

		auto colorStore = ImColorSRGB(0x999999ff);

		if (goal->construction.size()) {
			Section("Construction");
			for (auto [spec,count]: goal->construction) {
				drawSpecButton(spec);
				SameLine(); PrintRight(fmtc("%u/%u", spec->count.extant, count));
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				SmallBar((float)spec->count.extant/(float)count);
				PopStyleColor(1);
				SpacingV();
			}
		}
		if (goal->productionItem.size() || goal->productionFluid.size()) {
			Section("Production");
			for (auto [iid,count]: goal->productionItem) {
				auto item = Item::get(iid);
				drawItemButton(item);
				SameLine(); PrintRight(fmtc("%u/%u", item->produced, count));
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				SmallBar((float)item->produced/(float)count);
				PopStyleColor(1);
				SpacingV();
			}
			for (auto [fid,count]: goal->productionFluid) {
				auto fluid = Fluid::get(fid);
				drawFluidButton(fluid);
				SameLine(); PrintRight(fmtc("%u/%u", fluid->produced, count));
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				SmallBar((float)fluid->produced/(float)count);
				PopStyleColor(1);
				SpacingV();
			}
		}
		if (goal->supplies.size()) {
			Section("Supply");
			for (auto stack: goal->supplies) {
				drawItemButton(Item::get(stack.iid));
				SameLine(); PrintRight(fmtc("%u/%u", Item::supplied[stack.iid], stack.size));
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				SmallBar((float)Item::supplied[stack.iid]/(float)stack.size);
				PopStyleColor(1);
				SpacingV();
			}
		}
		if (goal->rates.size()) {
			uint minute = 60*60;
			uint hour = minute*60;
			Section(fmtc("Supply over any %dh period", goal->period/hour));
			if (IsItemHovered()) tip(fmt(
				"Throughput goals are moving windows, and will auto-complete when"
				" supply rates for the listed items are sustained at or above the"
				" target level(s) for the last N hours.\n\n"
				"Basically, make everything turn green on the charts.\n\n"
				"Balancing and scaling up the factory to meet the target rate is"
				" the intention, but if you prefer to stockpile and manually"
				" release items for launch then go for it!\n\n"
				"Suggestion: consider stockpiling a moderate buffer as a fallback"
				" but also attempt to hit the sustained throughput target."
				" Whatever the outcome, later goals will consume any excess products."
			));
			for (auto& rate: goal->rates) {
				drawItemButtonNoBullet(Item::get(rate.iid));
				SameLine(); PrintRight(fmtc("%d/h", rate.count), true);
				SpacingV();
				goalRateChart(goal, rate, (float)Config::height(0.05f));
			}
		}

		if (goal->license.specs.size() || goal->license.recipes.size()) {
			Section("Unlock");
			for (auto spec: goal->license.specs) if (spec->build) { drawSpecButton(spec); }
			for (auto recipe: goal->license.recipes) { drawRecipeButton(recipe); }
			SpacingV();
		}

		if (goal->reward) {
			Section("Perks");
			Print("Upgrade points");
			SameLine(); PrintRight(fmtc("%d", goal->reward));
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.goal[goal] = false;
}

void RecipePopup::drawGoalButton(Goal* goal) {

	if (SmallButtonInline(goal->title.c_str())) {
		locate.goal = goal;
		expanded.goal[goal] = !expanded.goal[goal];
	}
	highlited.goal[goal] = highlited.goal[goal] || IsItemHovered();
}


