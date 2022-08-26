#include "common.h"
#include "config.h"
#include "popup.h"
#include <filesystem>
#include <fstream>

#include "../imgui/setup.h"

using namespace ImGui;

EntityPopup2::EntityPopup2() : Popup() {
}

EntityPopup2::~EntityPopup2() {
}

void EntityPopup2::useEntity(uint eeid) {
	eid = eeid;
}

void EntityPopup2::draw() {
	bool showing = true;

	Sim::locked([&]() {
		if (!eid || !Entity::exists(eid) || Entity::get(eid).isGhost()) {
			eid = 0;
			show(false);
			return;
		}

		Entity &en = Entity::get(eid);

		if (opened && en.spec->named) {
			std::snprintf(name, sizeof(name), "%s", en.name().c_str());
		}

		if (opened && en.spec->networker) {
			auto& networker = en.networker();
			for (uint i = 0, l = std::min((uint)networker.interfaces.size(), 4u); i < l; i++) {
				auto& interface = networker.interfaces[i];
				std::snprintf(interfaces[i], sizeof(interfaces[i]), "%s", interface.ssid.c_str());
			}
		}

		auto focusedTab = [&]() {
			bool focused = opened;
			opened = false;
			return (focused ? ImGuiTabItemFlags_SetSelected: ImGuiTabItemFlags_None) | ImGuiTabItemFlags_NoPushId;
		};

		auto signalKey = [&](int id, Signal::Key& key, bool metas = true) {
			auto title = key ? key.title(): "(no signal)";
			auto pick = Button(title.c_str(), ImVec2(-1,0));
			if (IsItemHovered()) tip("Change signal");
			if (signalPicker(pick, false)) key = signalPicked;
		};

		auto signalConstant = [&](int id, Signal& signal, bool lone = true) {
			float space = GetContentRegionAvail().x;
			if (BeginTable(fmtc("##signal-const-%d", id), lone ? 3: 2)) {
				TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.25);
				if (lone) TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.334);

				TableNextColumn();
				signalKey(id, signal.key);

				TableNextColumn();
				SetNextItemWidth(-1);
				InputInt(fmtc("##val%d", id), &signal.value);

				if (lone) {
					TableNextColumn();
					Print("Constant Signal");
				}

				EndTable();
			}
		};

		auto signalCondition = [&](int id, Signal::Condition& condition, bool lone, std::function<bool(void)> check = nullptr) {

			if (lone) {
				float space = GetContentRegionAvail().x;
				BeginTable(fmtc("##signal-cond-%d", id), check || en.spec->networker ? 4: 3);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.1);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.25);
				if (check) TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.332);
			}

			TableNextColumn();
			SetNextItemWidth(-1);
			signalKey(id, condition.key);

			TableNextColumn();
			SetNextItemWidth(-1);

			auto ops = std::map<Signal::Operator,std::string>{
				{ Signal::Operator::Eq,  "=",  },
				{ Signal::Operator::Ne,  "!=", },
				{ Signal::Operator::Lt,  "<",  },
				{ Signal::Operator::Lte, "<=", },
				{ Signal::Operator::Gt,  ">",  },
				{ Signal::Operator::Gte, ">=", },
				{ Signal::Operator::NMod, "!%",},
			};

			if (BeginCombo(fmtc("##op%d", id), ops[condition.op].c_str())) {
				for (auto [op,opName]: ops) {
					if (Selectable(opName.c_str(), condition.op == op)) {
						condition.op = op;
					}
				}
				EndCombo();
			}

			if (condition.key == Signal::Key(Signal::Meta::Now) && IsItemHovered()) tip(
				"!% means divisible modulo without remainder. (SIGNAL % VALUE) == 0."
				" Similar to the pure math | (vertical pipe) operator meaning 'divides',"
				" but players with a software background see the pipe as the more"
				" familiar logical OR."
			);

			TableNextColumn();
			SetNextItemWidth(-1);
			InputInt(fmtc("##cmp%d", id), &condition.cmp);

			if (check && !condition.valid()) {
				TableNextColumn();
				Print("invalid");
			} else
			if (check) {
				TableNextColumn();
				Print(check() ? "true": "false");
			}
