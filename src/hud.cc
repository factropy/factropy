#include "common.h"
#include "config.h"
#include "popup.h"
#include "sim.h"
#include "entity.h"
#include "vehicle.h"
#include "energy.h"
#include "string.h"
#include "enemy.h"
#include "chunk.h"
#include "scene.h"
#include "gui.h"
#include "hud.h"

void HUD::draw() {
	using namespace ImGui;
	auto& style = GetStyle();

	// popup "big"
	float h = (float)Config::height(0.75f);
	float w = h*1.45;

	ImVec2 size = {
		w,
		0.0f,
	};

	ImVec2 pos = {
		((float)Config::window.width-size.x)/2.0f,
		0.0f,
	};

	SetNextWindowSize(size, ImGuiCond_Always);
	SetNextWindowPos(pos, ImGuiCond_Always);

	struct {
		bool electricity = false;
		bool goal = false;
	} hovered;

	Goal* goal = Goal::current();
	ElectricityNetworkState estate;

	Sim::locked([&]() {
		estate = ElectricityNetwork::aggregate();
	});

	Begin("##hud", nullptr, flags | ImGuiWindowFlags_NoBringToFrontOnFocus);
		PushFont(Config::hud.font.imgui);

		BeginTable("##hud-layout",3);

		TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, size.x*0.3);
		TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
		TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, size.x*0.3);

		TableNextColumn();

			BeginGroup();
			Print("Electricity:"); SameLine();
				auto capacity = estate.capacityReady; // + estate.capacityBufferedReady;
				PrintRight(fmtc("%s / %s", estate.demand.formatRate(), capacity.formatRate()));
				OverflowBar(estate.demand.portion(capacity), ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));
				OverflowBar(estate.bufferedLevel.portion(estate.bufferedLimit), ImColorSRGB(0xffff00ff), ImColorSRGB(0x9999ddff));
			EndGroup();
			hovered.electricity = IsItemHovered();

		ImVec2 button = {-1,GetItemRectSize().y};

		TableNextColumn();

			BeginTable("##hud-buttons",3);

			TableNextColumn();

				if (Button("Build [E]", button)) {
					gui.doBuild = true;
				}

			TableNextColumn();

				if (Button("Menu", button)) {
					gui.doMenu = true;
				}

			TableNextColumn();

				if (Button(fmtc("Upgrade (%d)", Goal::chits), button)) {
					gui.doUpgrade = true;
				}

			EndTable();

		TableNextColumn();
			BeginGroup();
			Print("Goal:"); SameLine();

			if (goal) {
				PrintRight(goal->title.c_str());
				Sim::locked([&]() {
					SmallBar(goal->progress());
					PushStyleColor(ImGuiCol_PlotHistogram, ImColorSRGB(0x888888ff));
					SmallBar(Goal::overallProgress());
					PopStyleColor(1);
				});
			}

			if (!goal) {
				PrintRight("BYO!");
				SmallBar(0.0f);
				PushStyleColor(ImGuiCol_PlotHistogram, ImColorSRGB(0x888888ff));
				SmallBar(Goal::overallProgress());
				PopStyleColor(1);
			}

			EndGroup();
			hovered.goal = IsItemHovered();

		EndTable();
		PopFont();

	size = GetWindowSize();
	End();

	if (Sim::alerts.active) {
		Sim::locked([&]() {
			if (!Sim::alerts.active) return;

			SetNextWindowSize(ImVec2(-1,-1), ImGuiCond_Always);

			float bsize = GetFontSize()*1.3f;
			auto noticeBG = Color(0x666666ff);
			auto noticeFG = Color(0xffffffff);
			auto warningBG = Color(0xdd8800ff);
			auto warningFG = Color(0xffffffff);
			auto criticalBG = Color(0xff0000ff);
			auto criticalFG = Color(0xffffffff);
			int id = 0;

			auto button = [&](const char* icon, const Color& bg, const Color& fg, const std::string& msg, Point target) {
				PushStyleColor(ImGuiCol_Button, bg.gamma());
				PushStyleColor(ImGuiCol_Text, fg.gamma());
				if (Button(fmtc("%s##alert-%d", icon, ++id), ImVec2(bsize,bsize))) {
					gui.doMap = true;
					gui.mapPopup->retarget = target;
				}
				if (IsItemHovered()) Popup::tip(msg);
				PopStyleColor(2);
				SameLine();
			};

			Begin("##alerts", nullptr, flags);
				if (Sim::alerts.entitiesDamaged) {
					Point target = Point::Zero; for (auto eid: Entity::damaged) if (Entity::exists(eid)) { target = Entity::get(eid).pos(); break; }
					button(ICON_FA_FIRE_EXTINGUISHER, criticalBG, criticalFG, fmt("Entities damaged: %u", Sim::alerts.entitiesDamaged), target);
				}
				if (Sim::alerts.vehiclesBlocked) {
					Point target = Point::Zero; for (auto& cart: Cart::all) if (cart.blocked) { target = cart.en->pos(); break; }
					button(ICON_FA_TRUCK, warningBG, warningFG, fmt("Vehicles blocked: %u", Sim::alerts.vehiclesBlocked), target);
				}
				if (Sim::alerts.monocarsBlocked) {
					Point target = Point::Zero; for (auto& car: Monocar::all) if (car.blocked) { target = car.en->pos(); break; }
					button(ICON_FA_TRAIN, warningBG, warningFG, fmt("Monorail cars blocked: %u", Sim::alerts.monocarsBlocked), target);
				}

				struct entry {
					int icon = 0;
					uint count = 0;
					Point target = Point::Zero;
				};

				minimap<entry,&entry::icon> entries;

				if (Sim::alerts.customCritical) {
					entries.clear();

					for (auto& router: Router::all) {
						if (router.alert.critical) {
							auto& entry = entries[router.alert.critical];
							entry.target = router.en->pos();
							entry.count++;
						}
					}

					for (auto& entry: entries) {
						button(Sim::customIcons[entry.icon], criticalBG, criticalFG, fmt("Custom critical alerts: %u", entry.count), entry.target);
					}
				}

				if (Sim::alerts.customWarning) {
					entries.clear();

					for (auto& router: Router::all) {
						if (router.alert.warning) {
							auto& entry = entries[router.alert.warning];
							entry.target = router.en->pos();
							entry.count++;
						}
					}

					for (auto& entry: entries) {
						button(Sim::customIcons[entry.icon], warningBG, warningFG, fmt("Custom warnings: %u", entry.count), entry.target);
					}
				}

				if (Sim::alerts.customNotice) {
					entries.clear();

					for (auto& router: Router::all) {
						if (router.alert.notice) {
							auto& entry = entries[router.alert.notice];
							entry.target = router.en->pos();
							entry.count++;
						}
					}

					for (auto& entry: entries) {
						button(Sim::customIcons[entry.icon], noticeBG, noticeFG, fmt("Custom notices: %u", entry.count), entry.target);
					}
				}

				auto asize = GetWindowSize();
				SetWindowPos(ImVec2(((float)Config::window.width - asize.x)/2.0f, size.y), ImGuiCond_Always);
			End();
		});
	}

