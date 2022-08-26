#include "common.h"
#include "config.h"
#include "popup.h"
#include "enemy.h"
#include "gui.h"

#include "../imgui/setup.h"

using namespace ImGui;

MainMenu::MainMenu() : Popup() {
	capsule = loadTexture("assets/capsule.png");
}

MainMenu::~MainMenu() {
}

void MainMenu::draw() {
	bool showing = true;

	auto saveConfig = [&](bool changed = true) {
		if (changed) Config::save();
		return changed;
	};

	narrow();
	Begin(fmtc("%s : %ld##mainmenu", Config::mode.saveName, Sim::seed), &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		auto focusedTab = [&]() {
			bool focused = opened;
			opened = false;
			return (focused ? ImGuiTabItemFlags_SetSelected: ImGuiTabItemFlags_None) | ImGuiTabItemFlags_NoPushId;
		};

		if (BeginTabBar("main-tabs", ImGuiTabBarFlags_None)) {
			if (BeginTabItem("Game", nullptr, focusedTab())) {

				SpacingV();
				ImageBanner(capsule.id, capsule.w, capsule.h);
				SpacingV();

				Notice(Config::version.text);

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				if (BeginTable("##menu1-game", 2)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.5);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.5);

					TableNextColumn();

						if (Button("Save [F5]", ImVec2(-1,0))) {
							gui.doSave = true;
						}

					TableNextColumn();

						if (IsWindowAppearing()) {
							quitting.period = 0;
							quitting.ticker = 0;
						}

						if (quitting.period > 0 && quitting.ticker < quitting.period) {
							quitting.ticker++;
						}

						if (quitting.period > 0 && quitting.ticker >= quitting.period) {
							gui.doQuit = true;
						}

						if (quitting.period == 0 && Button("Quit", ImVec2(-1,0))) {
							quitting.period = gui.fps*2;
							quitting.ticker = 0;
						}

						if (quitting.period > 0 && Button("Continue", ImVec2(-1,0))) {
							quitting.period = 0;
							quitting.ticker = 0;
						}

					EndTable();
				}

				PopStyleVar();

				if (quitting.period > 0) {
					SmallBar((float)quitting.ticker/(float)quitting.period);
				}

				SpacingV();

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				if (BeginTable("##menu2-game", 2)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.5);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.5);

					TableNextColumn();

						if (Button("Signals", ImVec2(-1,0))) {
							gui.doSignals = true;
						}
						if (IsItemHovered()) tip(
							"Custom signals for use in rules and on wifi."
						);
						SpacingV();

					TableNextColumn();

						if (Button(Config::mode.pause ? "Resume": "Pause", ImVec2(-1,0))) {
							Config::mode.pause = !Config::mode.pause;
						}
						if (IsItemHovered()) tip("[Pause]");
						SpacingV();

					TableNextColumn();

						if (Button("Zeppelins [Z]", ImVec2(-1,0))) {
							gui.doZeppelins = true;
						}

					TableNextColumn();

						if (Button("Map [M]", ImVec2(-1,0))) {
							gui.doMap = true;
						}

					EndTable();
				}
				PopStyleVar();

				SpacingV();

				Section("Settings");

				PushItemWidth(GetContentRegionAvail().x*0.5);
				saveConfig(InputIntClamp("Autosave##autosave", &Config::mode.autosave, 5, 60, 5, 5));
				if (IsItemHovered()) tip(
					"Autosave interval in minutes."
				);
				saveConfig(InputIntClamp("UPS", &Config::engine.ups, 1, 1000, 1, 1));
				if (IsItemHovered()) tip(
					"Simulation updates per second. Increase to make time speed up."
				);
				PopItemWidth();

				saveConfig(Checkbox("Lock UPS to FPS", &Config::engine.pulse));
				if (IsItemHovered()) tip(
					"UPS and FPS are loosely coupled by default, allowing FPS to"
					" fluctuate without impacting the apparent speed of the game."
				);

				saveConfig(Checkbox("Build Grid [G]", &Config::mode.grid));
				if (IsItemHovered()) tip(
					"Show the build grid when zoomed in."
				);

				saveConfig(Checkbox("Build Alignment [H]", &Config::mode.alignment));
				if (IsItemHovered()) tip(
					"Show alignment lines when placing a ghost or blueprint."
				);

				saveConfig(Checkbox("Camera Snap [TAB]", &Config::mode.cardinalSnap));
				if (IsItemHovered()) tip(
					"Align the camera when close to the cardinal directions: North, South, East, West."
				);

				saveConfig(Checkbox("Tip Window", &Config::mode.autotip));
				if (IsItemHovered()) tip(
					"Show the tip window after 1s of mouse inactivity. [SPACEBAR] will always manually show it."
				);

				saveConfig(Checkbox("Enemy Attacks", &Enemy::enable));
				if (IsItemHovered()) tip(
					"The enemy, livid that your logistics skills are so awesome, will"
					" periodically send missiles to disrupt your supply chain."
				);

				EndTabItem();
			}

			if (BeginTabItem("Graphics", nullptr, focusedTab())) {

				SpacingV();
				Section("Quality");
				if (BeginTable("##renderer-quick-buttons", 3)) {
					TableNextRow();
					TableNextColumn();
					if (Button("High", ImVec2(-1,0))) {
						Config::window.horizon = 1500;
						Config::window.fog = 1000;
						Config::window.zoomUpperLimit = 500;
						Config::window.levelsOfDetail[0] = 150;
						Config::window.levelsOfDetail[1] = 600;
						Config::window.levelsOfDetail[2] = 1000;
						Config::window.levelsOfDetail[3] = 1500;
						Config::mode.shadowmap = true;
						Config::mode.itemShadows = true;
						Config::mode.meshMerging = true;
						Config::mode.waterwaves = true;
						Config::mode.treebreeze = true;
						Config::mode.filters = true;
						saveConfig();
					}

					TableNextColumn();
					if (Button("Medium", ImVec2(-1,0))) {
						Config::window.horizon = 1200;
						Config::window.fog = 800;
						Config::window.zoomUpperLimit = 400;
						Config::window.levelsOfDetail[0] = 100;
						Config::window.levelsOfDetail[1] = 500;
						Config::window.levelsOfDetail[2] = 800;
						Config::window.levelsOfDetail[3] = 1200;
						Config::mode.shadowmap = true;
						Config::mode.itemShadows = true;
						Config::mode.meshMerging = true;
						Config::mode.waterwaves = true;
						Config::mode.treebreeze = true;
						Config::mode.filters = true;
						saveConfig();
					}

					TableNextColumn();
					if (Button("Low", ImVec2(-1,0))) {
						Config::window.horizon = 900;
						Config::window.fog = 500;
						Config::window.zoomUpperLimit = 300;
						Config::window.levelsOfDetail[0] = 50;
						Config::window.levelsOfDetail[1] = 300;
						Config::window.levelsOfDetail[2] = 500;
						Config::window.levelsOfDetail[3] = 800;
						Config::mode.shadowmap = true;
						Config::mode.itemShadows = false;
						Config::mode.meshMerging = true;
						Config::mode.waterwaves = false;
						Config::mode.treebreeze = false;
						Config::mode.filters = false;
						saveConfig();
					}

					EndTable();
				}
				SpacingV();

				Section("Custom");
				PushItemWidth(GetContentRegionAvail().x*0.5);

				saveConfig(InputIntClamp("FPS", &Config::window.fps, 10, 1000, 10, 10));
				if (IsItemHovered()) tip(
					"Frames per second. Only works if the game was started with VSYNC off."
					" FPS and UPS are only loosely coupled however there is a stage on every frame"
					" where the renderer must lock the simulation to extract data for the entities"
					" in view. Therefore setting FPS too high will eventually impact UPS."
				);

				saveConfig(InputIntClamp("FOV", &Config::window.fov, 45, 135, 5, 5));
				if (IsItemHovered()) tip(
					"Horizontal field of view."
				);

				saveConfig(InputIntClamp("Horizon (m)", &Config::window.horizon, 750, 1500, 10, 100));
				if (IsItemHovered()) tip(
					"Distance from ground zero (the point on the ground the camera is pointing at)"
					" to render entities. Terrain is always rendered at least this far too."
				);

				saveConfig(InputIntClamp("Fog (m)", &Config::window.fog, 100, 1000, 10, 100));
				if (IsItemHovered()) tip(
					"Distance from ground zero (the point on the ground the camera is pointing at)"
					" where fog blur begins."
				);

				saveConfig(InputIntClamp("Zoom (m)", &Config::window.zoomUpperLimit, 100, 1000, 10, 100));
				if (IsItemHovered()) tip(
					"Maximum camera distance from ground zero (the point on the ground the camera is pointing at)."
				);

				saveConfig(InputIntClamp("LOD close (m)", &Config::window.levelsOfDetail[0], 10, 1500, 10, 100));
				if (IsItemHovered()) tip(
					"Distance from the camera to render high-detail entity and item models."
				);

				saveConfig(InputIntClamp("LOD near (m)", &Config::window.levelsOfDetail[1], Config::window.levelsOfDetail[0], 1500, 10, 100));
				if (IsItemHovered()) tip(
					"Distance from the camera to render medium-detail entity and low-detail item models."
				);

				saveConfig(InputIntClamp("LOD far (m)", &Config::window.levelsOfDetail[2], Config::window.levelsOfDetail[1], 1500, 10, 100));
				if (IsItemHovered()) tip(
					"Distance from the camera to render low-detail entity models (items will be invisible)."
				);

				saveConfig(InputIntClamp("LOD distant (m)", &Config::window.levelsOfDetail[3], Config::window.levelsOfDetail[2], 1500, 10, 100));
				if (IsItemHovered()) tip(
					"Distance from the camera to render very low-detail entity models (items and small"
					" entities will be invisible)."
				);

				if (BeginCombo("##ground", Config::window.ground == Config::window.groundSand ? "sand": "grass")) {
					if (Selectable("sand", Config::window.ground == Config::window.groundSand)) {
						Config::window.ground = Config::window.groundSand;
						Config::window.grid = Config::window.gridSand;
						saveConfig();
					}
					if (Selectable("grass", Config::window.ground == Config::window.groundGrass)) {
						Config::window.ground = Config::window.groundGrass;
						Config::window.grid = Config::window.gridGrass;
						saveConfig();
					}
					EndCombo();
				}
				PopItemWidth();

				saveConfig(Checkbox("Shadows", &Config::mode.shadowmap));
				if (IsItemHovered()) tip(
					"Render shadows. Increases GPU load."
				);

				if (Config::mode.shadowmap) {
					saveConfig(Checkbox("Shadows: Items", &Config::mode.itemShadows));
					if (IsItemHovered()) tip(
						"Render item shadows. Increases GPU load."
					);
				}

				saveConfig(Checkbox("Mesh Merging", &Config::mode.meshMerging));
				if (IsItemHovered()) tip(
					"Generate merged meshes for groups of identical items on backed-up belts."
					" Reduces CPU load. Slightly increases GPU load."
				);

				saveConfig(Checkbox("Procedural Waves", &Config::mode.waterwaves));
				if (IsItemHovered()) tip(
					"Water motion. Slightly increases GPU load."
				);

				saveConfig(Checkbox("Procedural Breeze", &Config::mode.treebreeze));
				if (IsItemHovered()) tip(
					"Subtle tree motion. Slightly increases GPU load."
				);

				saveConfig(Checkbox("Procedural Textures", &Config::mode.filters));
				if (IsItemHovered()) tip(
					"Close-up surface effects on concrete and metal. Slightly increases GPU load."
				);

				saveConfig(Checkbox("MSAA", &Config::window.antialias));
				if (IsItemHovered()) tip(
					"Normal GL_MULTISAMPLE. Disable if better anti-aliasing is done via GPU driver settings."
				);

				if (Checkbox("VSYNC", &Config::window.vsync)) {
					SDL_GL_SetSwapInterval(Config::window.vsync ? 1:0);
					saveConfig();
				}

				saveConfig(Checkbox("Show UPS", &Config::mode.overlayUPS));
				saveConfig(Checkbox("Show FPS", &Config::mode.overlayFPS));

				EndTabItem();
			}

			if (BeginTabItem("Controls", nullptr, focusedTab())) {
				SpacingV();
				Warning("Not yet configurable.");

				int id = 0;
				float width = GetContentRegionAvail().x/2.0;

				auto display = [&](const char* name, Config::KeyMouseCombo combo, const char* text) {
					PushFont(Config::sidebar.font.imgui);
					if (BeginTable(fmtc("#controls-%d", ++id), 2, ImGuiTableFlags_BordersInnerH)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);
						TableNextRow();
						TableNextColumn();
						Print(name);
						std::string keys;
						for (const auto& mod: combo.mods) {
							if (keys.size()) keys += " + ";
							keys += std::string(SDL_GetKeyName(mod));
						}
						for (const auto& button: combo.buttonsDown) {
							if (keys.size()) keys += " + ";
							switch (button) {
								case SDL_BUTTON_LEFT:
									keys += "Left Mouse";
									break;
								case SDL_BUTTON_MIDDLE:
									keys += "Middle Mouse";
									break;
								case SDL_BUTTON_RIGHT:
									keys += "Right Mouse";
									break;
							}
						}
						for (const auto& button: combo.buttonsReleased) {
							if (keys.size()) keys += " + ";
							switch (button) {
								case SDL_BUTTON_LEFT:
									keys += "Left Mouse";
									break;
								case SDL_BUTTON_MIDDLE:
									keys += "Middle Mouse";
									break;
								case SDL_BUTTON_RIGHT:
									keys += "Right Mouse";
									break;
							}
						}
						for (const auto& mod: combo.keysReleased) {
							if (keys.size()) keys += " + ";
							keys += std::string(SDL_GetKeyName(mod));
						}
						TableNextColumn();
						Print(keys);
						EndTable();
					}
					if (IsItemHovered()) tip(text);
					PopFont();
				};

				Section("Game Interface");

				PushFont(Config::sidebar.font.imgui);
				if (BeginTable(fmtc("#controls-0", ++id), 2, ImGuiTableFlags_BordersInnerH)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);

					TableNextRow();
					TableNextColumn();
					Print("Information");
					TableNextColumn();
					Print("Hover + Space Bar");
					EndTable();
				}
				if (IsItemHovered()) tip(
					"Hover the mouse over almost anything in the game"
					" -- hills, lakes, structures, vehicles --"
					" and hold [SPACEBAR] to see useful information."
				);
				PopFont();

				display("Escape", Config::controls[Config::Action::Escape],
					"Stop the current action. Close the current window."
				);
				display("Save", Config::controls[Config::Action::Save],
					"Background save the current game."
				);
				display("Build", Config::controls[Config::Action::Build],
					"Open the build window."
				);
				display("Pause", Config::controls[Config::Action::Pause],
					"Pause the game."
				);
				display("Open", Config::controls[Config::Action::Open],
					"Open an entity window."
				);

				SpacingV();
				Section("Ghosts and Blueprints");

				display("Cycle", Config::controls[Config::Action::Cycle],
					"Cycle through ghost variations: straight belt or corner belts. Underground entry or exit. Pipe configurations."
				);
				display("SpecUp", Config::controls[Config::Action::SpecUp],
					"Switch to a related ghost variation. Belt to balancer. Balancer to loader."
				);
				display("SpecDown", Config::controls[Config::Action::SpecDown],
					"Switch to a related ghost variation. Belt to Underground belt. Pipe to underground pipe."
				);
				display("Construct", Config::controls[Config::Action::Construct],
					"Place a ghost or blueprint for construction."
				);
				display("ConstructForce", Config::controls[Config::Action::ConstructForce],
					"Place a ghost or blueprint for construction ignoring any conflicts."
				);
				display("Copy", Config::controls[Config::Action::Copy],
					"Create a blueprint from selected entities."
				);
				display("Cut", Config::controls[Config::Action::Cut],
					"Create a blueprint from selected entities, and schedule them for deconstruction."
				);
				display("Paste", Config::controls[Config::Action::Paste],
					"Load the last blueprint."
				);
				display("CopyConfig", Config::controls[Config::Action::CopyConfig],
					"Copy settings from hovered entity."
				);
				display("PasteConfig", Config::controls[Config::Action::PasteConfig],
					"Apply the most recent copied settings to hovered or selected entities."
				);
				display("Pipette", Config::controls[Config::Action::Pipette],
					"Copy an entity as a ghost."
				);
				display("Rotate", Config::controls[Config::Action::Rotate],
					"Rotate a ghost or entity (not supported by all entity types)."
				);
				display("Deconstruct", Config::controls[Config::Action::Deconstruct],
					"Schedule the selected entities for deconstruction."
				);
				display("DeconstructForce", Config::controls[Config::Action::DeconstructForce],
					"Schedule the selected entities for deconstruction."
				);
				display("ToggleConstruct", Config::controls[Config::Action::ToggleConstruct],
					"Switch a ghost between construction and deconstruction."
				);
				display("Upgrade", Config::controls[Config::Action::Upgrade],
					"Schedule an entity for upgrade. If no upgrade path exists or has been unlocked, nothing will happen."
				);
				display("UpgradeCascade", Config::controls[Config::Action::UpgradeCascade],
					"Upgrade a set of connected belts or tubes."
				);
				display("SelectJunk", Config::controls[Config::Action::SelectJunk],
					"Toggle selection between normal entities and trees/rocks."
				);
				display("SelectUnder", Config::controls[Config::Action::SelectUnder],
					"Toggle selection between normal entities and piles/slabs."
				);

				SpacingV();
				Section("Vehicle Control");

				display("Direct", Config::controls[Config::Action::Direct],
					"Set the currently controlled entity: Zeppelin, Blimp, Cart, Truck."
				);
				display("Move", Config::controls[Config::Action::Move],
					"Move the currently controlled entity: Zeppelin, Blimp, Cart, Truck."
				);

				SpacingV();
				Section("Links and Routes");

				display("Link", Config::controls[Config::Action::Link],
					"Create a link from an entity to another: waypoint to waypoint, tube to tube."
				);
				display("Connect", Config::controls[Config::Action::Connect],
					"Connect the last linked entity to another."
				);
				display("Disconnect", Config::controls[Config::Action::Disconnect],
					"Disconnect the last linked entity from another."
				);
				display("RouteRed", Config::controls[Config::Action::RouteRed],
					"Set the linked waypoint to create a red route."
				);
				display("RouteBlue", Config::controls[Config::Action::RouteBlue],
					"Set the linked waypoint to create a blue route."
				);
				display("RouteGreen", Config::controls[Config::Action::RouteGreen],
					"Set the linked waypoint to create a green route."
				);
