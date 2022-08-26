#pragma once

// Balancer components link up with Conveyors to balance items.

struct Balancer;
struct BalancerSettings;
struct BalancerGroup;

#include "entity.h"
#include "conveyor.h"
#include "slabmap.h"
#include "hashset.h"
#include <queue>

struct Balancer {
	uint id;
	Entity* en;
	BalancerGroup* group;
	miniset<uint> filter;
	bool managed;

	struct {
		bool input;
		bool output;
	} priority;

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Balancer,&Balancer::id> all;
	static Balancer& create(uint id);
	static Balancer& get(uint id);

	static inline hashset<uint> changed;

	void destroy();

	BalancerSettings* settings();
	void setup(BalancerSettings*);

	void update();
	Point left();
	Point right();
	Balancer& manage();
	Balancer& unmanage();
};

struct BalancerGroup {
	static inline hashset<BalancerGroup*> all;
	static inline hashset<BalancerGroup*> changed;
	struct Member {
		Balancer* balancer = nullptr;
		Conveyor* conveyor = nullptr;
		uint64_t inLeft = 0;
		uint64_t outLeft = 0;
		uint64_t inRight = 0;
		uint64_t outRight = 0;
	};
	minivec<Member> members;
};

struct BalancerSettings {
	miniset<uint> filter;
	struct {
		bool input = false;
		bool output = false;
	} priority;
	BalancerSettings() = default;
	BalancerSettings(Balancer& balancer);
};
