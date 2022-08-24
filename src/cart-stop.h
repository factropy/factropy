#pragma once

// CartStop components are markers with rules used by Carts to load/unload

struct CartStop;
struct CartStopSettings;

#include "entity.h"
#include "miniset.h"
#include "cart.h"
#include "cart-waypoint.h"

struct CartStop {
	uint id = 0;
	Entity* en = nullptr;

	static void tick();
	static void reset();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<CartStop,&CartStop::id> all;
	static CartStop& create(uint id);
	static CartStop& get(uint id);
	void destroy();
	void update();

	CartStopSettings* settings();
	void setup(CartStopSettings*);

	enum class Depart {
		Inactivity = 0,
		Empty,
		Full,
	};

	Depart depart;
};

struct CartStopSettings {
	CartStop::Depart depart;
	Signal::Condition condition;
	CartStopSettings() = default;
	CartStopSettings(CartStop& stop);
};