//	auto colorEnergy = ImColorSRGB(0x9999ddff);
	auto colorStore = ImGui::ImColorSRGB(0x999999ff);

	if (hovered.electricity || hovered.goal) {

		ImVec2 tooltip_pos = GetMousePos();
		tooltip_pos.x += (Config::window.hdpi/96.0) * 16;
		tooltip_pos.y += (Config::window.vdpi/96.0) * 16;
		SetNextWindowPos(tooltip_pos);

		SetNextWindowSize(ImVec2(size.x*0.18,-1));

//		if (!goal->rates.size())
//			SetNextWindowBgAlpha(style.Colors[ImGuiCol_PopupBg].w * 0.75f);

		Begin("##space-tip", nullptr, flags | ImGuiWindowFlags_NoInputs);

		PushFont(Config::sidebar.font.imgui);

		if (hovered.electricity) {
			Sim::locked([&]() {
				if (Sim::tick%60 == 0) {
					electricity.generators.clear();
					electricity.consumers.clear();
					for (auto& pair: Spec::all) {
						auto& spec = pair.second;
						if (spec->statsGroup != spec) continue;
						if (!spec->count.extant) continue;

						if (spec->generateElectricity || spec->bufferElectricity) {
							electricity.generators.push_back(spec);
						}

						if (spec->consumeElectricity || spec->consumeCharge) {
							electricity.consumers.push_back(spec);
						}
					}

					std::sort(electricity.consumers.begin(), electricity.consumers.end(), [&](const Spec* a, const Spec* b) {
						auto energyA = Energy(a->energyConsumption.ticks[a->energyConsumption.tick(Sim::tick-1)]);
						auto energyB = Energy(b->energyConsumption.ticks[b->energyConsumption.tick(Sim::tick-1)]);
						return energyA > energyB;
					});

					std::sort(electricity.generators.begin(), electricity.generators.end(), [&](const Spec* a, const Spec* b) {
						auto energyA = Energy(a->energyGeneration.ticks[a->energyGeneration.tick(Sim::tick-1)]);
						auto energyB = Energy(b->energyGeneration.ticks[b->energyGeneration.tick(Sim::tick-1)]);
						return energyA > energyB;
					});
				}

				Energy otherStuff = 0;

				for (int i = 0, l = electricity.generators.size(); i < l; i++) {
					auto spec = electricity.generators[i];
					auto energy = Energy(spec->energyGeneration.ticks[spec->energyGeneration.tick(Sim::tick-1)]);
					if (i > 10) {
						otherStuff += energy;
						continue;
					}
					Print(spec->title.c_str());
					SameLine(); PrintRight(fmtc("%s", energy.formatRate()));
					PushStyleColor(ImGuiCol_PlotHistogram, Color(0x00cc00ff));
					SmallBar(energy.portion(estate.supply));
					PopStyleColor(1);
					Spacing();
				}

				if (otherStuff) {
					Print("(other)");
					SameLine(); PrintRight(fmtc("%s", otherStuff.formatRate()));
					PushStyleColor(ImGuiCol_PlotHistogram, Color(0x00cc00ff));
					SmallBar(otherStuff.portion(estate.supply));
					PopStyleColor(1);
					Spacing();
				}

				otherStuff = 0;

				for (int i = 0, l = electricity.consumers.size(); i < l; i++) {
					auto spec = electricity.consumers[i];
					auto energy = Energy(spec->energyConsumption.ticks[spec->energyConsumption.tick(Sim::tick-1)]);
					if (i > 10) {
						otherStuff += energy;
						continue;
					}
					Print(spec->title.c_str());
					SameLine(); PrintRight(fmtc("%s", energy.formatRate()));
					PushStyleColor(ImGuiCol_PlotHistogram, Color(0xcc0000ff));
					SmallBar(energy.portion(estate.demand));
					PopStyleColor(1);
					Spacing();
				}

				if (otherStuff) {
					Print("(other)");
					SameLine(); PrintRight(fmtc("%s", otherStuff.formatRate()));
					PushStyleColor(ImGuiCol_PlotHistogram, Color(0xcc0000ff));
					SmallBar(otherStuff.portion(estate.demand));
					PopStyleColor(1);
					Spacing();
				}
			});
		}

		if (hovered.goal && goal) {
			Sim::locked([&]() {
				if (goal->construction.size()) {
					Header("Construction");
					for (auto [spec,count]: goal->construction) {
						Print(spec->title.c_str());
						// 0.2.x saves may not have constructed; remove extant later
						auto constructed = std::max(spec->count.constructed, spec->count.extant);
						SameLine(); PrintRight(fmtc("%u/%u", constructed, count));
						PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
						SmallBar((float)constructed/(float)count);
						PopStyleColor(1);
						Spacing();
					}
				}
				if (goal->productionItem.size() || goal->productionFluid.size()) {
					Header("Production");
					for (auto [iid,count]: goal->productionItem) {
						auto item = Item::get(iid);
						Print(item->title.c_str());
						SameLine(); PrintRight(fmtc("%u/%u", item->produced, count));
						PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
						SmallBar((float)item->produced/(float)count);
						PopStyleColor(1);
						Spacing();
					}
					for (auto [fid,count]: goal->productionFluid) {
						auto fluid = Fluid::get(fid);
						Print(fluid->title.c_str());
						SameLine(); PrintRight(fmtc("%u/%u", fluid->produced, count));
						PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
						SmallBar((float)fluid->produced/(float)count);
						PopStyleColor(1);
						Spacing();
					}
				}
				if (goal->supplies.size()) {
					Header("Supply");
					for (auto stack: goal->supplies) {
						Print(Item::get(stack.iid)->title.c_str());
						SameLine(); PrintRight(fmtc("%u/%u", Item::supplied[stack.iid], stack.size));
						PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
						SmallBar((float)Item::supplied[stack.iid]/(float)stack.size);
						PopStyleColor(1);
						Spacing();
					}
				}
				if (goal->rates.size()) {
					Header(fmtc("Supply over any %dh period", goal->period/goal->interval));
					for (auto& rate: goal->rates) {
						Print(Item::get(rate.iid)->title.c_str());
						SameLine(); PrintRight(fmtc("%d/h", rate.count));
						Popup::goalRateChart(goal, rate, (float)Config::height(0.05f));
					}
				}

				if (goal->license.specs.size() || goal->license.recipes.size()) {
					Header("Unlock");
					for (auto spec: goal->license.specs) if (spec->build) { Bullet(); Print(spec->title); }
					for (auto recipe: goal->license.recipes) { Bullet(); Print(recipe->title); }
					Spacing();
				}

				if (goal->reward) {
					Header("Perks");
					Print("Upgrade points");
					SameLine(); PrintRight(fmtc("%d", goal->reward));
				}
			});
		}

		PopFont();
		End();
	}

	if (goal && goal->hints.size()) {
		SetNextWindowSize(ImVec2(-1,-1), ImGuiCond_Always);
		SetNextWindowBgAlpha(style.Colors[ImGuiCol_PopupBg].w * 0.0f);
		PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		Begin("##hints", nullptr, flags | ImGuiWindowFlags_NoInputs);
			PushFont(Config::hud.font.imgui);

			for (auto hint: goal->hints) {
				Print(hint.c_str());
			}

			PopFont();
		SetWindowPos(ImVec2(0, (float)Config::window.height - GetWindowSize().y), ImGuiCond_Always);
		PopStyleVar(1);
		End();
	}

	SetNextWindowSize(ImVec2(-1,-1), ImGuiCond_Always);
	SetNextWindowBgAlpha(style.Colors[ImGuiCol_PopupBg].w * 0.0f);
	PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	Begin("##ups", nullptr, flags | ImGuiWindowFlags_NoInputs);
		PushFont(Config::mono.font.imgui);

		int ups = std::round(gui.ups);
		int fps = std::round(gui.fps);

		if (Config::mode.overlayUPS) Print(fmtc("%d UPS", ups));
		if (Config::mode.overlayFPS) Print(fmtc("%d FPS", fps));

		if (Config::mode.overlayDebug) {
			Print(fmtc("%f FOVY", scene.fovy));
			Print(fmtc("%s POS", std::string(scene.position)));
			Print(fmtc("%s TAR", std::string(scene.groundTarget())));
			Print(fmtc("%s OFF", std::string(scene.offset)));
			Print(fmtc("%s CAM", std::string(scene.camera)));
			Print(fmtc("Crafters hot %u cold %u", Crafter::hot.size(), Crafter::cold.size()));
			Print(fmtc("Drone Ports hot %u cold %u", Depot::hot.size(), Depot::cold.size()));
			std::vector<Spec*> specs;
			for (auto& [_,spec]: Spec::all) specs.push_back(spec);
			std::sort(specs.begin(), specs.end(), [&](auto a, auto b) { return a->count.render > b->count.render; });
			for (int i = 0, l = std::min(10, (int)specs.size()); i < l; i++) {
				Print(fmtc("%d %s", specs[i]->count.render, specs[i]->name));
			}
		}

		PopFont();
	SetWindowPos(ImVec2((float)Config::window.width - GetWindowSize().x, 0), ImGuiCond_Always);
	PopStyleVar(1);
	End();

	SetNextWindowPos(ImVec2(0,0), ImGuiCond_Always);
	SetNextWindowSize(ImVec2(-1,-1), ImGuiCond_Always);
	SetNextWindowBgAlpha(style.Colors[ImGuiCol_PopupBg].w * 0.0f);
	PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	Begin("##messages", nullptr, flags | ImGuiWindowFlags_NoInputs);
		PushFont(Config::hud.font.imgui);

		if (Config::mode.pause) {
			Print("Game paused...");
		}

		for (auto message: scene.output.visible) {
			Print(message.text.c_str());
		}

		PopFont();
	PopStyleVar(1);
	End();
}

