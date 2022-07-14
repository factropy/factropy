#pragma once

// Monorail components are towers connected by Curves and coloured routes,
// navigated by Monorail cars

struct Monorail;
struct MonorailSettings;

#include "entity.h"
#include "slabmap.h"
#include "gridmap.h"

struct Monorail {
	uint id = 0;
	Entity* en = nullptr;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Monorail,&Monorail::id> all;
	static Monorail& create(uint id);
	static Monorail& get(uint id);

	void destroy();
	void update();

	static const int Red = 0;
	static const int Blue = 1;
	static const int Green = 2;
	uint out[3];
	miniset<uint> in;

	Rail railTo(Monorail* other);

	uint claimer = 0;
	uint64_t claimed = 0;
	bool claim(uint cid);
	bool occupied();
	bool canUnload();
	bool canLoad();
	void unload(uint cid);
	uint load();

	bool filling = true;
	bool emptying = true;

	struct {
		bool contents = false;
	} transmit;

	Point positionUnload();
	Point positionEmpty();
	Point positionFill();
	Point positionLoad();

	Entity* containerAt(Point pos);

	struct Container {
		enum State {
			Unloaded = 0,
			Descending,
			Empty,
			Shunting,
			Fill,
			Ascending,
			Loaded,
		};
		State state = Unloaded;
		uint cid = 0;
		Entity* en = nullptr;
		Store* store = nullptr;
	};

	minivec<Container> containers;

	bool containerAdd(Container::State s, uint cid);

	MonorailSettings* settings();
	void setup(MonorailSettings*);

	Point arrive();
	Point depart();
	Point origin();

	struct RailLine {
		Rail rail;
		int line;
	};

	std::vector<RailLine> railsOut();

	bool connectOut(int line, uint nid);
	bool disconnectOut(int line, uint nid = 0);

	struct Redirection {
		Signal::Condition condition;
		int line = 0;
	};

	minivec<Redirection> redirections;

	int redirect(int line, minimap<Signal,&Signal::key> signals);
	Monorail* next(int* line, minimap<Signal,&Signal::key> signals);
};

struct MonorailSettings {
	MonorailSettings(Monorail& monorail);
	minivec<Monorail::Redirection> redirections;
	bool filling;
	bool emptying;
	struct {
		bool contents;
	} transmit;
	Point out[3];
	Point dir; // original direction
};
