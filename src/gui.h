#pragma once
#include "common.h"
#include "scene.h"
#include "popup.h"
#include "toolbar.h"
#include "hud.h"
#include "config.h"
#include "log.h"

struct GUI {
	Popup* popup = nullptr;
	bool doMenu = false;
	bool doDebug = false;
	bool doSave = false;
	bool doLog = false;
	bool doStats = false;
	bool doBuild = false;
	bool doUpgrade = false;
	bool doSignals = false;
	bool doPlan = false;
	bool doMap = false;
	bool doVehicles = false;
	bool doPaint = false;
	bool doEscape = false;
	bool doQuit = false;
	bool focused = false;
	bool prepared = false;

	StatsPopup2* statsPopup = nullptr;
	EntityPopup2* entityPopup = nullptr;
	RecipePopup* recipePopup = nullptr;
	UpgradePopup* upgradePopup = nullptr;
	SignalsPopup* signalsPopup = nullptr;
	PlanPopup* planPopup = nullptr;
	MapPopup* mapPopup = nullptr;
	VehiclePopup* vehiclePopup = nullptr;
	PaintPopup* paintPopup = nullptr;
	MainMenu* mainMenu = nullptr;
	DebugMenu* debugMenu = nullptr;
	LoadingPopup *loading = nullptr;

	HUD* hud = nullptr;
	Toolbar* toolbar = nullptr;

	float ups = 0.0f;
	float fps = 0.0f;

	std::map<std::string,std::string> controlHintsRelated;
	std::map<std::string,std::string> controlHintsGeneral;
	std::map<std::string,std::string> controlHintsSpecific;

	struct {
		uint planId = 0;
		Plan* plan = nullptr;
		Point pos = Point::Zero;
		uint64_t tick = 0;
	} lastConstruct;

	GUI() = default;
	void init();
	void reset();
	void prepare();
	bool active();
	void render();
	void update();
	void log(std::string msg);
	void togglePopup(Popup*);
};

extern GUI gui;