//				display("RouteSetNext", Config::controls[Config::Action::RouteSetNext],
//					"Make a coloured route between waypoints."
//				);
//				display("RouteClrNext", Config::controls[Config::Action::RouteClrNext],
//					"Remove a coloured route between waypoints."
//				);

				SpacingV();
				Section("QoL");

				display("Flush", Config::controls[Config::Action::Flush],
					"Flush a pipe of fluid. Clear a belt or tube of items."
				);
				display("ToggleGrid", Config::controls[Config::Action::ToggleGrid],
					"Toggle the build grid."
				);
				display("ToggleAlignment", Config::controls[Config::Action::ToggleAlignment],
					"Toggle blueprint alignment lines."
				);
				display("ToggleEnable", Config::controls[Config::Action::ToggleEnable],
					"Toggle an entity on/off (enable, disable)."
				);
				display("ToggleCardinalSnap", Config::controls[Config::Action::ToggleCardinalSnap],
					"Toggle camera snapping near the cardinal directions: North, South, East, West."
				);

				SpacingV();
				Section("Other");

				display("Stats", Config::controls[Config::Action::Stats],
					"Open the stats window."
				);
				display("Map", Config::controls[Config::Action::Map],
					"Open the map window."
				);
				display("Paint", Config::controls[Config::Action::Paint],
					"Open the paint window."
				);
				display("Log", Config::controls[Config::Action::Log],
					"Open the log window."
				);
				display("Zeppelins", Config::controls[Config::Action::Zeppelins],
					"Open the Zeppelins window."
				);

				EndTabItem();
			}
			EndTabBar();
		}
	End();
	if (visible) show(showing);
}

