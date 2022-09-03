#include "common.h"
#include "config.h"
#include "popup.h"
#include "catenate.h"

#include "../imgui/setup.h"

using namespace ImGui;

StatsPopup2::StatsPopup2() : Popup() {
}

StatsPopup2::~StatsPopup2() {
}

void StatsPopup2::prepare() {
	for (auto [_,item]: Item::names) itemsAlpha.push_back(item);
	std::sort(itemsAlpha.begin(), itemsAlpha.end(), [&](auto a, auto b) { return a->title < b->title; });
	filter[0] = 0;
}

void StatsPopup2::draw() {
	bool showing = true;

	uint64_t second = 60;
	uint64_t minute = second*60;
	uint64_t hour = minute*60;

	int id = 0;

	big();
	Begin("Stats", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	if (BeginTabBar("stats-tabs", ImGuiTabBarFlags_None)) {

		if (BeginTabItem("Energy")) {

			Sim::locked([&]() {
				if (Sim::tick < 60*60) return;
			});

			EndTabItem();
		}

		if (BeginTabItem("Production")) {
			SpacingV();
			Warning("Imagine an old-school under-construction gif here :)");

			SpacingV();
			if (BeginTable("##prod-form", 2)) {
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x/4.0);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x/4.0);

				TableNextColumn();
				PushItemWidth(-1);
				InputTextWithHint("##prod-filter", "filter items", filter, sizeof(filter));
				inputFocused = IsItemActive();
				PopItemWidth();

				TableNextColumn();
				PushItemWidth(-1);
				if (BeginCombo("##prod-filter-mode", any ? "Match any word": "Match all words")) {
					if (Selectable("Match any word", any)) any = true;
					if (Selectable("Match all words", !any)) any = false;
					EndCombo();
				}
				PopItemWidth();
				EndTable();
			}

			double prod[61];
			double cons[61];

			auto empty = [&]() {
				for (int i = 0; i < 61; i++) prod[i] = 0;
				for (int i = 0; i < 61; i++) cons[i] = 0;
			};

			float column = GetContentRegionAvail().x/2.0;

			auto chart60 = [&](std::function<void(void)> generate) {
				if (ImPlot::BeginPlot(fmtc("##%d", ++id), ImVec2(-1,column*0.2), ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs)) {
					empty();
					generate();
					ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
					ImPlot::SetupAxis(ImAxis_X2, nullptr, ImPlotAxisFlags_NoDecorations);
					ImPlot::PlotLine("prod", &prod[1], 59);
					ImPlot::PlotLine("cons", &cons[1], 59);
					ImPlot::EndPlot();
				}
			};

			auto needle = std::string(filter);

			auto match = [&](const std::string& title) {
				for (auto sub: discatenate(needle, " ")) {
					auto it = std::search(
						title.begin(), title.end(), sub.begin(), sub.end(),
						[](char a, char b) {
							return std::tolower(a) == std::tolower(b);
						});
					if (any && it != title.end()) return true;
					if (!any && it == title.end()) return false;
				}
				return !any;
			};

			Sim::locked([&]() {
				if (Sim::tick < 60*60) return;

				for (auto item: itemsAlpha) {
					ensure(item->title.size());
					if (needle.size() && !match(item->title)) continue;

					if (BeginTable(fmtc("#item-prod-%u", item->id), 2)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, column);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, column);

						TableNextColumn();
						TextCentered(fmtc("%s / min", item->title));
						chart60([&]() {
							int i = Sim::tick <= hour ? (hour-Sim::tick)/minute: 0;
							for (uint64_t tick = Sim::tick <= hour ? 0: Sim::tick-hour; tick < Sim::tick; tick += minute, i++) {
								prod[i] = item->production.minutes[item->production.minute(tick)];
								cons[i] = item->consumption.minutes[item->consumption.minute(tick)];
							}
						});

						TableNextColumn();
						TextCentered(fmtc("%s / sec", item->title));
						chart60([&]() {
							int i = Sim::tick <= minute ? (minute-Sim::tick)/second: 0;
							for (uint64_t tick = Sim::tick <= minute ? 0: Sim::tick-minute; tick < Sim::tick; tick += second, i++) {
								prod[i] = item->production.seconds[item->production.second(tick)];
								cons[i] = item->consumption.seconds[item->consumption.second(tick)];
							}
						});

						EndTable();
					}
				}
			});

			EndTabItem();
		}

		if (BeginTabItem("SIM")) {

			struct Series {
				std::string title;
				TimeSeries* ts = nullptr;
				double data[61];
			};

			std::deque<Series> plots = {
				{ .title = "Chunk", .ts = &Sim::statsChunk},
				{ .title = "EntityPre", .ts = &Sim::statsEntityPre},
				{ .title = "EntityPost", .ts = &Sim::statsEntityPost},
				{ .title = "Ghost", .ts = &Sim::statsGhost},
				{ .title = "Networker", .ts = &Sim::statsNetworker},
				{ .title = "Pile", .ts = &Sim::statsPile},
				{ .title = "Explosive", .ts = &Sim::statsExplosive},
				{ .title = "Store", .ts = &Sim::statsStore},
				{ .title = "Arm", .ts = &Sim::statsArm},
				{ .title = "Crafter", .ts = &Sim::statsCrafter},
				{ .title = "Venter", .ts = &Sim::statsVenter},
				{ .title = "Effector", .ts = &Sim::statsEffector},
				{ .title = "Launcher", .ts = &Sim::statsLauncher},
				{ .title = "Conveyor", .ts = &Sim::statsConveyor},
				{ .title = "Unveyor", .ts = &Sim::statsUnveyor},
				{ .title = "Loader", .ts = &Sim::statsLoader},
				{ .title = "Balancer", .ts = &Sim::statsBalancer},
				{ .title = "Path", .ts = &Sim::statsPath},
				{ .title = "Vehicle", .ts = &Sim::statsVehicle},
				{ .title = "Cart", .ts = &Sim::statsCart},
				{ .title = "Pipe", .ts = &Sim::statsPipe},
				{ .title = "DronePort", .ts = &Sim::statsDepot},
				{ .title = "Drone", .ts = &Sim::statsDrone},
				{ .title = "Missile", .ts = &Sim::statsMissile},
				{ .title = "Explosion", .ts = &Sim::statsExplosion},
				{ .title = "Turret", .ts = &Sim::statsTurret},
				{ .title = "Computer", .ts = &Sim::statsComputer},
				{ .title = "Router", .ts = &Sim::statsRouter},
				{ .title = "Enemy", .ts = &Sim::statsEnemy},
				{ .title = "Zeppelin", .ts = &Sim::statsZeppelin},
				{ .title = "FlightLogistic", .ts = &Sim::statsFlightLogistic},
				{ .title = "FlightPath", .ts = &Sim::statsFlightPath},
				{ .title = "Tube", .ts = &Sim::statsTube},
				{ .title = "Teleporter", .ts = &Sim::statsTeleporter},
				{ .title = "Monorail", .ts = &Sim::statsMonorail},
				{ .title = "Monocar", .ts = &Sim::statsMonocar},
				{ .title = "Source", .ts = &Sim::statsSource},
				{ .title = "PowerPole", .ts = &Sim::statsPowerPole},
				{ .title = "Charger", .ts = &Sim::statsCharger},
			};

			std::sort(plots.begin(), plots.end(), [](const auto& a, const auto& b) {
				return a.title < b.title;
			});

			Sim::locked([&]() {
				if (Sim::tick < 60) return;

				double yMax = 0.0;
				for (auto& plot: plots) {
					int i = 0;
					for (uint64_t tick = Sim::tick-60; tick < Sim::tick; tick++, i++) {
						plot.data[i] = plot.ts->ticks[plot.ts->tick(tick)];
					}
					yMax = std::max(yMax, plot.ts->secondMax);
				}

				double yLim = std::ceil(yMax);

				PushFont(Config::sidebar.font.imgui);
				if (ImPlot::BeginPlot(fmtc("##%d", ++id), ImVec2(-1,-1), ImPlotFlags_NoChild | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs)) {
					ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
					ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, yLim, ImPlotCond_Always);

					for (auto& plot: plots) {
						ImPlot::PlotLine(plot.title.c_str(), &plot.data[1], 59);
					}

					ImPlot::EndPlot();
				}
				PopFont();
			});

			EndTabItem();
		}

		if (BeginTabItem("GUI")) {

			struct Series {
				std::string title;
				TimeSeries* ts = nullptr;
				double data[61];
			};

			std::deque<Series> plots = {
				{.title = "update", .ts = &scene.stats.update},
				{.title = "updateTerrain", .ts = &scene.stats.updateTerrain},
				{.title = "updateEntities", .ts = &scene.stats.updateEntities},
				{.title = "updateEntitiesFind", .ts = &scene.stats.updateEntitiesFind},
				{.title = "updateEntitiesLoad", .ts = &scene.stats.updateEntitiesLoad},
				{.title = "updateEntitiesHover", .ts = &scene.stats.updateEntitiesHover},
				{.title = "updateEntitiesInstances", .ts = &scene.stats.updateEntitiesInstances},
				{.title = "updateEntitiesItems", .ts = &scene.stats.updateEntitiesItems},
				{.title = "updateCurrent", .ts = &scene.stats.updateCurrent},
				{.title = "updatePlacing", .ts = &scene.stats.updatePlacing},
				{.title = "render", .ts = &scene.stats.render},
			};

			if (scene.frame < 60) return;

			double yMax = 0.0;
			for (auto& plot: plots) {
				int i = 0;
				for (uint64_t tick = scene.frame-60; tick < scene.frame; tick++, i++) {
					plot.data[i] = plot.ts->ticks[plot.ts->tick(tick)];
				}
				yMax = std::max(yMax, plot.ts->secondMax);
			}

			double yLim = 16.0;
			while (yLim < yMax) yLim = std::floor(yLim + 16);

			if (ImPlot::BeginPlot(fmtc("##%d", ++id), ImVec2(-1,-1), ImPlotFlags_NoChild | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs)) {
				ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, yLim, ImPlotCond_Always);

				for (auto& plot: plots) {
					ImPlot::PlotLine(plot.title.c_str(), &plot.data[1], 59);
				}

				ImPlot::EndPlot();
			}

			EndTabItem();
		}

		if (BeginTabItem("Debug")) {
			auto readable = [&](std::size_t bytes) {
				std::size_t K = 1024;
				if (bytes < K) return fmt("%llu B", bytes);
				if (bytes < K*K) return fmt("%llu kB", bytes/K);
				if (bytes < K*K*K) return fmt("%llu MB", bytes/K/K);
				return fmt("%llu GB", bytes/K/K/K);
			};

			if (BeginTable("memory", 3, ImGuiTableFlags_RowBg)) {
				TableSetupColumn("component");
				TableSetupColumn("memory");
				TableSetupColumn("extant");

				TableHeadersRow();

				TableNextRow();
				TableNextColumn();
				Print("World");
				TableNextColumn();
				Print(readable(world.memory()).c_str());
				TableNextColumn();

				TableNextRow();
				TableNextColumn();
				Print("Chunk");
				TableNextColumn();
				Print(readable(Chunk::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Chunk::all.size()));

				TableNextRow();
				TableNextColumn();
				Print("Entity");
				TableNextColumn();
				Print(readable(Entity::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Entity::all.size()));

				TableNextRow();
				TableNextColumn();
				Print("Arm");
				TableNextColumn();
				Print(readable(Arm::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Arm::all.size()));

				TableNextRow();
				TableNextColumn();
				Print("Conveyor");
				TableNextColumn();
				Print(readable(Conveyor::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Conveyor::managed.size() + Conveyor::unmanaged.size()));

				TableNextRow();
				TableNextColumn();
				Print("Loader");
				TableNextColumn();
				Print(readable(Loader::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Loader::all.size()));

				TableNextRow();
				TableNextColumn();
				Print("Store");
				TableNextColumn();
				Print(readable(Store::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Store::all.size()));

				EndTable();
			}

			if (BeginTable("recipes", 3, ImGuiTableFlags_RowBg)) {
				TableSetupColumn("recipe");
				TableSetupColumn("total energy");
				TableSetupColumn("raw materials");

				TableHeadersRow();

				std::vector<Recipe*> recipes;
				for (auto [_,recipe]: Recipe::names) recipes.push_back(recipe);
				reorder(recipes, [&](auto a, auto b) { return a->title < b->title; });

				for (auto recipe: recipes) {
					TableNextRow();

					TableNextColumn();
					Print(recipe->title.c_str());

					TableNextColumn();
					Print(recipe->totalEnergy().format().c_str());

					TableNextColumn();
					for (auto stack: recipe->totalRawItems()) {
						Print(fmt("%s(%u)", Item::get(stack.iid)->name, stack.size));
					}
					for (auto amount: recipe->totalRawFluids()) {
						Print(fmt("%s(%u)", Fluid::get(amount.fid)->name, amount.size));
					}
				}

				EndTable();
			}
			EndTabItem();
		}

		EndTabBar();
	}

	mouseOver = IsWindowHovered();
	subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);

	End();
	if (visible) show(showing);
}

