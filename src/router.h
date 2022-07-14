#pragma once

// Router components connect wifi networks and can generate alerts based
// on packet rules.

struct Router;
struct RouterSettings;

#include "slabmap.h"
#include "entity.h"
#include "networker.h"
#include "signal.h"

struct Router {
	uint id;
	Entity* en;
	Networker* networker;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Router,&Router::id> all;
	static Router& create(uint id);
	static Router& get(uint id);

	struct Rule {
		Signal::Condition condition;
		enum class Mode {
			Forward = 0,
			Generate,
			Alert,
		};
		Mode mode = Mode::Forward;
		Signal signal;
		int nicSrc = 0;
		int nicDst = 0;
		enum class Alert {
			Notice = 0,
			Warning,
			Critical,
		};
		Alert alert = Alert::Warning;
		int icon = 1;
	};

	minivec<Rule> rules;
	struct {
		int notice = 0;
		int warning = 0;
		int critical = 0;
	} alert;

	void destroy();
	void update();

	RouterSettings* settings();
	void setup(RouterSettings* settings);
};

struct RouterSettings {
	minivec<Router::Rule> rules;
	RouterSettings(Router& router);
};

