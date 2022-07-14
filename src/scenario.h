#pragma once

struct Scenario;

#include "common.h"
#include "item.h"
#include "fluid.h"
#include "spec.h"
#include "entity.h"
#include "world.h"
#include "sim.h"
#include "scene.h"
#include "goal.h"
#include "goal.h"
#include "message.h"

class RelaMod;

struct Scenario {
	std::map<std::string,Mesh*> meshes;
	std::map<Spec*,std::string> specUpgrades;
	std::map<Spec*,std::vector<std::string>> specUpgradeCascades;
	std::map<Spec*,std::string> specCycles;
	std::map<Spec*,std::string> specPipettes;
	std::map<Spec*,std::string> specStatsGroups;
	std::map<Spec*,std::string> specSourceItems;
	std::map<Spec*,std::string> specSourceFluids;
	std::map<Spec*,std::string> specUnveyorPartners;
	std::map<Spec*,std::string> specFollows;
	std::map<Spec*,std::string> specFollowConveyors;
	std::map<Spec*,std::string> specUpwards;
	std::map<Spec*,std::string> specDownwards;
	std::deque<Mat4> matrices;
	virtual void items() = 0;
	virtual void fluids() = 0;
	virtual void recipes() = 0;
	virtual void specifications() = 0;
	virtual void goals() = 0;
	virtual void messages() = 0;
	virtual void create() = 0;
	virtual void load() = 0;
	virtual bool attack() = 0;
	virtual float attackFactor() = 0;
	virtual ~Scenario() {};
};

extern Scenario* scenario;

struct ScenarioBase : Scenario {
	RelaMod* rela = nullptr;
	std::vector<Spec*> rocks;
	ScenarioBase();
	void items();
	void fluids();
	void recipes();
	void specifications();
	void goals();
	void messages();
	void generate();
	void create();
	void init();
	void load();
	bool attack();
	float attackFactor();
	void tick();
	Mesh* mesh(const char* name);
	void conveyorTier(int tier, uint color, uint chevronColor);
	void cartTier(int tier);
	void truckTier(int tier);
	void blimpTier(int tier);
	void droneDepotTier(int tier);
};