//			else
//			if (en.spec->networker) {
//				TableNextColumn();
//				auto network = en.networker().input().network;
//				Print(condition.evaluate(network ? network->signals: Signal::NoSignals) ? "true": "false");
//			}

			if (lone) {
				EndTable();
			}
		};

		struct Option {
			uint id;
			const char* title;
			bool operator<(const Option& other) {
				return std::string(title) < std::string(other.title);
			};
		};

		std::vector<Option> optionFluids;

		for (auto& [_,fluid]: Fluid::names) {
			if (fluid->raw || fluid->manufacturable())
				optionFluids.push_back({fluid->id, fluid->title.c_str()});
		}

		std::sort(optionFluids.begin(), optionFluids.end(), [](const auto& a, const auto& b) {
			return Fluid::get(a.id)->title < Fluid::get(b.id)->title;
		});

		bool operableStore = en.spec->store && (en.spec->storeSetLower || en.spec->storeSetUpper);

		if (operableStore || en.spec->computer || en.spec->router || en.spec->powerpole) medium(); else small();
		Begin(fmtc("%s###entity", en.title()), &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if (BeginTabBar("entity-tabs", ImGuiTabBarFlags_None)) {

			if (en.spec->arm && BeginTabItem("Arm", nullptr, focusedTab())) {
				Arm& arm = en.arm();

				PushID("arm");

				SpacingV();
				SmallBar(arm.orientation);

				SpacingV();
				Section("Item Filtering");

				uint remove = 0;
				uint insert = 0;

				int i = 0;
				for (auto iid: arm.filter) {
					if (ButtonStrip(i++, fmtc(" %s ", Item::get(iid)->title))) remove = iid;
					if (IsItemHovered()) tip("Remove item from filter set.");
				}

				auto pick = ButtonStrip(i++, arm.filter.size() ? " + Item ": " Set Filter ");
				if (IsItemHovered()) tip("Add item to filter set.");
				if (itemPicker(pick)) insert = itemPicked;

				if (remove) arm.filter.erase(remove);
				if (insert) arm.filter.insert(insert);

				SpacingV();
				Section("Belt Lane Control");

				Checkbox("Input near", &arm.inputNear);
				Checkbox("Input far", &arm.inputFar);
				Checkbox("Output near", &arm.outputNear);
				Checkbox("Output far", &arm.outputFar);

				SpacingV();
				Section("Enable Rule");

				const char* monitor = nullptr;
				switch (arm.monitor) {
					case Arm::Monitor::InputStore: {
						monitor = "Source store";
						break;
					}
					case Arm::Monitor::OutputStore: {
						monitor = "Destination store";
						break;
					}
					case Arm::Monitor::Network: {
						monitor = "Wifi Network";
						break;
					}
				}
				if (BeginCombo("Monitor##moniotor-combo", monitor)) {
					if (Selectable("Source store", arm.monitor == Arm::Monitor::InputStore)) {
						arm.monitor = Arm::Monitor::InputStore;
					}
					if (Selectable("Destination store", arm.monitor == Arm::Monitor::OutputStore)) {
						arm.monitor = Arm::Monitor::OutputStore;
					}
					if (Selectable("Wifi Network", arm.monitor == Arm::Monitor::Network)) {
						arm.monitor = Arm::Monitor::Network;
					}
					EndCombo();
				}
				signalCondition(0, arm.condition, true, [&]() { return arm.checkCondition(); });

				PopID();

				EndTabItem();
			}

			if (en.spec->loader && BeginTabItem("Loader", nullptr, focusedTab())) {
				Loader& loader = en.loader();

				PushID("loader");

				SpacingV();
				Section("Item Filtering");

				uint remove = 0;
				uint insert = 0;

				int i = 0;
				for (auto iid: loader.filter) {
					if (ButtonStrip(i++, fmtc(" %s ", Item::get(iid)->title))) remove = iid;
					if (IsItemHovered()) tip("Remove item from filter set.");
				}

				auto pick = ButtonStrip(i++, loader.filter.size() ? " + Item ": " Set Filter ");
				if (IsItemHovered()) tip("Add item to filter set.");
				if (itemPicker(pick)) insert = itemPicked;

				if (remove) loader.filter.erase(remove);
				if (insert) loader.filter.insert(insert);

				SpacingV();
				Section("Enable Rule");
				if (BeginCombo("Monitor", loader.monitor == Loader::Monitor::Store ? "Store": "Network")) {
					if (Selectable("Store", loader.monitor == Loader::Monitor::Store)) {
						loader.monitor = Loader::Monitor::Store;
					}
					if (Selectable("Network", loader.monitor == Loader::Monitor::Network)) {
						loader.monitor = Loader::Monitor::Network;
					}
					EndCombo();
				}
				signalCondition(0, loader.condition, true, [&]() { return loader.checkCondition(); });

				Checkbox("Ignore container limits", &loader.ignore);
				if (IsItemHovered()) tip("Loader will override container item limits.");

				PopID();

				EndTabItem();
			}

			if (en.spec->pipeValve && BeginTabItem("Valve", nullptr, focusedTab())) {
				Pipe& pipe = en.pipe();

				PushID("pipe");
				LevelBar(pipe.network->level());

				int level = (int)(pipe.overflow * 100.0f);
				if (SliderInt("level", &level, 0, 100)) {
					pipe.overflow = std::round((float)level) / 100.0f;
				}

				if (pipe.filter) {
					Fluid* fluid = Fluid::get(pipe.filter);
					if (Button(fluid->title.c_str())) {
						pipe.filter = 0;
					}
				}

				if (BeginCombo("filter", nullptr)) {
					for (auto& option: optionFluids) {
						if (pipe.filter != option.id && Selectable(option.title, false)) {
							pipe.filter = option.id;
						}
					}
					EndCombo();
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->pipeCapacity > Liquid::l(10000) && BeginTabItem("Fluid Tank", nullptr, focusedTab())) {
				Pipe& pipe = en.pipe();

				PushID("fluid-tank");

				Print(pipe.network->fid ? Fluid::get(pipe.network->fid)->title: "(empty)");
				SameLine();
				PrintRight(fmtc("%s / %s",
					Liquid(pipe.network->fid ? ((float)en.spec->pipeCapacity.value * pipe.network->level()): 0).format(),
					Liquid(pipe.network->fid ? pipe.network->count(pipe.network->fid): 0).format()
				));

				SmallBar(pipe.network->level());

				Pipe::transmitters.erase(en.id);
				Checkbox("Transmit fluid level", &en.pipe().transmit);
				if (en.pipe().transmit && !en.isGhost()) {
					Pipe::transmitters.insert(en.id);
				}

				PopID();

				EndTabItem();
			}

			if (operableStore && BeginTabItem("Storage", nullptr, focusedTab())) {
				Store& store = en.store();

				PushID("store-manual");

					int id = 0;

					uint clear = 0;
					uint down = 0;

					Mass usage = store.usage();
					Mass limit = store.limit();

					SpacingV();
					SmallBar(usage.portion(limit));

					float width = GetContentRegionAvail().x;
					SpacingV();

					auto pick = Button(" + Item ");
					if (IsItemHovered()) tip(
						"Accept an item with limits."
					);

					if (itemPicker(pick)) {
						store.levelSet(Item::get(itemPicked)->id, 0, 0);
					}

					if (en.spec->construction || en.spec->depot) {
						SameLine();
						if (Button(" + Construction ")) {
							std::set<Item*> items;
							for (auto [_,spec]: Spec::all) {
								if (!spec->licensed) continue;
								if (spec->junk) continue;
								for (auto [iid,_]: spec->materials) {
									items.insert(Item::get(iid));
								}
							}
							for (auto item: items) {
								if (store.level(item->id)) continue;
								int limit = (int)store.limit().value/item->mass.value;
								int step  = (int)std::max(1U, (uint)limit/100);
								store.levelSet(item->id, 0, store.magic ? step: 0);
							}
						}
						if (IsItemHovered()) tip(
							"Accept all construction materials with limits."
						);
					}

					if (!en.spec->overflow) {
						const char* policy = "Allow";
						if (!store.block && store.purge) policy = "Purge";
						if (store.block) policy = "Block";

						SameLine();
						SetNextItemWidth(-1);
						if (BeginCombo("##other-items", fmtc("Other items: %s", policy))) {
							if (Selectable("Allow", !store.block && !store.purge)) {
								store.block = false;
								store.purge = false;
							}
							if (IsItemHovered()) tip("Other items will be allowed.");
							if (en.spec->logistic && Selectable("Purge", !store.block && store.purge)) {
								store.block = false;
								store.purge = true;
							}
							if (IsItemHovered()) tip("Other items will be allowed, then moved to overflow containers by drones.");
							if (Selectable("Block", store.block)) {
								store.block = true;
								store.purge = true;
							}
							if (IsItemHovered()) tip("Other items will be blocked.");
							EndCombo();
						}
					}

					PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemInnerSpacing.x/2,GetStyle().ItemInnerSpacing.y/2));

					SpacingV();
					if (BeginTable(fmtc("##store-levels-%u", id++), en.spec->crafter ? 10: 9)) {
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, GetFontSize());
						TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, width*0.1f);
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, width*0.04f);
						TableSetupColumn("Lower", ImGuiTableColumnFlags_WidthFixed, width*0.2f);
						TableSetupColumn("Upper", ImGuiTableColumnFlags_WidthFixed, width*0.2f);
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, width*0.04f);
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, width*0.04f);
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, width*0.04f);

						TableNextRow(ImGuiTableRowFlags_Headers);
						TableNextColumn();
						TableHeader("");

						TableNextColumn();
						TableHeader("Item");
						if (IsItemClicked()) store.sortAlpha();

						TableNextColumn();
						TableHeader("#");
						TableNextColumn();
						TableHeader("");
						TableNextColumn();
						TableHeader("Lower");
						TableNextColumn();
						TableHeader("Upper");
						TableNextColumn();
						TableHeader("");
						TableNextColumn();
						TableHeader("");

						if (en.spec->crafter) {
							TableNextColumn();
							TableHeader("craft");
						}

						for (auto& level: store.levels) {
							Item *item = Item::get(level.iid);
							TableNextRow();

							int limit = (int)std::max(level.upper, (uint)store.limit().value/item->mass.value);

							int step = 100;
							if (store.limit() < Mass::kg(1001)) step = 50;
							if (store.limit() < Mass::kg(501)) step = 25;

							TableNextColumn();
							itemIcon(item);
//							if (tipBegin()) {
//								Print(fmtc("count %u", store.count(item->id)));
//								Print(fmtc("countNet %u", store.countNet(item->id)));
//								Print(fmtc("countSpace %u", store.countSpace(item->id)));
//								Print(fmtc("countLessReserved %u", store.countLessReserved(item->id)));
//								Print(fmtc("countPlusPromised %u", store.countPlusPromised(item->id)));
//								Separator();
//								Print(fmtc("countRequesting %u", store.countRequesting(item->id)));
//								Print(fmtc("countProviding %u", store.countProviding(item->id)));
//								Print(fmtc("countActiveProviding %u", store.countActiveProviding(item->id)));
//								Print(fmtc("countAccepting %u", store.countAccepting(item->id)));
//								Print(fmtc("countAllowing %u", store.countAllowing(item->id)));
//								tipEnd();
//							}

							TableNextColumn();
							Print(fmtc("%s", item->title));

							TableNextColumn();
							Print(fmtc("%d", store.count(level.iid)));

							TableNextColumn();
							if (store.isRequesting(level.iid)) {
								Print("r"); if (IsItemHovered()) tip("requesting");
							} else
							if (store.isActiveProviding(level.iid)) {
								Print("a"); if (IsItemHovered()) tip("actively providing");
							} else
							if (store.isProviding(level.iid)) {
								Print("p"); if (IsItemHovered()) tip("passively providing");
							}

							int lower = level.lower;
							int upper = level.upper;

							TableNextColumn();
							if (en.spec->storeSetLower) {
								SetNextItemWidth(width*0.2f);
								if (InputIntClamp(fmtc("##%d", id++), &lower, 0, limit, step, step*10)) {
									store.levelSet(level.iid, lower, upper);
								}
							}

							TableNextColumn();
							if (en.spec->storeSetUpper) {
								SetNextItemWidth(width*0.2f);
								if (InputIntClamp(fmtc("##%d", id++), &upper, 0, limit, step, step*10)) {
									store.levelSet(level.iid, lower, upper);
								}
							}

							TableNextColumn();
							if (Button(fmtc("%s##%d", ICON_FA_THERMOMETER_FULL, id++), ImVec2(-1,0))) {
								store.levelSet(level.iid, lower, limit);
							}
							if (IsItemHovered()) tip("maximum upper limit");

							TableNextColumn();
							if (Button(fmtc("%s##%d", ICON_FA_LEVEL_DOWN, id++), ImVec2(-1,0))) {
								down = level.iid;
							}
							if (IsItemHovered()) tip("move row to bottom");

							TableNextColumn();
							if (Button(fmtc("%s##%d", ICON_FA_TIMES, id++), ImVec2(-1,0))) {
								clear = level.iid;
							}
							if (IsItemHovered()) tip("remove row limits");
						}

						for (Stack stack: store.stacks) {
							if (store.level(stack.iid) != nullptr) continue;
							Item *item = Item::get(stack.iid);

							int limit = (uint)store.limit().value/item->mass.value;
							int step  = (int)std::max(1U, (uint)limit/100);

							TableNextRow();

							TableNextColumn();
							itemIcon(item);

							TableNextColumn();
							Print(fmtc("%s", item->title));

							TableNextColumn();
							Print(fmtc("%d", store.count(stack.iid)));

							TableNextColumn();
							TableNextColumn();

							TableNextColumn();
							if (Button(fmtc("+##%d", id++), ImVec2(-1,0))) {
								uint size = stack.size%step ? stack.size+step-(stack.size%step): stack.size;
								store.levelSet(stack.iid, 0, size);
							}
							if (IsItemHovered()) tip(
								"Limit item"
							);

							TableNextColumn();
							TableNextColumn();
							TableNextColumn();

							if (en.spec->crafter) {
								TableNextColumn();
							}
						}

						EndTable();
					}
					PopStyleVar();

					if (clear) {
						store.levelClear(clear);
					}

					if (down) {
						auto level = store.level(down);
						uint lower = level->lower;
						uint upper = level->upper;
						store.levelClear(down);
						store.levelSet(down, lower, upper);
					}

				PopID();

				EndTabItem();
			}

			if (en.spec->teleporter && en.spec->teleporterSend && BeginTabItem("Teleporter", nullptr, focusedTab())) {
				Teleporter& teleporter = en.teleporter();

				PushID("teleporter-send");

				if (Button("teleport now")) {
					teleporter.trigger = true;
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->launcher && BeginTabItem("Launcher", nullptr, focusedTab())) {
				Launcher& launcher = en.launcher();
				Store& store = en.store();

				PushID("launcher");

				auto fuelRequired = launcher.fuelRequired();
				auto fuelAccessable = launcher.fuelAccessable();

				SpacingV();
				launcherNotice(launcher);

				SmallBar(launcher.progress);
				if (IsItemHovered()) tip("Launch progress");

				auto colorStore = ImColorSRGB(0x999999ff);
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);

				for (auto& amount: fuelRequired) {
					SpacingV();
					auto need = Liquid(amount.size);
					auto have = Liquid(fuelAccessable.has(amount.fid) ? fuelAccessable[amount.fid].size: (uint)0);
					Title(Fluid::get(amount.fid)->title.c_str()); SameLine();
					PrintRight(fmtc("%s / %s", have.format(), need.format()));
					SmallBar(have.portion(need));
				}

				SpacingV();

				uint insert = 0;
				uint remove = 0;

				int i = 0;
				for (auto iid: launcher.cargo) {
					if (ButtonStrip(i++, fmtc(" %s (%u)", Item::get(iid)->title, store.count(iid)))) remove = iid;
					if (IsItemHovered()) tip("Drop item type from payload");
				}

				auto pick = ButtonStrip(i++, launcher.cargo.size() ? " + Item ": " Set Payload ");
				if (IsItemHovered()) tip("Add an item type to payload.");
				if (itemPicker(pick)) insert = itemPicked;

				if (remove) launcher.cargo.erase(remove);
				if (insert) launcher.cargo.insert(insert);

				SpacingV();

				SmallBar(store.usage().portion(store.limit()));
				if (IsItemHovered()) tip("Loading progress");

				SpacingV();

				PopStyleColor(1);

				Section("Launch Rule");
				if (BeginCombo("Monitor", "Network")) {
					if (launcher.en->spec->networker && Selectable("Network", launcher.monitor == Launcher::Monitor::Network)) {
						launcher.monitor = Launcher::Monitor::Network;
					}
					EndCombo();
				}
				signalCondition(0, launcher.condition, true, [&]() { return launcher.checkCondition(); });

				bool ready = launcher.ready();
				if (Button(" Launch ") && ready) launcher.activate = true;
				if (IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !ready) tip("Not ready");

				PopID();

				EndTabItem();
			}

			if (en.spec->crafter && en.spec->crafterShowTab && BeginTabItem("Crafting", nullptr, focusedTab())) {
				Crafter& crafter = en.crafter();

				PushID("crafter");
				SpacingV();
				crafterNotice(crafter);

				SmallBar(crafter.progress);
				SpacingV();

				auto craftable = [&](Recipe* recipe) {
					return recipe->manufacturable() && crafter.craftable(recipe);
				};

				if (crafter.recipe) {
					auto pick = Button(fmtc(" %s ", crafter.recipe->title.c_str()));
					if (IsItemHovered()) tip("Change recipe");
					if (recipePicker(pick, craftable)) crafter.craft(recipePicked);
				}
				else {
					if (recipePicker(Button(" Set Recipe "), craftable)) {
						crafter.craft(recipePicked);
					}
				}

				if (crafter.recipe) {
					float width = GetContentRegionAvail().x;

					if (crafter.recipe->inputItems.size() || crafter.recipe->inputFluids.size()) {

						SpacingV();
						if (BeginTable("inputs", 4)) {
							TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
							TableSetupColumn("Resource", ImGuiTableColumnFlags_WidthStretch);
							TableSetupColumn("Consumes", ImGuiTableColumnFlags_WidthFixed, width*0.2f);
							TableSetupColumn("Contains", ImGuiTableColumnFlags_WidthFixed, width*0.2f);

							TableHeadersRow();

							auto atLeast = [&](int limit, int have, std::string s) {
								if (have < limit) {
									TextColored(0xff0000ff, s.c_str());
								}
								else {
									Print(s.c_str());
								}
								if (IsItemHovered()) tip(
									"Resource input buffers allow for 2x recipe consumption."
									" To avoid production stalling keep input buffers full."
								);
							};

							if (crafter.recipe->inputItems.size() && en.spec->store) {
								for (auto& stack: crafter.inputItemsState()) {
									TableNextRow();

									std::string& name = Item::get(stack.iid)->title;
									int consume = crafter.recipe->inputItems[stack.iid];
									int have = stack.size;

									TableNextColumn();
									itemIcon(Item::get(stack.iid));

									TableNextColumn();
									Print(name.c_str());

									TableNextColumn();
									Print(fmtc("%d", consume));

									TableNextColumn();
									atLeast(consume, have, fmt("%d", have));
								}
							}

							if (crafter.recipe->inputFluids.size()) {
								for (auto& amount: crafter.inputFluidsState()) {
									TableNextRow();

									std::string& name = Fluid::get(amount.fid)->title;
									int consume = crafter.recipe->inputFluids[amount.fid];
									int have = amount.size;

									TableNextColumn();
									fluidIcon(Fluid::get(amount.fid));

									TableNextColumn();
									Print(name.c_str());

									TableNextColumn();
									Print(Liquid(consume).format());

									TableNextColumn();
									atLeast(consume, have, Liquid(have).format());
								}
							}

							EndTable();
						}
					}

					if (crafter.recipe->outputItems.size() || crafter.recipe->outputFluids.size() || crafter.recipe->mine || crafter.recipe->drill) {

						SpacingV();
						if (BeginTable("outputs", 4)) {
							TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
							TableSetupColumn("Product", ImGuiTableColumnFlags_WidthStretch);
							TableSetupColumn("Produces", ImGuiTableColumnFlags_WidthFixed, width*0.2f);
							TableSetupColumn("Contains", ImGuiTableColumnFlags_WidthFixed, width*0.2f);

							TableHeadersRow();

							auto atMost = [&](int limit, int have, std::string s) {
								if (have && have >= limit) {
									TextColored(0xff0000ff, s.c_str());
								}
								else {
									Print(s.c_str());
								}
							};

							if (crafter.recipe->outputItems.size() && en.spec->store) {
								for (auto& stack: crafter.outputItemsState()) {
									TableNextRow();

									std::string& name = Item::get(stack.iid)->title;
									int produce = crafter.recipe->outputItems[stack.iid];
									int have = stack.size;

									TableNextColumn();
									itemIcon(Item::get(stack.iid));

									TableNextColumn();
									Print(name.c_str());

									TableNextColumn();
									Print(fmtc("%d", produce));

									TableNextColumn();
									atMost(produce+1, have, fmt("%d", have));
									if (IsItemHovered()) {
										tip("Product output buffers are capped at 2x production, and"
											" a run will not start if there is insufficient space."
											" To avoid production stalling keep output buffers empty."
										);
									}
								}
							}

							if (crafter.recipe->mine && en.spec->store) {
								TableNextRow();

								std::string& name = Item::get(crafter.recipe->mine)->title;
								int produce = 1;
								int have = en.store().count(crafter.recipe->mine);
								int limit = en.store().limit().items(crafter.recipe->mine);

								TableNextColumn();
								itemIcon(Item::get(crafter.recipe->mine));

								TableNextColumn();
								Print(name.c_str());

								TableNextColumn();
								Print(fmtc("%d", produce));

								TableNextColumn();
								atMost(limit, have, fmtc("%d", have));
								if (IsItemHovered() && crafter.recipe->mine) {
									tip(
										"Mining product output buffers are capped by the entity storage capacity."
									);
								}
							}

							if (crafter.recipe->outputFluids.size()) {
								for (auto& amount: crafter.outputFluidsState()) {
									TableNextRow();

									std::string& name = Fluid::get(amount.fid)->title;
									int produce = crafter.recipe->outputFluids[amount.fid];
									int have = amount.size;

									TableNextColumn();
									fluidIcon(Fluid::get(amount.fid));

									TableNextColumn();
									Print(name.c_str());

									TableNextColumn();
									Print(Liquid(produce).format().c_str());

									TableNextColumn();
									atMost(produce+1, have, Liquid(have).format());
									if (IsItemHovered()) {
										tip(
											"Fluid product buffers must be flushed to a pipe network before the next run starts."
										);
									}
								}
							}

							if (crafter.recipe->drill) {
								TableNextRow();

								std::string& name = Fluid::get(crafter.recipe->drill)->title;
								int produce = 100;
								int have = 0;

								for (auto& amount: crafter.exportFluids) {
									if (amount.fid == crafter.recipe->drill) have = amount.size;
								}

								TableNextColumn();
								fluidIcon(Fluid::get(crafter.recipe->drill));

								TableNextColumn();
								Print(name.c_str());

								TableNextColumn();
								Print(Liquid(produce).format().c_str());

								TableNextColumn();
								atMost(0, have, Liquid(have).format());
								if (IsItemHovered() && crafter.recipe->drill) {
									tip(
										"Drilling product buffers must be flushed to a pipe network before the next run starts."
									);
								}
							}

							EndTable();
						}
					}

					if (crafter.en->spec->crafterTransmitResources && crafter.recipe->mine) {
						Checkbox("Transmit hill ore count", &crafter.transmit);
					}

					if (crafter.en->spec->crafterTransmitResources && crafter.recipe->drill) {
						Checkbox("Transmit lake oil count", &crafter.transmit);
					}
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->vehicle && BeginTabItem("Vehicle", nullptr, focusedTab())) {
				Vehicle& vehicle = en.vehicle();

				PushID("vehicle");

				bool patrol = en.vehicle().patrol;
				if (Checkbox("patrol", &patrol)) {
					vehicle.patrol = patrol;
				}

				bool handbrake = vehicle.handbrake;
				if (Checkbox("handbrake", &handbrake)) {
					vehicle.handbrake = handbrake;
				}

				int n = 0;
				int thisWay = 0, dropWay = -1;

				for (auto waypoint: vehicle.waypoints) {
					if (waypoint->stopId && Entity::exists(waypoint->stopId)) {
						Print(Entity::get(waypoint->stopId).name().c_str());
					} else {
						Print(fmt("%f,%f,%f", waypoint->position.x, waypoint->position.y, waypoint->position.z));
					}

					if (waypoint == vehicle.waypoint) {
						SameLine();
						Print("(current)");
					}

					int thisCon = 0, dropCon = -1;
					for (auto condition: waypoint->conditions) {

						if_is<Vehicle::DepartInactivity>(condition, [&](auto con) {
							Print(fmt("inactivity %d", con->seconds));
							InputInt(fmtc("seconds##%d", n++), &con->seconds);
						});

						if_is<Vehicle::DepartItem>(condition, [&](auto con) {

							SetNextItemWidth(200);
							if (BeginCombo(fmtc("##%d", n++), (con->iid ? Item::get(con->iid)->title.c_str(): "item"), ImGuiComboFlags_None)) {
								for (auto& item: Item::all) {
									if (Selectable(fmtc("%s##%d", item.title, n++), con->iid == item.id)) {
										con->iid = item.id;
									}
								}
								EndCombo();
							}

							std::map<uint,std::string> ops = {
								{ Vehicle::DepartItem::Eq,  "==" },
								{ Vehicle::DepartItem::Ne,  "!=" },
								{ Vehicle::DepartItem::Lt,  "<" },
								{ Vehicle::DepartItem::Lte, "<=" },
								{ Vehicle::DepartItem::Gt,  ">" },
								{ Vehicle::DepartItem::Gte, ">=" },
							};

							SameLine();
							SetNextItemWidth(100);
							if (BeginCombo(fmtc("##%d", n++), ops[con->op].c_str(), ImGuiComboFlags_None)) {
								for (auto [op,opname]: ops) {
									if (Selectable(fmtc("%s##%d-%u", opname.c_str(), n++, op), con->op == op)) {
										con->op = op;
									}
								}
								EndCombo();
							}

							SameLine();
							SetNextItemWidth(200);
							InputInt(fmtc("##%d", n++), (int*)(&con->count));
						});

						SameLine();
						if (Button(fmtc("-con##%d", n++))) {
							dropCon = thisCon;
						}

						thisCon++;
					}

					if (dropCon >= 0) {
						auto it = waypoint->conditions.begin();
						std::advance(it, dropCon);
						delete *it;
						waypoint->conditions.erase(it);
					}

					if (Button(fmtc("+activity##%d", n++))) {
						waypoint->conditions.push_back(new Vehicle::DepartInactivity());
					}

					SameLine();

					if (Button(fmtc("+item##%d", n++))) {
						waypoint->conditions.push_back(new Vehicle::DepartItem());
					}

					if (Button(fmtc("-way##%d", n++))) {
						dropWay = thisWay;
					}

					thisWay++;
				}

				if (dropWay >= 0) {
					auto it = vehicle.waypoints.begin();
					std::advance(it, dropWay);
					delete *it;
					vehicle.waypoints.erase(it);
				}

				for (auto [eid,name]: Entity::names) {
					if (Button(fmtc("+way %s##%d", name.c_str(), n++))) {
						vehicle.addWaypoint(eid);
					}
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->cart && BeginTabItem("Vehicle", nullptr, focusedTab())) {
				Cart& cart = en.cart();

				PushID("cart");

				auto route = [&]() {
					if (cart.line == CartWaypoint::Red) return "Red";
					if (cart.line == CartWaypoint::Blue) return "Blue";
					if (cart.line == CartWaypoint::Green) return "Green";
					ensure(false); return "Red";
				};

				if (BeginCombo("Route", route())) {
					if (Selectable("Red", cart.line == CartWaypoint::Red)) {
						cart.line = CartWaypoint::Red;
					}
					if (Selectable("Blue", cart.line == CartWaypoint::Blue)) {
						cart.line = CartWaypoint::Blue;
					}
					if (Selectable("Green", cart.line == CartWaypoint::Green)) {
						cart.line = CartWaypoint::Green;
					}
					EndCombo();
				}

				signalConstant(0, cart.signal);

				PopID();

				EndTabItem();
			}

			if (en.spec->cartStop && BeginTabItem("Vehicle Stop", nullptr, focusedTab())) {
				CartStop& stop = en.cartStop();

				PushID("cart-stop");

				const char* selected = "Inactivity";
				if (stop.depart == CartStop::Depart::Empty) selected = "Vehicle Empty";
				if (stop.depart == CartStop::Depart::Full) selected = "Vehicle Full";

				if (BeginCombo("Departure condition", selected)) {
					if (Selectable("Inactivity", stop.depart == CartStop::Depart::Inactivity)) {
						stop.depart = CartStop::Depart::Inactivity;
					}
					if (Selectable("Vehicle Empty", stop.depart == CartStop::Depart::Empty)) {
						stop.depart = CartStop::Depart::Empty;
					}
					if (Selectable("Vehicle Full", stop.depart == CartStop::Depart::Full)) {
						stop.depart = CartStop::Depart::Full;
					}
					EndCombo();
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->cartWaypoint && BeginTabItem("Vehicle Waypoint", nullptr, focusedTab())) {
				CartWaypoint& waypoint = en.cartWaypoint();

				PushID("cart-waypoint");
				int down = -1;
				int clear = -1;

				SpacingV();
				Section("Vehicle Redirection Rules");

				auto route = [&](int line) {
					if (line == CartWaypoint::Red) return "Red";
					if (line == CartWaypoint::Blue) return "Blue";
					if (line == CartWaypoint::Green) return "Green";
					ensure(false); return "Red";
				};

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				for (int i = 0; i < (int)waypoint.redirections.size(); i++) {
					auto& redirection = waypoint.redirections[i];

					float space = GetContentRegionAvail().x;
					if (BeginTable(fmtc("##signal-cond-%d", i), 6)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.1);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.2);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.15);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);

						signalCondition(i, redirection.condition, false);

						TableNextColumn();
						SetNextItemWidth(-1);
						if (BeginCombo(fmtc("##route%d", i), route(redirection.line))) {
							if (Selectable("Red", redirection.line == CartWaypoint::Red)) {
								redirection.line = CartWaypoint::Red;
							}
							if (Selectable("Blue", redirection.line == CartWaypoint::Blue)) {
								redirection.line = CartWaypoint::Blue;
							}
							if (Selectable("Green", redirection.line == CartWaypoint::Green)) {
								redirection.line = CartWaypoint::Green;
							}
							EndCombo();
						}

						TableNextColumn();
						SetNextItemWidth(-1);
						if (Button(fmtc("%s##down%d", ICON_FA_LEVEL_DOWN, i), ImVec2(-1,0))) {
							down = i;
						}

						TableNextColumn();
						SetNextItemWidth(-1);
						if (Button(fmtc("%s##clear%d", ICON_FA_TRASH, i), ImVec2(-1,0))) {
							clear = i;
						}

						EndTable();
					}
				}

				PopStyleVar(1);

				if (down >= 0) {
					waypoint.redirections.push_back(waypoint.redirections[down]);
					waypoint.redirections.erase(down);
				}

				if (clear >= 0) {
					waypoint.redirections.erase(clear);
				}

				if (Button("Add rule")) {
					waypoint.redirections.push_back({});
					waypoint.redirections.back().condition.op = Signal::Operator::Gt;
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->monorailStop && BeginTabItem("Monorail Stop", nullptr, focusedTab())) {
				auto& monorail = en.monorail();

				PushID("monorail-stop");

				SpacingV();
				Section("Container Wait Rules");

				Checkbox("Wait for containers to fill before loading", &monorail.filling);
				Checkbox("Wait for containers to empty after unloading", &monorail.emptying);

				BeginDisabled(!en.spec->networker);
				Checkbox("Transmit container contents", &monorail.transmit.contents);
				EndDisabled();

				PopID();

				EndTabItem();
			}

			if (en.spec->monorail && BeginTabItem("Monorail Waypoint", nullptr, focusedTab())) {
				Monorail& waypoint = en.monorail();

				PushID("monorail-waypoint");
				int down = -1;
				int clear = -1;

				SpacingV();
				Section("Monorail Car Redirection Rules");

				auto route = [&](int line) {
					if (line == Monorail::Red) return "Red";
					if (line == Monorail::Blue) return "Blue";
					if (line == Monorail::Green) return "Green";
					ensure(false); return "Red";
				};

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				for (int i = 0; i < (int)waypoint.redirections.size(); i++) {
					auto& redirection = waypoint.redirections[i];

					float space = GetContentRegionAvail().x;
					if (BeginTable(fmtc("##signal-cond-%d", i), 6)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.1);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.2);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.15);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);

						signalCondition(i, redirection.condition, false);

						TableNextColumn();
						SetNextItemWidth(-1);
						if (BeginCombo(fmtc("##route%d", i), route(redirection.line))) {
							if (Selectable("Red", redirection.line == Monorail::Red)) {
								redirection.line = Monorail::Red;
							}
							if (Selectable("Blue", redirection.line == Monorail::Blue)) {
								redirection.line = Monorail::Blue;
							}
							if (Selectable("Green", redirection.line == Monorail::Green)) {
								redirection.line = Monorail::Green;
							}
							EndCombo();
						}

						TableNextColumn();
						SetNextItemWidth(-1);
						if (Button(fmtc("%s##down%d", ICON_FA_LEVEL_DOWN, i), ImVec2(-1,0))) {
							down = i;
						}

						TableNextColumn();
						SetNextItemWidth(-1);
						if (Button(fmtc("%s##clear%d", ICON_FA_TRASH, i), ImVec2(-1,0))) {
							clear = i;
						}

						EndTable();
					}
				}

				PopStyleVar(1);

				if (down >= 0) {
					waypoint.redirections.push_back(waypoint.redirections[down]);
					waypoint.redirections.erase(down);
				}

				if (clear >= 0) {
					waypoint.redirections.erase(clear);
				}

				if (Button("Add rule")) {
					waypoint.redirections.push_back({});
					waypoint.redirections.back().condition.op = Signal::Operator::Gt;
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->monocar && BeginTabItem("Monocar", nullptr, focusedTab())) {
				Monocar& monocar = en.monocar();

				PushID("monocar");

				auto route = [&]() {
					if (monocar.line == Monorail::Red) return "Red";
					if (monocar.line == Monorail::Blue) return "Blue";
					if (monocar.line == Monorail::Green) return "Green";
					ensure(false); return "Red";
				};

				if (BeginCombo("Route", route())) {
					if (Selectable("Red", monocar.line == Monorail::Red)) {
						monocar.line = Monorail::Red;
					}
					if (Selectable("Blue", monocar.line == Monorail::Blue)) {
						monocar.line = Monorail::Blue;
					}
					if (Selectable("Green", monocar.line == Monorail::Green)) {
						monocar.line = Monorail::Green;
					}
					EndCombo();
				}

				if (IsWindowAppearing()) checks.clear();
				checks.resize(monocar.constants.size());

				if (BeginTable("##constant-signals", 2)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

					int i = 0;
					for (auto& signal: monocar.constants) {
						TableNextRow();
						TableNextColumn();
						Checkbox(fmtc("##check%d", i), &checks[i]);
						TableNextColumn();
						signalConstant(i, signal, false);
						i++;
					}

					EndTable();
				}

				if (Button("Add Signal##add-signal")) {
					monocar.constants.resize(monocar.constants.size()+1);
					monocar.constants.back().value = 1;
				}

				bool checked = false;
				for (auto& check: checks) checked = checked || check;

				if (checked && Button("Drop Selected##drop-signals")) {
					for (int i = checks.size()-1; i >= 0; --i) {
						if (checks[i]) monocar.constants.erase(i);
					}
					checks.clear();
				}

				SpacingV();

				switch (monocar.state) {
					case Monocar::State::Start:
					case Monocar::State::Acquire:
					case Monocar::State::Travel:
						PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
						Button("Depart");
						PopStyleColor(1);
						break;
					case Monocar::State::Stop:
					case Monocar::State::Unload:
					case Monocar::State::Unloading:
					case Monocar::State::Load:
					case Monocar::State::Loading:
						if (Button("Depart"))
							monocar.flags.move = true;
						break;
				}

				if (IsItemHovered()) tip(
					"Car will depart a monorail stop as soon as possible."
				);

				SpacingV();

				Checkbox("Hand Brake", &monocar.flags.stop);
				if (IsItemHovered()) tip(
					"Car will stop immediately but continue to hold a reservation"
					" for the next tower, which may block other cars."
				);

				PopID();

				EndTabItem();
			}

			if (en.spec->computer && BeginTabItem("Computer", nullptr, focusedTab())) {
				auto& computer = en.computer();

				PushID("computer");

				if (BeginTable("##toggles", 3)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.33);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.33);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.34);

					TableNextColumn();
					bool enabled = en.isEnabled();
					if (Checkbox("Power On", &enabled)) {
						en.setEnabled(enabled);
						computer.reboot();
					}

					TableNextColumn();
					if (Checkbox("Debug Mode", &computer.debug)) {
						computer.reboot();
					}

					TableNextColumn();
					switch (computer.err) {
						case Computer::Ok: {
							Print("System Ready");
							break;
						}
						case Computer::What: {
							Print("Report a Bug!");
							if (IsItemHovered()) tip(
								"The entity is in a state that should be impossible."
								" Please report a bug in the game and attach your program!"
							);
							break;
						}
						case Computer::StackUnderflow: {
							Print("Stack Underflow");
							break;
						}
						case Computer::StackOverflow: {
							Print("Stack Overflow");
							break;
						}
						case Computer::OutOfBounds: {
							Print("Out of Bounds");
							break;
						}
						case Computer::Syntax: {
							Print("Syntax Error");
							break;
						}
					}
					EndTable();
				}

				if (BeginCombo("##source", nullptr)) {
					for (const auto& file: std::filesystem::directory_iterator("programs/")) {
						if (Selectable((const char*)(file.path().c_str()), false)) {
							auto slurp = std::ifstream(file.path());
							computer.source = std::string(
								(std::istreambuf_iterator<char>(slurp)),
								(std::istreambuf_iterator<char>())
							);
						}
					}
					EndCombo();
				}
				SameLine();
				if (Button("Load Program")) {
					computer.reboot();
				}

				{
					Section("Environment Variables");
					if (IsItemHovered()) {
						tip(
							"Environment signals key/value pairs are written "
							"to RAM each tick starting at address 00."
						);
					}
					uint env = 0;
					std::vector<Signal::Key> drop;

					if (BeginTable("##environment", 2)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.6);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

						for (auto& signal: computer.env) {
							TableNextRow();
							TableNextColumn();
							signalConstant(env, signal, false);
							TableNextColumn();
							if (Button(fmtc("Remove##forget-%u", env))) {
								drop.push_back(signal.key);
							}
							if (IsItemHovered()) {
								tip("Remove this environment signal");
							}
							env++;
						}
						TableNextRow();
						TableNextColumn();
						signalConstant(env, computer.add, false);
						TableNextColumn();
						if (Button("Create##define-env")) {
							computer.env[computer.add.key].value = computer.add.value;
							computer.add.key.type = Signal::Type::Stack;
							computer.add.key.id = 0;
							computer.add.value = 0;
						}
						if (IsItemHovered()) {
							tip("Add or overwrite an environment signal");
						}

						EndTable();
					}
					for (auto& key: drop) {
						computer.env.erase(key);
					}
				}

				auto memory = [&](int id, minivec<int>& mem) {
					if (BeginTable(fmtc("##memory%d", id), 9)) {
						for (uint i = 0; i < mem.size(); ) {
							TableNextRow();
							Print(fmtc("%03d", i));

							for (uint j = 0; j < 8 && i < mem.size(); j++, i++) {
								TableNextColumn();
								Print(fmt("%d", mem[i]));
							}
						}
						EndTable();
					}
				};

				Section("RAM");
				memory(1, computer.ram);
				if (IsItemHovered()) {
					tip(
						"RAM is read/write memory for use by the program."
					);
				}

				Section("ROM");
				memory(2, computer.rom);
				if (IsItemHovered()) {
					tip(
						"ROM is read-only memory used by the compiler to store static "
						"stuff like environment variables and string literals."
					);
				}

				Section(fmtc("Program: %u cycles", (uint)computer.code.size()));
				Print(computer.log.size() ? computer.log.c_str(): "(no output)");

				NewLine();

				for (uint i = 0; i < computer.code.size(); i++) {
					auto& instruction = computer.code[i];

					switch (instruction.opcode) {
						case Computer::Nop: {
							Print(fmtc("Nop, %d", instruction.value));
							break;
						}
						case Computer::Rom: {
							Print(fmtc("Rom, %d", instruction.value));
							break;
						}
						case Computer::Ram: {
							Print(fmtc("Ram, %d", instruction.value));
							break;
						}
						case Computer::Jmp: {
							Print(fmtc("Jmp, %d", instruction.value));
							break;
						}
						case Computer::Jz: {
							Print(fmtc("Jz, %d", instruction.value));
							break;
						}
						case Computer::Call: {
							Print(fmtc("Call, %d", instruction.value));
							break;
						}
						case Computer::Ret: {
							Print(fmtc("Ret, %d", instruction.value));
							break;
						}
						case Computer::Push: {
							Print(fmtc("Push, %d", instruction.value));
							break;
						}
						case Computer::Pop: {
							Print(fmtc("Pop, %d", instruction.value));
							break;
						}
						case Computer::Dup: {
							Print(fmtc("Dup, %d", instruction.value));
							break;
						}
						case Computer::Drop: {
							Print(fmtc("Drop, %d", instruction.value));
							break;
						}
						case Computer::Over: {
							Print(fmtc("Over, %d", instruction.value));
							break;
						}
						case Computer::Swap: {
							Print(fmtc("Swap, %d", instruction.value));
							break;
						}
						case Computer::Fetch: {
							Print(fmtc("Fetch, %d", instruction.value));
							break;
						}
						case Computer::Store: {
							Print(fmtc("Store, %d", instruction.value));
							break;
						}
						case Computer::Lit: {
							Print(fmtc("Lit, %d", instruction.value));
							break;
						}
						case Computer::Add: {
							Print(fmtc("Add, %d", instruction.value));
							break;
						}
						case Computer::Sub: {
							Print(fmtc("Sub, %d", instruction.value));
							break;
						}
						case Computer::Mul: {
							Print(fmtc("Mul, %d", instruction.value));
							break;
						}
						case Computer::Div: {
							Print(fmtc("Div, %d", instruction.value));
							break;
						}
						case Computer::Mod: {
							Print(fmtc("Mod, %d", instruction.value));
							break;
						}
						case Computer::And: {
							Print(fmtc("And, %d", instruction.value));
							break;
						}
						case Computer::Or: {
							Print(fmtc("Or, %d", instruction.value));
							break;
						}
						case Computer::Xor: {
							Print(fmtc("Xor, %d", instruction.value));
							break;
						}
						case Computer::Inv: {
							Print(fmtc("Inv, %d", instruction.value));
							break;
						}
						case Computer::Eq: {
							Print(fmtc("Eq, %d", instruction.value));
							break;
						}
						case Computer::Ne: {
							Print(fmtc("Ne, %d", instruction.value));
							break;
						}
						case Computer::Lt: {
							Print(fmtc("Lt, %d", instruction.value));
							break;
						}
						case Computer::Lte: {
							Print(fmtc("Lte, %d", instruction.value));
							break;
						}
						case Computer::Gt: {
							Print(fmtc("Gt, %d", instruction.value));
							break;
						}
						case Computer::Gte: {
							Print(fmtc("Gte, %d", instruction.value));
							break;
						}
						case Computer::Min: {
							Print(fmtc("Min, %d", instruction.value));
							break;
						}
						case Computer::Max: {
							Print(fmtc("Max, %d", instruction.value));
							break;
						}
						case Computer::Print: {
							Print(fmtc("Print, %d", instruction.value));
							break;
						}
						case Computer::DumpTop: {
							Print(fmtc("DumpTop, %d", instruction.value));
							break;
						}
						case Computer::DumpStack: {
							Print(fmtc("DumpStack, %d", instruction.value));
							break;
						}
						case Computer::Send: {
							Print(fmtc("Send, %d", instruction.value));
							break;
						}
						case Computer::Recv: {
							Print(fmtc("Recv, %d", instruction.value));
							break;
						}
						case Computer::Sniff: {
							Print(fmtc("Sniff, %d", instruction.value));
							break;
						}
						case Computer::Now: {
							Print(fmtc("Now, %d", instruction.value));
							break;
						}
					}

					if (computer.ip == (int)i) {
						SameLine();
						Print("<--ip");
					}
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->router && BeginTabItem("Wifi Router", nullptr, focusedTab())) {
				auto& router = en.router();
				auto& networker = en.networker();

				PushID("router");

				int i = 0;
				int clear = -1;

				auto wifiConnection = [&](int id, int* nic) {
					std::string title = networker.interfaces[*nic].network
						? networker.interfaces[*nic].network->ssid: "Wifi Interface 1";
					if (BeginCombo(fmtc("##wifi-connection-%d", id), title.c_str())) {
						for (int i = 0, l = networker.interfaces.size(); i < l; i++) {
							auto& interface = networker.interfaces[i];
							std::string title = interface.network ? interface.network->ssid: fmt("Wifi Interface %d", i+1);
							if (Selectable(title.c_str(), *nic == i)) *nic = i;
						}
						EndCombo();
					}
				};

				float space = GetContentRegionAvail().x;
				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				int n = 0;
				for (auto& rule: router.rules) {

					auto check = [&]() {
						return rule.condition.evaluate(networker.interfaces[rule.nicSrc].network
							? networker.interfaces[rule.nicSrc].network->signals : Signal::NoSignals);
					};

					auto title = fmt("Rule %d", ++n);

					SpacingV();
					if (rule.mode == Router::Rule::Mode::Alert && rule.alert == Router::Rule::Alert::Warning && check()) {
						Warning(title);
					}
					else
					if (rule.mode == Router::Rule::Mode::Alert && rule.alert == Router::Rule::Alert::Critical && check()) {
						Alert(title);
					}
					else {
						Section(title);
					}

					if (BeginTable(fmtc("##table%d", ++i), 5)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.1);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.2);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.225);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);

						signalCondition(++i, rule.condition, false, check);

						TableNextColumn();
						if (Button(fmtc("%s##clear%d", ICON_FA_TRASH), ImVec2(-1,0))) {
							clear = &rule - router.rules.data();
						}
						EndTable();
					}
					SpacingV();

					if (BeginTable(fmtc("##table%d", ++i), 2)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.63);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

						const char* selectedMode = nullptr;
						switch (rule.mode) {
							case Router::Rule::Mode::Forward: selectedMode = "Forward signal"; break;
							case Router::Rule::Mode::Generate: selectedMode = "Generate signal"; break;
							case Router::Rule::Mode::Alert: selectedMode = "Raise alert"; break;
						}
						TableNextColumn();
						PushItemWidth(-1);
						if (BeginCombo(fmtc("##rule%d", ++i), selectedMode)) {
							if (Selectable("Forward signal", rule.mode == Router::Rule::Mode::Forward)) rule.mode = Router::Rule::Mode::Forward;
							if (Selectable("Generate signal", rule.mode == Router::Rule::Mode::Generate)) rule.mode = Router::Rule::Mode::Generate;
							if (Selectable("Raise alert", rule.mode == Router::Rule::Mode::Alert)) rule.mode = Router::Rule::Mode::Alert;
							EndCombo();
						}
						PopItemWidth();
						SpacingV();
						TableNextColumn();
						Print("Action when true");

						if (rule.condition.key != Signal::Key(Signal::Meta::Now)) {
							TableNextColumn();
							PushItemWidth(-1);
							wifiConnection(++i, &rule.nicSrc);
							PopItemWidth();
							SpacingV();
							TableNextColumn();
							Print("Input network");
						}

						switch (rule.mode) {
							case Router::Rule::Mode::Forward: {
								TableNextColumn();
								PushItemWidth(-1);
								wifiConnection(++i, &rule.nicDst);
								PopItemWidth();
								SpacingV();
								TableNextColumn();
								Print("Output network");
								break;
							}
							case Router::Rule::Mode::Generate: {
								TableNextColumn();
								PushItemWidth(-1);
								wifiConnection(++i, &rule.nicDst);
								PopItemWidth();
								SpacingV();
								TableNextColumn();
								Print("Output network");

								TableNextColumn();
								PushItemWidth(space*0.397);
								signalKey(++i, rule.signal.key, false);
								PopItemWidth();
								SetNextItemWidth(-1);
								SameLine();
								InputInt(fmtc("##int%d", ++i), &rule.signal.value);
								PopItemWidth();
								SpacingV();
								TableNextColumn();
								Print("Output signal");
								break;
							}
							case Router::Rule::Mode::Alert: {
								const char* selectedAlert = nullptr;
								switch (rule.alert) {
									case Router::Rule::Alert::Notice: selectedAlert = "Notice"; break;
									case Router::Rule::Alert::Warning: selectedAlert = "Warning"; break;
									case Router::Rule::Alert::Critical: selectedAlert = "Critical"; break;
								}
								TableNextColumn();
								PushItemWidth(-1);
								if (BeginCombo(fmtc("##alert-level-%d", ++i), selectedAlert)) {
									if (Selectable("Notice", rule.alert == Router::Rule::Alert::Notice)) rule.alert = Router::Rule::Alert::Notice;
									if (Selectable("Warning", rule.alert == Router::Rule::Alert::Warning)) rule.alert = Router::Rule::Alert::Warning;
									if (Selectable("Critical", rule.alert == Router::Rule::Alert::Critical)) rule.alert = Router::Rule::Alert::Critical;
									EndCombo();
								}
								PopItemWidth();
								SpacingV();
								TableNextColumn();
								Print("Alert level");

								TableNextColumn();
								int r = i;
								float s = GetFontSize() * 1.5;
								auto size = ImVec2(s, s);
								for (int i = 1, l = Sim::customIcons.size(); i < l; i++) {
									auto& icon = Sim::customIcons[i];
									bool active = i == rule.icon;
									if (GetContentRegionAvail().x < size.x) { NewLine(); SpacingV(); }
									if (active) PushStyleColor(ImGuiCol_Button, Color(0x009900ff));
									if (Button(fmtc("%s##icon-%d-%d", icon, r, i), size)) rule.icon = i;
									if (active) PopStyleColor(1);
									SameLine();
								}
								NewLine();
								TableNextColumn();
								Print("Alert icon");
								break;
							}
						}
						EndTable();
					}
				}

				if (clear >= 0) {
					router.rules.erase(clear);
				}

				Separator();
				SpacingV();

				if (Button("Add Rule##add")) {
					router.rules.resize(router.rules.size()+1);
				}

				PopStyleVar();
				PopID();

				EndTabItem();
			}

			if (en.spec->depot && BeginTabItem("Drone Port", nullptr, focusedTab())) {
				auto& depot = en.depot();

				PushID("depot");

				Checkbox("Construction", &depot.construction);
				if (IsItemHovered()) tip("Drone Port will dispatch drones to construct ghosts.");

				Checkbox("Deconstruction", &depot.deconstruction);
				if (IsItemHovered()) tip("Drone Port will dispatch drones to deconstruct ghosts.");

				if (en.spec->depotAssist && en.spec->networker) {
					Checkbox("Wifi Network", &depot.network);
					if (IsItemHovered()) tip("Drone Port will dispatch drones to networked containers.");
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->balancer && BeginTabItem("Balancer", nullptr, focusedTab())) {
				auto& balancer = en.balancer();

				PushID("balancer");

				SpacingV();
				Section("Priority");
				Checkbox("Input", &balancer.priority.input);
				Checkbox("Output", &balancer.priority.output);

				SpacingV();
				Section("Item Filtering");

				uint remove = 0;
				uint insert = 0;

				int i = 0;
				for (auto iid: balancer.filter) {
					if (ButtonStrip(i++, fmtc(" %s ", Item::get(iid)->title))) remove = iid;
					if (IsItemHovered()) tip("Remove item from filter set.");
				}

				auto pick = ButtonStrip(i++, balancer.filter.size() ? " + Item ": " Set Filter ");
				if (IsItemHovered()) tip("Add item to filter set.");
				if (itemPicker(pick)) insert = itemPicked;

				if (remove) balancer.filter.erase(remove);
				if (insert) balancer.filter.insert(insert);

				PopID();

				EndTabItem();
			}

			if (en.spec->tube && BeginTabItem("Tube", nullptr, focusedTab())) {
				auto& tube = en.tube();

				PushID("tube");

				SpacingV();
				Section("Input / Output Modes");
				auto mode = [&](Tube::Mode m) {
					switch (m) {
						case Tube::Mode::BeltOrTube: return "belt-or-tube";
						case Tube::Mode::BeltOnly: return "belt-only";
						case Tube::Mode::TubeOnly: return "tube-only";
						case Tube::Mode::BeltPriority: return "belt-priority";
						case Tube::Mode::TubePriority: return "tube-priority";
					}
					return "belt-or-tube";
				};

				if (BeginCombo("Input", mode(tube.input))) {
					for (auto m: std::vector<Tube::Mode>{
						Tube::Mode::BeltOrTube,
						Tube::Mode::BeltOnly,
						Tube::Mode::TubeOnly,
						Tube::Mode::BeltPriority,
						Tube::Mode::TubePriority,
					}) {
						if (Selectable(mode(m), tube.input == m)) {
							tube.input = m;
						}
					}
					EndCombo();
				}

				if (BeginCombo("Output", mode(tube.output))) {
					for (auto m: std::vector<Tube::Mode>{
						Tube::Mode::BeltOrTube,
						Tube::Mode::BeltOnly,
						Tube::Mode::TubeOnly,
						Tube::Mode::BeltPriority,
						Tube::Mode::TubePriority,
					}) {
						if (Selectable(mode(m), tube.output == m)) {
							tube.output = m;
						}
					}
					EndCombo();
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->networker && BeginTabItem("Wifi Network", nullptr, focusedTab())) {
				auto& networker = en.networker();

				PushID("networker");

				for (uint i = 0, l = std::min((uint)networker.interfaces.size(), 4u); i < l; i++) {
					auto& interface = networker.interfaces[i];

					SpacingV();
					Section(fmtc("Wifi Interface %u", i+1));
					PushID(fmtc("networker.interface%d", i));

					if (!interface.network && interface.ssid.size()) {
						Alert(fmt("Network %s is not in range.", interface.ssid));
					}

					if (interface.network) {
						Notice(fmt("Connected to \"%s\".", interface.ssid));
					}

					if (networker.isHub() && !interface.network) {
						bool saveSSID = InputTextWithHint(fmtc("##%u", i), "<new network name>", interfaces[i], sizeof(interfaces[i]), ImGuiInputTextFlags_EnterReturnsTrue);
						inputFocused = IsItemActive();
						SameLine();
						if (Button(fmtc("Create##%u", i)) || saveSSID) {
							interface.ssid = std::string(interfaces[i]);
							std::snprintf(interfaces[i], sizeof(interfaces[i]), "%s", interface.ssid.c_str());
							Networker::rebuild = true;
						}
						SpacingV();
					}

					if (BeginCombo("##networks", interface.ssid.c_str())) {
						for (auto ssid: networker.ssids()) {
							if (Selectable(ssid.c_str(), interface.ssid == ssid)) {
								interface.ssid = ssid;
								std::snprintf(interfaces[i], sizeof(interfaces[i]), "%s", interface.ssid.c_str());
								interfaces[i][0] = 0;
								Networker::rebuild = true;
							}
						}
						EndCombo();
					}

					if (!interface.network) {
						SameLine();
						Print("All networks");
					}

					if (interface.network) {
						SameLine();
						if (Button("Disconnect")) {
							interface.ssid = "";
							interfaces[i][0] = 0;
							Networker::rebuild = true;
						}
					}

					BeginChild("##signals", ImVec2(-1,300));

						if (operableStore) {
							Checkbox("Transmit items in storage", &en.store().transmit);
						}

						if (interface.network) {
							Print("Network signals");
							for (auto signal: interface.network->signals) {
								Print(std::string(signal).c_str());
							}
						}
						else {
							Print("Local signals");
							for (auto signal: interface.signals) {
								Print(std::string(signal).c_str());
							}
						}

					EndChild();

					PopID();
					Separator();
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->powerpole && BeginTabItem("Electricity", nullptr, focusedTab())) {
				auto& powerpole = en.powerpole();

				PushID("powerpole");

				SpacingV();
				powerpoleNotice(powerpole);

				if (powerpole.network) {
					auto network = powerpole.network;

					auto capacity = network->capacityReady;
					SpacingV();
					Print(fmtc("Electricity Network #%u", network->id)); SameLine();
					PrintRight(fmtc("%s / %s", network->demand.formatRate(), capacity.formatRate()));
					OverflowBar(network->load, ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));
					OverflowBar(network->bufferedLevel.portion(network->bufferedLimit), ImColorSRGB(0xffff00ff), ImColorSRGB(0x9999ddff));

//					SpacingV();
//					OverflowBar(network->satisfaction, ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));
//					OverflowBar(network->charge, ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));
//					OverflowBar(network->discharge, ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));

					struct Stat {
						Spec* spec = nullptr;
						Energy energy = 0;
					};

					minivec<Stat> production;
					minivec<Stat> consumption;

					for (auto& [spec,series]: network->production) {
						auto energy = Energy(series.ticks[series.tick(Sim::tick-1)]);
						if (energy) production.push_back({spec,energy});
					}

					for (auto& [spec,series]: network->consumption) {
						auto energy = Energy(series.ticks[series.tick(Sim::tick-1)]);
						if (energy) consumption.push_back({spec,energy});
					}

					std::sort(production.begin(), production.end(), [&](const auto& a, const auto& b) {
						return a.energy > b.energy;
					});

					std::sort(consumption.begin(), consumption.end(), [&](const auto& a, const auto& b) {
						return a.energy > b.energy;
					});

					SpacingV();
					SpacingV();
					if (BeginTable("list", 2)) {
						float column = GetContentRegionAvail().x/2;
						TableSetupColumn("Consumers", ImGuiTableColumnFlags_WidthFixed, column);
						TableSetupColumn("Producers", ImGuiTableColumnFlags_WidthFixed, column);
						TableHeadersRow();

						TableNextColumn();
						SpacingV();
						for (auto& [spec,energy]: consumption) {
							Print(spec->title.c_str());
							SameLine(); PrintRight(fmtc("%s", energy.formatRate()));
							PushStyleColor(ImGuiCol_PlotHistogram, Color(0xaaaaaaff));
							SmallBar(energy.portion(network->demand));
							PopStyleColor(1);
							SpacingV();
						}

						TableNextColumn();
						SpacingV();
						for (auto& [spec,energy]: production) {
							Print(spec->title.c_str());
							SameLine(); PrintRight(fmtc("%s", energy.formatRate()));
							PushStyleColor(ImGuiCol_PlotHistogram, Color(0xaaaaaaff));
							SmallBar(energy.portion(network->supply));
							PopStyleColor(1);
							SpacingV();
						}

						EndTable();
					}
				}

				PopID();
				EndTabItem();
			}

			if (BeginTabItem("Settings", nullptr, focusedTab())) {

				PushID("entity");
					{
						bool permanent = en.isPermanent();
						if (Checkbox("Permanent", &permanent)) en.setPermanent(permanent);
						if (IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) tip(
							"Permanent structures cannot be accidentally deconstructed."
							"\n\nWARNING -- Removing permanent structures that have not"
							" yet been unlocked may make it difficult to continue!"
						);
					}
					if (en.spec->enable) {
						bool enabled = en.isEnabled();
						if (Checkbox("Enabled", &enabled)) en.setEnabled(enabled);
					}
					if (en.spec->generateElectricity) {
						bool generating = en.isGenerating();
						if (Checkbox("Generate Electricity", &generating)) en.setGenerating(generating);
					}
					if (en.spec->named) {
						bool saveEnter = InputText("Name", name, sizeof(name), ImGuiInputTextFlags_EnterReturnsTrue);
						inputFocused = IsItemActive();
						SameLine();
						if (Button(" Save ") || saveEnter) {
							if (std::strlen(name)) en.rename(name);
						}
					}
					if (en.spec->consumeFuel) {
						Burner& burner = en.burner();
						Print("Fuel");
						LevelBar(burner.energy.portion(burner.buffer));
						for (Stack stack: burner.store.stacks) {
							Item* item = Item::get(stack.iid);
							Print(fmtc("%s(%u)", item->title, stack.size));
						}
					}
				PopID();

				EndTabItem();
			}

			EndTabBar();
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

