#include "gui.h"
#include "scene.h"
#include "sim.h"
#include "entity.h"
#include "gui-entity.h"
#include "plan.h"
#include "enemy.h"

GUI gui;

namespace Log {
	channel<std::string,-1> log;
}

void GUI::init() {
	statsPopup = new StatsPopup2();
	entityPopup = new EntityPopup2();
	recipePopup = new RecipePopup();
	upgradePopup = new UpgradePopup();
	signalsPopup = new SignalsPopup();
	planPopup = new PlanPopup();
	mapPopup = new MapPopup();
	vehiclePopup = new VehiclePopup();
	paintPopup = new PaintPopup();
	mainMenu = new MainMenu();
	debugMenu = new DebugMenu();
	loading = new LoadingPopup();
	hud = new HUD();
	toolbar = new Toolbar();
}

void GUI::reset() {
	popup = nullptr;
	doMenu = false;
	doDebug = false;
	doSave = false;
	doLog = false;
	doStats = false;
	doBuild = false;
	doUpgrade = false;
	doSignals = false;
	doPlan = false;
	doMap = false;
	doVehicles = false;
	doPaint = false;
	doEscape = false;
	doQuit = false;
	focused = false;
	prepared = false;

	delete statsPopup;
	statsPopup = nullptr;
	delete entityPopup;
	entityPopup = nullptr;
	delete recipePopup;
	recipePopup = nullptr;
	delete upgradePopup;
	upgradePopup = nullptr;
	delete signalsPopup;
	signalsPopup = nullptr;
	delete planPopup;
	planPopup = nullptr;
	delete mapPopup;
	mapPopup = nullptr;
	delete vehiclePopup;
	vehiclePopup = nullptr;
	delete paintPopup;
	paintPopup = nullptr;
	delete mainMenu;
	mainMenu = nullptr;
	delete debugMenu;
	debugMenu = nullptr;
	delete loading;
	loading = nullptr;
	delete hud;
	hud = nullptr;
	delete toolbar;
	toolbar = nullptr;

	ups = 0;
	fps = 0;

	controlHintsRelated.clear();
	controlHintsGeneral.clear();
	controlHintsSpecific.clear();

	lastConstruct.planId = 0;
	lastConstruct.plan = nullptr;
	lastConstruct.pos = Point::Zero;
	lastConstruct.tick = 0;
}

void GUI::prepare() {
	statsPopup->prepare();
	entityPopup->prepare();
	recipePopup->prepare();
	upgradePopup->prepare();
	signalsPopup->prepare();
	planPopup->prepare();
	vehiclePopup->prepare();
	paintPopup->prepare();
	mapPopup->prepare();
	mainMenu->prepare();
	debugMenu->prepare();
	loading->prepare();
	prepared = true;
}

void GUI::togglePopup(Popup* p) {
	if (popup && popup->inputFocused) return;

	if (popup == p) {
		popup->show(false);
		popup = nullptr;
	} else {
		if (popup) popup->show(false);
		popup = p;
		popup->show(true);
	}
};

void GUI::render() {
	using namespace ImGui;

	for (auto msg: Log::log.recv_all()) {
		loading->print(msg);
	}

	if (prepared && hud) hud->draw();
	if (prepared && toolbar) toolbar->draw();
	if (popup) popup->draw();
}

bool GUI::active() {
	return popup != nullptr;
}

