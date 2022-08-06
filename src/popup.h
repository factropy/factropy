#pragma once

// ImGUI popup windows.

#include "scene.h"
#include "recipe.h"
#include "item.h"
#include "fluid.h"
#include "time-series.h"
#include "goal.h"

struct Popup {
	float w = 100.0;
	float h = 100.0;
	bool visible = false;
	bool opened = false;
	bool mouseOver = false;
	bool inputFocused = false;
	Popup();
	virtual ~Popup();
	virtual void draw();
	virtual void prepare();
	void show(bool state = true);
	void big();
	void small();
	void medium();
	void narrow();
	void center();
	void topRight();
	void bottomLeft();
	float relativeWidth(float w);
	static void tip(const std::string& s);
	static std::string wrap(uint line, std::string text);

	static void crafterNotice(Crafter& crafter);
	static void launcherNotice(Launcher& launcher);
	static void powerpoleNotice(PowerPole& pole);
	static void goalRateChart(Goal* goal, Goal::Rate& rate, float h);
};

struct SignalsPopup : Popup {
	char create[50];
	std::map<uint,char[50]> inputs;
	SignalsPopup();
	~SignalsPopup();
	void draw() override;
};

struct StatsPopup2 : Popup {
	char filter[50];
	bool any = true;
	std::vector<Item*> itemsAlpha;
	StatsPopup2();
	~StatsPopup2();
	void draw() override;
	void prepare() override;
};

struct EntityPopup2 : Popup {
	uint eid = 0;
	char name[50];
	char interfaces[3][50];
	minivec<bool> checks;
	float color[3];
	EntityPopup2();
	~EntityPopup2();
	void draw() override;
	void useEntity(uint eid);
};

struct LoadingPopup : Popup {
	Scene::Texture banner;
	std::vector<std::string> log;
	float progress = 0.0f;
	LoadingPopup();
	~LoadingPopup();
	void draw() override;
	void print(std::string msg);
};

struct RecipePopup : Popup {
	bool showUnavailable = false;

	struct {
		Item* item = nullptr;
		Fluid* fluid = nullptr;
		Recipe* recipe = nullptr;
		Spec* spec = nullptr;
		Goal* goal = nullptr;
	} locate;

	struct {
		std::map<Item*,bool> item;
		std::map<Fluid*,bool> fluid;
		std::map<Recipe*,bool> recipe;
		std::map<Spec*,bool> spec;
		std::map<Goal*,bool> goal;
	} highlited;

	struct {
		std::map<Item*,bool> item;
		std::map<Fluid*,bool> fluid;
		std::map<Recipe*,bool> recipe;
		std::map<Spec*,bool> spec;
		std::map<Goal*,bool> goal;
	} expanded;

	struct {
		std::vector<Item*> items;
		std::vector<Fluid*> fluids;
		std::vector<Recipe*> recipes;
		std::vector<Spec*> specs;
		std::vector<Goal*> goals;
	} sorted;

	RecipePopup();
	~RecipePopup();
	void draw() override;
	void prepare() override;
	void drawItem(Item* item);
	void drawItemButton(Item* item);
	void drawItemButtonNoBullet(Item* item);
	void drawItemButton(Item* item, int count);
	void drawItemButton(Item* item, int count, int limit);
	void drawFluid(Fluid* fluid);
	void drawFluidButton(Fluid* fluid);
	void drawFluidButton(Fluid* fluid, int count);
	void drawRecipe(Recipe* recipe);
	void drawRecipeButton(Recipe* recipe);
	void drawSpec(Spec* spec);
	void drawSpecButton(Spec* spec);
	void drawGoal(Goal* goal);
	void drawGoalButton(Goal* goal);
};

struct UpgradePopup : Popup {
	UpgradePopup();
	~UpgradePopup();
	void draw() override;
};

struct PlanPopup : Popup {
	PlanPopup();
	~PlanPopup();
	void draw() override;
};

struct ZeppelinPopup : Popup {
	ZeppelinPopup();
	~ZeppelinPopup();
	void draw() override;
};

struct PaintPopup : Popup {
	struct RGB {
		float r = 1.0;
		float g = 1.0;
		float b = 1.0;
	};
	RGB rgb;
	miniset<Color> colors;
	PaintPopup();
	~PaintPopup();
	void draw() override;
};

struct MapPopup : Popup {
	enum {
		TileLand = 0,
		TileHill,
		TileLake,
	};

	struct Tile {
		int8_t terrain = 0;
	};

	struct CTile {
		int16_t x = 0;
		int16_t y = 0;
		int16_t w = 0;
		int8_t terrain = 0;
	};

	struct Structure {
		int16_t x0 = 0;
		int16_t y0 = 0;
		int16_t x1 = 0;
		int16_t y1 = 0;
		Color c = 0xffffffff;
	};

	struct MTube {
		int16_t x0 = 0;
		int16_t y0 = 0;
		int16_t x1 = 0;
		int16_t y1 = 0;
	};

	struct MRail {
		int16_t x0 = 0;
		int16_t y0 = 0;
		int16_t x1 = 0;
		int16_t y1 = 0;
	};

	std::vector<CTile> ctiles;
	std::vector<MTube> tubes;
	std::vector<MRail> rails;

	std::vector<Structure> slabs;
	std::vector<Structure> buildings;
	std::vector<Structure> generators;
	std::vector<Structure> others;
	std::vector<Structure> damaged;
	std::vector<Structure> belts;
	std::vector<Structure> pipes;

	Point target = Point::Zero;
	Point retarget = Point::Zero;
	int scale = 4;

	MapPopup();
	~MapPopup();
	void draw() override;
	void prepare() override;
};

struct MainMenu : Popup {
	Scene::Texture capsule;

	struct {
		int period = 0;
		int ticker = 0;
	} quitting;

	char saveas[50];
	char create[50];
	std::string load;

	minivec<bool> games;

	enum class SaveStatus {
		Current = 0,
		SaveAsOk,
		SaveAsFail,
	};

	SaveStatus saveStatus;

	MainMenu();
	~MainMenu();
	void draw() override;
};

struct DebugMenu : Popup {
	DebugMenu();
	~DebugMenu();
	void draw() override;
};