void GUI::update() {
	controlHintsRelated.clear();
	controlHintsSpecific.clear();
	controlHintsGeneral.clear();

	bool worldFocused = !focused;

	std::set<Config::Action> actionsEnabled;

	auto specCopyable = [&](Spec* spec) {
		return !spec->junk && spec->licensed && spec->clone;
	};

	auto specDeletable = [&](Spec* spec) {
		return (spec->junk || spec->deconstructable) && !spec->forceDelete;
	};

	auto specForceDeletable = [&](Spec* spec) {
		return (spec->junk || spec->deconstructable) && spec->forceDelete;
	};

	auto specUpgradable = [&](Spec* spec) {
		return spec->upgrade && spec->upgrade->licensed;
	};

	auto hoveringDirecting = [&]() {
		return scene.directing && scene.hovering && scene.directing->id == scene.hovering->id;
	};

	auto hoveringDirectable = [&]() {
		return scene.hovering && scene.hovering->spec->directable();
	};

	auto hoveringPermanentEntity = [&]() {
		return scene.hovering && (scene.hovering->flags & Entity::PERMANENT) != 0;
	};

	auto hoveringCopyable = [&]() {
		return scene.hovering && specCopyable(scene.hovering->spec);
	};

	auto hoveringDeletable = [&]() {
		return scene.hovering && specDeletable(scene.hovering->spec) && !hoveringDirecting() && !hoveringPermanentEntity();
	};

	auto hoveringForceDeletable = [&]() {
		return scene.hovering && !hoveringDirecting() && !hoveringPermanentEntity() && specForceDeletable(scene.hovering->spec) ;
	};

	auto hoveringCutable = [&]() {
		return hoveringCopyable() && hoveringDeletable();
	};

	auto hoveringUpgradable = [&]() {
		return scene.hovering && specUpgradable(scene.hovering->spec);
	};

	auto hoveringConfigurable = [&]() {
		return hoveringCopyable();
	};

	auto somethingSelected = [&]() {
		return scene.selecting && scene.selected.size() > 0;
	};

	auto somethingSelectedCopyable = [&]() {
		if (somethingSelected()) {
			for (auto ge: scene.selected) if (specCopyable(ge->spec)) return true;
		}
		return false;
	};

	auto somethingSelectedDeletable = [&]() {
		if (somethingSelected()) {
			for (auto ge: scene.selected) if (specDeletable(ge->spec)) return true;
		}
		return false;
	};

	auto somethingSelectedForceDeletable = [&]() {
		if (somethingSelected()) {
			for (auto ge: scene.selected) if (specForceDeletable(ge->spec)) return true;
		}
		return false;
	};

	auto somethingSelectedCutable = [&]() {
		return somethingSelectedCopyable() && somethingSelectedDeletable();
	};

	auto somethingSelectedUpgradable = [&]() {
		if (somethingSelected()) {
			for (auto ge: scene.selected) if (specUpgradable(ge->spec)) return true;
		}
		return false;
	};

	// pipette and copy-single-entity are nearly identical, except copy preserves config
	auto cloneSingle = [&](bool cfg) {
		if (!scene.hovering) return;
		Sim::locked([&]() {
			auto se = scene.hovering;
			if (Entity::exists(se->id)) {
				Entity& en = Entity::get(se->id);
				auto ge = new GuiFakeEntity(se->spec->pipette ? se->spec->pipette: se->spec);

				ge->dir(en.dir());
				if (en.spec->pipette && en.spec->conveyor) {
					ge->dir(en.spec->conveyorOutput.transform(en.dir().rotation()));
				}

				ge->pos(en.pos());
				ge->floor(0.0f);
				if (cfg) ge->getConfig(en);

				auto plan = new Plan(scene.hovering->pos());
				plan->add(ge);
				plan->config = cfg;
				scene.planPush(plan);
			}
		});
	};

	if (hoveringCopyable() || somethingSelectedCopyable()) {
		controlHintsSpecific["Ctrl+C"] = "Copy";
		actionsEnabled.insert(Config::Action::Copy);
	}

	if (hoveringCutable() || somethingSelectedCutable()) {
		controlHintsSpecific["Ctrl+X"] = "Cut";
		actionsEnabled.insert(Config::Action::Cut);
	}

	auto actionCopy = [&]() {
		if (scene.connecting) {
			delete scene.connecting;
			scene.connecting = nullptr;
		}

		if (scene.routing) {
			delete scene.routing;
			scene.routing = nullptr;
			scene.routingHistory.clear();
		}

		if (scene.selecting && scene.selected.size()) {
			Sim::locked([&]() {
				uint piles = 0;
				uint others = 0;
				for (auto se: scene.selected) {
					if (se->spec->pile) piles++; else others++;
				}
				std::vector<GuiEntity*> group;
				for (auto se: scene.selected) {
					if (se->spec->junk) continue;
					if (!se->spec->plan) continue;
					if (se->spec->pile && others) continue;
					if (!se->spec->licensed) continue;

					if (Entity::exists(se->id)) {
						group.push_back(se);
					}
				}
				if (group.size()) {
					Point min = group.front()->pos();
					Point max = group.front()->pos();
					for (auto ge: group) {
						min.x = std::min(min.x, ge->pos().x);
						min.z = std::min(min.z, ge->pos().z);
						max.x = std::max(max.x, ge->pos().x);
						max.z = std::max(max.z, ge->pos().z);
					}
					min.y = 0;
					max.y = 0;
					Point diag = max-min;
					Point centroid = min+(diag*0.5);

					auto plan = new Plan(centroid);
					plan->config = true;

					for (auto se: group) {
						if (!Entity::exists(se->id)) continue;
						Entity& en = Entity::get(se->id);
						auto ge = new GuiFakeEntity(se->spec);
						ge->dir(en.dir());
						ge->pos(en.pos());
						ge->getConfig(en, true);
						plan->add(ge);
					}

					scene.planPush(plan);
				}
			});
			scene.selection = {Point::Zero, Point::Zero};
			scene.selecting = false;
		}
		else
		if (scene.hovering && !scene.hovering->spec->junk && scene.hovering->spec->clone && scene.hovering->spec->licensed) {
			cloneSingle(true);
		}
	};

	if (!scene.placing && Plan::all.size()) {
		controlHintsGeneral["Ctrl+V"] = "Paste last";
		actionsEnabled.insert(Config::Action::Paste);
	}

	auto actionPaste = [&]() {
		Sim::locked([&]() {
			scene.planPaste();
		});
	};

	if (hoveringConfigurable()) {
		controlHintsSpecific["Alt+C"] = "Copy config";
		actionsEnabled.insert(Config::Action::CopyConfig);
	}

	if (scene.settings && (scene.hovering || scene.selected.size())) {
		controlHintsSpecific["Atl+V"] = "Paste config";
		actionsEnabled.insert(Config::Action::PasteConfig);
	}

	auto actionCopyConfig = [&]() {
		if (scene.settings) {
			delete scene.settings;
			scene.settings = nullptr;
		}

		Sim::locked([&]() {
			auto en = Entity::find(scene.hovering->id);
			if (!en) return;
			scene.settings = new GuiFakeEntity(en->spec);
			scene.settings->getConfig(*en);
		});
	};

	auto actionPasteConfig = [&]() {
		if (!scene.settings) return;
		Sim::locked([&]() {
			if (scene.selected.size()) {
				for (auto ge: scene.selected) {
					auto en = Entity::find(ge->id);
					if (en) scene.settings->setConfig(*en);
				}
			}
			if (scene.hovering) {
				auto en = Entity::find(scene.hovering->id);
				if (en) scene.settings->setConfig(*en);
			}
		});
	};

	if (scene.placing || (scene.hovering && !scene.hovering->spec->junk && scene.hovering->spec->clone)) {
		controlHintsSpecific["Q"] = "Pipette";
		actionsEnabled.insert(Config::Action::Pipette);
	}

	auto actionPipette = [&]() {
		scene.planDrop();

		if (scene.connecting) {
			delete scene.connecting;
			scene.connecting = nullptr;
		}

		if (scene.routing) {
			delete scene.routing;
			scene.routing = nullptr;
			scene.routingHistory.clear();
		}

		if (scene.hovering && !scene.hovering->spec->junk && scene.hovering->spec->clone && scene.hovering->spec->licensed) {
			cloneSingle(false);
		}
	};

	if (hoveringUpgradable() || somethingSelectedUpgradable()) {
		controlHintsSpecific["U"] = "Upgrade";
		actionsEnabled.insert(Config::Action::Upgrade);
	}

	auto actionUpgrade = [&]() {
		if (scene.selecting && scene.selected.size()) {
			Sim::locked([&]() {
				for (auto se: scene.selected) {
					if (!Entity::exists(se->id)) continue;
					Entity::get(se->id).upgrade();
				}
			});
		}
		else
		if (scene.hovering && scene.hovering->spec->upgrade) {
			Sim::locked([&]() {
				auto se = scene.hovering;
				if (!Entity::exists(se->id)) return;
				Entity::get(se->id).upgrade();
			});
		}
	};

	if (!somethingSelected() && scene.hovering && scene.hovering->spec->upgrade && scene.hovering->spec->upgradeCascade.size()) {
		controlHintsSpecific["Shift+U"] = "Upgrade cascade";
		actionsEnabled.insert(Config::Action::UpgradeCascade);
	}

	auto actionUpgradeCascade = [&]() {
		if (!somethingSelected() && scene.hovering && scene.hovering->spec->upgrade && scene.hovering->spec->upgradeCascade.size()) {
			Sim::locked([&]() {
				auto se = scene.hovering;
				if (!Entity::exists(se->id)) return;
				Entity::upgradeCascade(se->id);
			});
		}
	};

	if ((scene.placing && scene.placing->canRotate()) || (scene.hovering && scene.hovering->spec->rotateExtant)) {
		controlHintsSpecific["R"] = "Rotate";
		actionsEnabled.insert(Config::Action::Rotate);
	}

	auto actionRotate = [&]() {
		if (scene.placing) {
			scene.placing->rotate();
		}
		else
		if (scene.hovering) {
			Sim::locked([&]() {
				if (Entity::exists(scene.hovering->id)) {
					Entity::get(scene.hovering->id)
						.rotate();
				}
			});
		}
	};

	if (scene.placing && scene.placing->canCycle()) {
		controlHintsRelated["C"] = scene.placing->entities[0]->spec->cycle->title.c_str();
		actionsEnabled.insert(Config::Action::Cycle);
	}

	auto actionCycle = [&]() {
		scene.placing->cycle();
	};

	bool hoveringOperable = worldFocused && scene.hovering && !scene.hovering->ghost && scene.hovering->spec->operable();

	if (!scene.placing
		&& !scene.routing
		&& !scene.connecting
		&& !scene.selecting
		&& hoveringOperable
	){
		controlHintsSpecific["LClick"] = "Open";
		actionsEnabled.insert(Config::Action::Open);
	}

	auto actionOpen = [&]() {
		if (popup) popup->show(false);
		popup = entityPopup;
		popup->show(true);
		entityPopup->useEntity(scene.hovering->id);
	};

	if (!scene.placing
		&& !scene.routing
		&& !scene.connecting
		&& !scene.selecting
		&& !hoveringDirecting()
		&& hoveringDirectable()
	){
		controlHintsSpecific["Ctrl+LClick"] = "Take control";
		actionsEnabled.insert(Config::Action::Direct);
	}

	auto actionDirect = [&]() {
		if (scene.directing && scene.hovering && scene.hovering->id == scene.directing->id) {
			delete scene.directing;
			scene.directing = nullptr;
		} else
		if (hoveringDirectable()) {
			delete scene.directing;
			scene.directing = new GuiEntity(scene.hovering->id);
		}
	};

	if (scene.hovering && (scene.hovering->spec->cartWaypoint || scene.hovering->spec->tube || scene.hovering->spec->monorail || scene.hovering->spec->powerpole)) {
		controlHintsSpecific["L"] = "Link up";
		actionsEnabled.insert(Config::Action::Link);
	}

	auto actionLink = [&]() {
		// link cartstop
		if (scene.hovering && (scene.hovering->spec->cartWaypoint || scene.hovering->spec->monorail)) {
			delete scene.routing;
			scene.routing = new GuiEntity(scene.hovering->id);

			auto ge = new GuiFakeEntity(scene.hovering->spec);
			ge->dir(scene.hovering->dir());
			ge->move(scene.hovering->pos())->floor(0.0f);

			scene.planDrop();
			auto plan = new Plan(ge->pos());
			plan->add(ge);
			plan->config = false;
			scene.planPush(plan);
			return;
		}

		// link connectable
		if (scene.hovering && scene.hovering->spec->tube) {
			delete scene.connecting;
			scene.connecting = new GuiEntity(scene.hovering->id);

			auto ge = new GuiFakeEntity(scene.hovering->spec);
			ge->dir(scene.hovering->dir());
			ge->move(scene.hovering->pos())->floor(0.0f);

			scene.planDrop();
			auto plan = new Plan(ge->pos());
			plan->add(ge);
			plan->config = false;
			scene.planPush(plan);
			return;
		}

		// link powerpole same as pipette
		if (scene.hovering && scene.hovering->spec->powerpole) {
			actionPipette();
			return;
		}
	};

	if (scene.connecting
		&& scene.connecting->spec->tube
		&& scene.hovering
		&& scene.connecting->id != scene.hovering->id
		&& scene.hovering->spec->tube
	){
		controlHintsSpecific["LClick"] = "Connect tube";
		controlHintsSpecific["RClick"] = "Disconnect tube";
		actionsEnabled.insert(Config::Action::Connect);
		actionsEnabled.insert(Config::Action::Disconnect);
	}

	auto actionConnect = [&]() {
		Sim::locked([&]() {
			if (Entity::exists(scene.connecting->id) && Entity::exists(scene.hovering->id)) {
				auto& en = Entity::get(scene.hovering->id);
				if (en.spec->tube) {
					if (!en.tube().connect(scene.connecting->id)) return;
					delete scene.connecting;
					scene.connecting = new GuiEntity(en.id);
				}
			}
		});
	};

	auto actionDisconnect = [&]() {
		Sim::locked([&]() {
			if (Entity::exists(scene.connecting->id) && Entity::exists(scene.hovering->id)) {
				auto& en = Entity::get(scene.hovering->id);
				if (en.spec->tube) {
					en.tube().disconnect(scene.connecting->id);
				}
			}
		});
	};

	if (scene.routing) {
		controlHintsSpecific["1"] = "Red";
		controlHintsSpecific["2"] = "Blue";
		controlHintsSpecific["3"] = "Green";
		actionsEnabled.insert(Config::Action::RouteRed);
		actionsEnabled.insert(Config::Action::RouteBlue);
		actionsEnabled.insert(Config::Action::RouteGreen);
	}

	auto actionRoute = [&](auto line) {
		if (scene.routing->spec->cartWaypoint)
			scene.routing->cartWaypointLine = line;
		if (scene.routing->spec->monorail)
			scene.routing->monorailLine = line;
	};

	if (scene.routing
		&& scene.hovering
		&& scene.routing->id != scene.hovering->id
		&& (scene.hovering->spec->cartWaypoint || scene.hovering->spec->monorail)
	){
		actionsEnabled.insert(Config::Action::RouteSetNext);
		controlHintsSpecific["LClick"] = "Set route";
	}

	auto actionRouteSetNext = [&]() {
		Sim::locked([&]() {
			if (Entity::exists(scene.routing->id) && Entity::exists(scene.hovering->id)) {
				if (Entity::get(scene.hovering->id).spec->cartWaypoint) {
					Entity::get(scene.routing->id).cartWaypoint()
						.setNext(scene.routing->cartWaypointLine, scene.hovering->pos());
					auto line = scene.routing->cartWaypointLine;
					scene.routingHistory.push(scene.routing->id);
					delete scene.routing;
					scene.routing = new GuiEntity(scene.hovering->id);
					scene.routing->cartWaypointLine = line;
				}
				if (Entity::get(scene.hovering->id).spec->monorail) {
					Entity::get(scene.routing->id).monorail()
						.connectOut(scene.routing->monorailLine, scene.hovering->id);
					auto line = scene.routing->monorailLine;
					scene.routingHistory.push(scene.routing->id);
					delete scene.routing;
					scene.routing = new GuiEntity(scene.hovering->id);
					scene.routing->monorailLine = line;
				}
			}
		});
	};

	if (scene.routing) {
		actionsEnabled.insert(Config::Action::RouteClrNext);
		controlHintsSpecific["RClick"] = "Clear route";
	}

	auto actionRouteClrNext = [&]() {
		Sim::locked([&]() {
			if (Entity::exists(scene.routing->id) && scene.routing->spec->cartWaypoint) {
				Entity::get(scene.routing->id).cartWaypoint()
					.clrNext(scene.routing->cartWaypointLine);
			}
			if (Entity::exists(scene.routing->id) && scene.routing->spec->monorail) {
				Entity::get(scene.routing->id).monorail()
					.disconnectOut(scene.routing->monorailLine);
			}
		});
	};

	if (scene.hovering && scene.hovering->spec->pipe) {
		controlHintsSpecific["Shift+F"] = "Flush pipe";
		actionsEnabled.insert(Config::Action::Flush);
	}

	if (scene.hovering && scene.hovering->spec->conveyor) {
		controlHintsSpecific["Shift+F"] = "Empty belt";
		actionsEnabled.insert(Config::Action::Flush);
	}

	if (scene.hovering && scene.hovering->spec->tube) {
		controlHintsSpecific["Shift+F"] = "Empty tube";
		actionsEnabled.insert(Config::Action::Flush);
	}

	auto actionFlush = [&]() {
		Sim::locked([&]() {
			if (!Entity::exists(scene.hovering->id)) return;

			if (scene.hovering->spec->pipe) {
				Pipe::get(scene.hovering->id).flush();
			}

			if (scene.hovering->spec->conveyor) {
				Conveyor::get(scene.hovering->id).flush();
			}

			if (scene.hovering->spec->tube) {
				Tube::get(scene.hovering->id).flush();
			}
		});
	};

	if (scene.placing) {
		controlHintsSpecific["LClick"] = "Construct";
		controlHintsSpecific["Shift+LClick"] = "Construct forced";
		actionsEnabled.insert(Config::Action::Construct);
		actionsEnabled.insert(Config::Action::ConstructForce);
	}

	auto actionConstruct = [&](bool force) {
		if (lastConstruct.plan
			&& scene.placing
			&& lastConstruct.plan == scene.placing
			&& lastConstruct.planId == scene.placing->id
			&& lastConstruct.tick > Sim::tick-60
			&& lastConstruct.pos.distance(scene.placing->position) < 0.5
		){
			return;
		}

		Sim::locked([&]() {
			if (force || (scene.placing->fits() && scene.placing->conforms())) {
				lastConstruct.planId = scene.placing->id;
				lastConstruct.plan = scene.placing;
				lastConstruct.pos = scene.placing->position;
				lastConstruct.tick = Sim::tick;

				struct PostConfig {
					GuiFakeEntity* te = nullptr;
					Entity* en = nullptr;
				};

				minivec<PostConfig> postConfiguration;

				for (uint i = 0; i < scene.placing->entities.size(); i++) {
					auto te = scene.placing->entities[i];

					for (auto& ej: Entity::intersecting(te->box().grow(1.0))) {
						if (ej->spec->junk) ej->deconstruct();
					}

					if (te->spec->once) {
						bool found = false;
						for (auto& en: Entity::all) {
							if (en.spec == te->spec) {
								found = true;
								break;
							}
						}
						if (found) continue;
					}

					// Plan will fit over existing entities in the right positions, but don't double up
					if (Entity::fits(te->spec, te->pos(), te->dir())) {
						Entity& en = Entity::create(Entity::next(), te->spec)
							.move(te->pos(), te->dir()).construct();

						if (scene.placing->config)
							postConfiguration.push({te,&en});

						if (scene.connecting && te->connectable(scene.connecting)) {
							if (en.spec->tube && en.tube().connect(scene.connecting->id)) {
								delete scene.connecting;
								scene.connecting = new GuiEntity(en.id);
							}
						}

						if (scene.placing->entities.size() == 1 && !scene.connecting && en.spec->tube) {
							scene.connecting = new GuiEntity(en.id);
						}

						if (scene.placing->entities.size() == 1 && en.spec->cartWaypoint && scene.routing && Entity::exists(scene.routing->id)) {
							Entity::get(scene.routing->id).cartWaypoint()
								.setNext(scene.routing->cartWaypointLine, en.pos());
						}

						if (scene.placing->entities.size() == 1 && en.spec->monorail && scene.routing && Entity::exists(scene.routing->id)) {
							Entity::get(scene.routing->id).monorail()
								.connectOut(scene.routing->monorailLine, en.id);
						}

						if (scene.placing->entities.size() == 1 && en.spec->cartWaypoint) {
							auto line = CartWaypoint::Red;
							if (scene.routing && scene.routing->spec->cartWaypoint) {
								line = scene.routing->cartWaypointLine;
								scene.routingHistory.push(scene.routing->id);
							}
							if (scene.routing) {
								delete scene.routing;
							}
							scene.routing = new GuiEntity(en.id);
							scene.routing->cartWaypointLine = line;
						}

						if (scene.placing->entities.size() == 1 && en.spec->monorail) {
							auto line = Monorail::Red;
							if (scene.routing && scene.routing->spec->monorail) {
								line = scene.routing->monorailLine;
								scene.routingHistory.push(scene.routing->id);
							}
							if (scene.routing) {
								delete scene.routing;
							}
							scene.routing = new GuiEntity(en.id);
							scene.routing->monorailLine = line;
						}

						if (scene.placing->entities.size() == 1) {
							scene.placing->placed(en.spec, en.pos(), en.dir());
							scene.placing->follow();
						}

					} else {
						auto en = Entity::at(te->pos());
						if (en) {
							if (en->spec == te->spec) {
								en->look(te->dir());

								if (scene.placing->config)
									postConfiguration.push({te, en});
							}

							if (scene.connecting && te->connectable(scene.connecting)) {
								if (en->spec->tube && en->tube().connect(scene.connecting->id)) {
									delete scene.connecting;
									scene.connecting = new GuiEntity(en->id);
								}
							}

							if (scene.routing && scene.routing->spec->monorail && en->spec->monorail && scene.routing->id != en->id && Entity::exists(scene.routing->id) && scene.placing->entities.size() == 1) {
								auto line = scene.routing->monorailLine;
								Entity::get(scene.routing->id).monorail().connectOut(line, en->id);
								scene.routingHistory.push(scene.routing->id);
								delete scene.routing;
								scene.routing = new GuiEntity(en->id);
								scene.routing->monorailLine = line;
							}
						}
					}
				}
				if (scene.placing->entities.size() == 1) {
					auto ge = scene.placing->entities[0];

					// bit convoluted: makes conveyor left/right switch to conveyor straight
					// rotated for the new output direction. Probably should use spec->follow
					// and another flag instead as more complex entities use conveyor components
					if (ge->spec->followConveyor && ge->spec->conveyor && !ge->spec->follow) {
						auto pe = new GuiFakeEntity(ge->spec->followConveyor);
						pe->dir(ge->spec->conveyorOutput.transform(ge->dir().rotation()).normalize());
						pe->pos(ge->pos());
						scene.placing->entities[0] = pe;
						delete ge;
					}
				}

				for (auto cfg: postConfiguration) {
					cfg.te->setConfig(*cfg.en, true);
				}
			}
		});
	};

	if (hoveringDeletable() || somethingSelectedDeletable()) {
		controlHintsSpecific["Delete"] = "Deconstruct";
		actionsEnabled.insert(Config::Action::Deconstruct);
	}

	if (hoveringForceDeletable() || somethingSelectedForceDeletable()) {
		controlHintsSpecific["Shift+Delete"] = "Deconstruct";
		actionsEnabled.insert(Config::Action::DeconstructForce);
	}

	auto actionDeconstruct = [&](bool force) {

		// Delete a group of selected entities
		if (scene.selected.size()) {
			Sim::locked([&]() {
				for (auto te: scene.selected) {
					uint id = te->id;
					auto& en = Entity::get(id);
					if (!Entity::exists(id)) continue;
					if (te->spec->forceDelete && !force) continue;
					if (scene.directing && scene.directing->id == id) continue;
					if (!te->spec->deconstructable) continue;
					if (en.isPermanent()) continue;
					en.deconstruct(true);
				}
			});
			scene.selection = {Point::Zero, Point::Zero};
			scene.selecting = false;
		}

		// Delete a single entity under the pointer
		if (scene.hovering && scene.hovering->spec->deconstructable) {
			Sim::locked([&]() {
				uint id = scene.hovering->id;
				auto te = scene.hovering;
				for (;;) {
					if (!Entity::exists(id)) break;
					auto& en = Entity::get(id);
					if (te->spec->forceDelete && !force) break;
					if (scene.directing && scene.directing->id == id) break;
					if (!te->spec->deconstructable) break;;
					if (en.isPermanent()) break;
					en.deconstruct(true);
					break;
				}
				if (scene.routing && id == scene.routing->id && scene.routing->spec->cartWaypoint) {
					auto line = scene.routing->cartWaypointLine;
					delete scene.routing;
					scene.routing = nullptr;
					// If the last cart waypoint in a route has been deleted, try to
					// revert to the previous waypoint and route color combination
					if (scene.routingHistory.size()) {
						id = scene.routingHistory.pop();
						if (id && Entity::exists(id)) {
							auto& en = Entity::get(id);
							if (en.spec->cartWaypoint) {
								scene.routing = new GuiEntity(id);
								scene.routing->cartWaypointLine = line;
								en.cartWaypoint().clrNext(line);
							}
						}
					}
				}
				if (scene.routing && id == scene.routing->id && scene.routing->spec->monorail) {
					auto line = scene.routing->monorailLine;
					delete scene.routing;
					scene.routing = nullptr;
					// If the last cart waypoint in a route has been deleted, try to
					// revert to the previous waypoint and route color combination
					if (scene.routingHistory.size()) {
						id = scene.routingHistory.pop();
						if (id && Entity::exists(id)) {
							auto& en = Entity::get(id);
							if (en.spec->monorail) {
								scene.routing = new GuiEntity(id);
								scene.routing->monorailLine = line;
								en.monorail().disconnectOut(line);
							}
						}
					}
				}
			});
		}
	};

	auto actionCut = [&]() {
		auto selection = scene.selection;
		auto selecting = scene.selecting;

		actionCopy();

		scene.selection = selection;
		scene.selecting = selecting;

		actionDeconstruct(false);
	};

	if (!scene.placing
		&& !scene.routing
		&& !scene.connecting
		&& !scene.selecting
		&& scene.directing
		&& hoveringDirectable()
		&& hoveringDirecting()
	){
		controlHintsSpecific["Ctrl+RClick"] = "Move";
	}

	if (scene.directing) {
		actionsEnabled.insert(Config::Action::Move);
	}

	auto actionMove = [&]() {
		Sim::locked([&]() {
			Point pos = scene.mouseGroundTarget();

			if (scene.directing && scene.directing->spec->vehicle && Entity::exists(scene.directing->id)) {
				Entity::get(scene.directing->id).vehicle().addWaypoint(pos);
				return;
			}
			if (scene.directing && scene.directing->spec->cart && Entity::exists(scene.directing->id)) {
				Entity::get(scene.directing->id).cart().travelTo(pos.round());
				return;
			}
			if (scene.directing && scene.directing->spec->zeppelin && Entity::exists(scene.directing->id)) {
				Entity::get(scene.directing->id).zeppelin().flyOver(pos);
				return;
			}
			if (scene.directing && scene.directing->spec->flightPath && Entity::exists(scene.directing->id)) {
				Entity::get(scene.directing->id).flightPath().depart(pos, Point::South);
				return;
			}
		});
	};

	if (scene.hovering
		&& !scene.routing
		&& !scene.connecting
	){
		actionsEnabled.insert(Config::Action::ToggleConstruct);
	}

	auto actionToggleConstruct = [&]() {
		Sim::locked([&]() {
			if (!Entity::exists(scene.hovering->id)) return;
			auto& en = Entity::get(scene.hovering->id);

			if (en.isConstruction()) {
				en.deconstruct();
				return;
			}

			if (en.isDeconstruction()) {
				// if another ghost has been placed in this spot...
				for (auto ec: Entity::intersecting(en.box().shrink(0.1))) {
					if (ec->id != en.id && ec->isConstruction()) return;
				}
				en.construct();
				return;
			}
		});
	};

	actionsEnabled.insert(Config::Action::ToggleGrid);

	auto actionToggleGrid = [&]() {
		Config::mode.grid = !Config::mode.grid;
	};

	actionsEnabled.insert(Config::Action::ToggleAlignment);

	auto actionToggleAlignment = [&]() {
		Config::mode.alignment = !Config::mode.alignment;
	};

	actionsEnabled.insert(Config::Action::ToggleCardinalSnap);

	auto actionToggleCardinalSnap = [&]() {
		Config::mode.cardinalSnap = !Config::mode.cardinalSnap;
		scene.snapNow = Config::mode.cardinalSnap;
	};

	bool toggleEnable = scene.hovering && scene.hovering->spec->enable;

	if (scene.selected.size()) {
		for (auto ge: scene.selected) toggleEnable = toggleEnable || ge->spec->enable;
	}

	if (scene.placing) {
		for (auto ge: scene.placing->entities) toggleEnable = toggleEnable || ge->spec->enable;
	}

	if (toggleEnable) {
		controlHintsSpecific["O"] = "On/Off";
		actionsEnabled.insert(Config::Action::ToggleEnable);
	}

	auto actionToggleEnable = [&]() {
		Sim::locked([&]() {
			if (scene.placing) {
				scene.placing->config = true;
				bool state = false;
				for (auto te: scene.placing->entities) {
					if (te->spec->enable) {
						state = te->isEnabled();
						break;
					}
				}
				for (auto te: scene.placing->entities) {
					te->setEnabled(!state);
				}
				return;
			}
			if (scene.selected.size()) {
				bool state = false;
				for (auto ge: scene.selected) {
					if (ge->spec->enable) {
						state = ge->isEnabled();
						break;
					}
				}
				for (auto ge: scene.selected) {
					auto et = Entity::find(ge->id);
					if (et) et->setEnabled(!state);
				}
				return;
			}
			if (scene.hovering) {
				auto et = Entity::find(scene.hovering->id);
				if (et) et->setEnabled(!et->isEnabled());
				return;
			}
		});
	};

	if (scene.placing && scene.placing->canUpward()) {
		controlHintsRelated["PageUp"] = scene.placing->canUpward()->title;
		actionsEnabled.insert(Config::Action::SpecUp);
	}

	auto actionSpecUp = [&]() {
		scene.placing->upward();
	};

	if (scene.placing && scene.placing->canDownward()) {
		controlHintsRelated["PageDown"] = scene.placing->canDownward()->title;
		actionsEnabled.insert(Config::Action::SpecDown);
	}

	auto actionSpecDown = [&]() {
		scene.placing->downward();
	};

	if (scene.placing) {
		controlHintsSpecific["Escape"] = "Discard plan";
	}
	else
	if (scene.selecting) {
		controlHintsSpecific["Escape"] = "Discard selection";
	}
	else
	if (scene.routing) {
		controlHintsSpecific["Escape"] = "Stop routing";
	}
	else
	if (scene.connecting) {
		controlHintsSpecific["Escape"] = "Stop routing";
	}
	else
	if (scene.connecting) {
		controlHintsSpecific["Escape"] = "Stop connecting";
	}

	if (scene.selecting && scene.selectingTypes != Scene::SelectJunk) {
		controlHintsSpecific["J"] = "Select trees/rocks";
		actionsEnabled.insert(Config::Action::SelectJunk);
	}

	if (scene.selecting && scene.selectingTypes == Scene::SelectJunk) {
		controlHintsSpecific["J"] = "Ignore trees/rocks";
		actionsEnabled.insert(Config::Action::SelectJunk);
	}

	auto actionSelectJunk = [&]() {
		scene.selectingTypes = (scene.selectingTypes == Scene::SelectJunk) ? Scene::SelectAll: Scene::SelectJunk;
	};

	if (scene.selecting && scene.selectingTypes != Scene::SelectUnder) {
		controlHintsSpecific["K"] = "Select piles/slabs";
		actionsEnabled.insert(Config::Action::SelectUnder);
	}

	if (scene.selecting && scene.selectingTypes == Scene::SelectUnder) {
		controlHintsSpecific["K"] = "Ignore piles/slabs";
		actionsEnabled.insert(Config::Action::SelectUnder);
	}

	auto actionSelectUnder = [&]() {
		scene.selectingTypes = (scene.selectingTypes == Scene::SelectUnder) ? Scene::SelectAll: Scene::SelectUnder;
	};

	actionsEnabled.insert(Config::Action::Plan);

	auto actionPlan = [&]() {
		doPlan = true;
	};

	actionsEnabled.insert(Config::Action::Map);

	auto actionMap = [&]() {
		doMap = true;
	};

	actionsEnabled.insert(Config::Action::Vehicles);

	auto actionVehicles = [&]() {
		doVehicles = true;
	};

	bool selectedPaint = false;

	if (scene.selected.size()) {
		for (auto& ge: scene.selected) {
			if (ge->spec->coloredCustom) selectedPaint = true;
		}
	}

	if (selectedPaint) {
		controlHintsSpecific["P"] = "Paint";
		actionsEnabled.insert(Config::Action::Paint);
	}

	auto actionPaint = [&]() {
		doPaint = true;
	};

	actionsEnabled.insert(Config::Action::Escape);

	auto actionEscape = [&]() {
		// cascade to open main menu or close popup
		doEscape = true;
		if (popup) return;

		if (scene.placing) {
			scene.planDrop();
			doEscape = false;
		}

		if (scene.connecting) {
			delete scene.connecting;
			scene.connecting = nullptr;
			doEscape = false;
		}

		if (scene.routing) {
			delete scene.routing;
			scene.routing = nullptr;
			scene.routingHistory.clear();
			doEscape = false;
		}

		if (scene.selecting) {
			scene.selection = {Point::Zero, Point::Zero};
			scene.selecting = false;
			doEscape = popup == paintPopup;
		}
	};

	actionsEnabled.insert(Config::Action::Save);

	auto actionSave = [&]() {
		doSave = true;
	};

	actionsEnabled.insert(Config::Action::Build);

	auto actionBuild = [&]() {
		doBuild = true;
	};

	actionsEnabled.insert(Config::Action::Stats);

	auto actionStats = [&]() {
		doStats = true;
	};

	actionsEnabled.insert(Config::Action::Log);

	auto actionLog = [&]() {
		doLog = true;
	};

	actionsEnabled.insert(Config::Action::Attack);

	auto actionAttack = [&]() {
		Enemy::trigger = true;
	};

	actionsEnabled.insert(Config::Action::Pause);

	auto actionPause = [&]() {
		Config::mode.pause = !Config::mode.pause;
	};

	actionsEnabled.insert(Config::Action::Debug);

	auto actionDebug = [&]() {
		doDebug = true;
	};

	actionsEnabled.insert(Config::Action::Debug2);

	auto actionDebug2 = [&]() {
//		Sim::locked([&]() {
//			if (scene.hovering && Entity::exists(scene.hovering->id)) {
//				Entity::get(scene.hovering->id).damage(1000000);
//			}
//		});
	};

	for (auto [action,combo]: Config::controls) {
		using namespace Config;

		if (!actionsEnabled.count(action)) continue;
		if (!combo.triggered()) continue;

		bool force = true;

		switch (action) {
			case Action::Copy: {
				//infof("Copy");
				if (worldFocused) actionCopy();
				break;
			}
			case Action::Cut: {
				//infof("Cut");
				if (worldFocused) actionCut();
				break;
			}
			case Action::Paste: {
				//infof("Paste");
				if (worldFocused) actionPaste();
				break;
			}
			case Action::CopyConfig: {
				//infof("CopyConfig");
				if (worldFocused) actionCopyConfig();
				break;
			}
			case Action::PasteConfig: {
				//infof("PasteConfig");
				if (worldFocused) actionPasteConfig();
				break;
			}
			case Action::Pipette: {
				//infof("Pipette");
				if (worldFocused) actionPipette();
				break;
			}
			case Action::Upgrade: {
				//infof("Upgrade");
				if (worldFocused) actionUpgrade();
				break;
			}
			case Action::UpgradeCascade: {
				//infof("UpgradeCascade");
				if (worldFocused) actionUpgradeCascade();
				break;
			}
			case Action::Rotate: {
				//infof("Rotate");
				if (worldFocused) actionRotate();
				break;
			}
			case Action::Cycle: {
				//infof("Cycle");
				if (worldFocused) actionCycle();
				break;
			}
			case Action::Open: {
				//infof("Open");
				if (worldFocused) actionOpen();
				break;
			}
			case Action::Direct: {
				//infof("Direct");
				if (worldFocused) actionDirect();
				break;
			}
			case Action::Move: {
				//infof("Move");
				if (worldFocused) actionMove();
				break;
			}
			case Action::Link: {
				//infof("Link");
				if (worldFocused) actionLink();
				break;
			}
			case Action::Connect: {
				//infof("Connect");
				if (worldFocused) actionConnect();
				break;
			}
			case Action::Disconnect: {
				//infof("Disconnect");
				if (worldFocused) actionDisconnect();
				break;
			}
			case Action::RouteRed: {
				//infof("RouteRed");
				if (worldFocused) actionRoute(CartWaypoint::Red);
				break;
			}
			case Action::RouteBlue: {
				//infof("RouteBlue");
				if (worldFocused) actionRoute(CartWaypoint::Blue);
				break;
			}
			case Action::RouteGreen: {
				//infof("RouteGreen");
				if (worldFocused) actionRoute(CartWaypoint::Green);
				break;
			}
			case Action::RouteSetNext: {
				//infof("RouteSetNext");
				if (worldFocused) actionRouteSetNext();
				break;
			}
			case Action::RouteClrNext: {
				//infof("RouteClrNext");
				if (worldFocused) actionRouteClrNext();
				break;
			}
			case Action::Flush: {
				//infof("Flush");
				if (worldFocused) actionFlush();
				break;
			}
			case Action::Construct: {
				//infof("Construct");
				if (worldFocused) actionConstruct(!force);
				break;
			}
			case Action::ConstructForce: {
				//infof("ConstructForce");
				if (worldFocused) actionConstruct(force);
				break;
			}
			case Action::Deconstruct: {
				//infof("Deconstruct");
				if (worldFocused) actionDeconstruct(!force);
				break;
			}
			case Action::DeconstructForce: {
				//infof("DeconstructForce");
				if (worldFocused) actionDeconstruct(force);
				break;
			}
			case Action::ToggleConstruct: {
				//infof("ToggleConstruct");
				if (worldFocused) actionToggleConstruct();
				break;
			}
			case Action::ToggleGrid: {
				//infof("ToggleGrid");
				if (worldFocused) actionToggleGrid();
				break;
			}
			case Action::ToggleAlignment: {
				//infof("ToggleAlignment");
				if (worldFocused) actionToggleAlignment();
				break;
			}
			case Action::ToggleEnable: {
				//infof("ToggleEnable");
				if (worldFocused) actionToggleEnable();
				break;
			}
			case Action::ToggleCardinalSnap: {
				//infof("ToggleCardinalSnap");
				if (worldFocused) actionToggleCardinalSnap();
				break;
			}
			case Action::SpecUp: {
				//infof("SpecUp");
				if (worldFocused) actionSpecUp();
				break;
			}
			case Action::SpecDown: {
				//infof("SpecDown");
				if (worldFocused) actionSpecDown();
				break;
			}
			case Action::SelectJunk: {
				//infof("SelectJunk");
				actionSelectJunk();
				break;
			}
			case Action::SelectUnder: {
				//infof("SelectUnder");
				actionSelectUnder();
				break;
			}
			case Action::Plan: {
				//infof("Plan");
				if (worldFocused || popup == planPopup)
					actionPlan();
				break;
			}
			case Action::Map: {
				//infof("Map");
				if (worldFocused || popup == mapPopup)
					actionMap();
				break;
			}
			case Action::Paint: {
				//infof("Paint");
				if (worldFocused || popup == planPopup)
					actionPaint();
				break;
			}
			case Action::Vehicles: {
				//infof("Vehicles");
				if (worldFocused || popup == vehiclePopup)
					actionVehicles();
				break;
			}
			case Action::Escape: {
				//infof("Escape");
				actionEscape();
				break;
			}
			case Action::Save: {
				//infof("Save");
				actionSave();
				break;
			}
			case Action::Build: {
				//infof("Build");
				// need better key combo integration with popups
				if (worldFocused || popup == recipePopup)
					actionBuild();
				break;
			}
			case Action::Stats: {
				//infof("Stats");
				actionStats();
				break;
			}
			case Action::Log: {
				//infof("Log");
				actionLog();
				break;
			}
			case Action::Attack: {
				//infof("Attack");
				actionAttack();
				break;
			}
			case Action::Pause: {
				//infof("Pause");
				actionPause();
				break;
			}
			case Action::Debug: {
				//infof("Debug");
				actionDebug();
				break;
			}
			case Action::Debug2: {
				//infof("Debug2");
				actionDebug2();
				break;
			}
		}
	}

	if (doEscape) {
		doEscape = false;

		// imgui child "popup" is visible over our popup
		if (popup && popup->subpopup) {
			return;
		}

		if (popup) {
			popup->show(false);
			popup = nullptr;
			return;
		}

		popup = mainMenu;
		popup->show(true);
	}

	if (doSave || (Config::mode.fresh && Sim::tick == 60)) {
		doSave = false;
		Sim::locked([&]() {
			bool ok = Sim::save(Config::savePath(Config::mode.saveName).c_str(), scene.position, scene.direction, scene.directing ? scene.directing->id: 0);
			if (ok) scene.print(fmt("Game \"%s\" saving in the background. Carry on...", Config::mode.saveName));
			if (!ok) scene.print(fmt("Background save already in progress."));
		});
	}

	if (doMenu) {
		doMenu = false;
		togglePopup(mainMenu);
	}

	if (doDebug) {
		doDebug = false;
		togglePopup(debugMenu);
	}

	if (doLog) {
		doLog = false;
		togglePopup(loading);
	}

	if (doStats) {
		doStats = false;
		togglePopup(statsPopup);
	}

	if (doBuild) {
		doBuild = false;
		togglePopup(recipePopup);
	}

	if (doUpgrade) {
		doUpgrade = false;
		togglePopup(upgradePopup);
	}

	if (doPlan) {
		doPlan = false;
		togglePopup(planPopup);
	}

	if (doMap) {
		doMap = false;
		togglePopup(mapPopup);
	}

	if (doVehicles) {
		doVehicles = false;
		togglePopup(vehiclePopup);
	}

	if (doPaint) {
		doPaint = false;
		togglePopup(paintPopup);
	}

	if (doSignals) {
		doSignals = false;
		togglePopup(signalsPopup);
	}

	if (popup && !popup->visible) {
		popup = nullptr;
	}
}
