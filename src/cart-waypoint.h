#pragma once

// CartWaypoint components are markers with rules used by Carts to navigate

struct CartWaypoint;
struct CartWaypointSettings;

#include "entity.h"
#include "cart.h"
#include "cart-stop.h"

struct CartWaypoint {
	uint id;

	static void reset();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<CartWaypoint,&CartWaypoint::id> all;
	static CartWaypoint& create(uint id);
	static CartWaypoint& get(uint id);
	void destroy();

	CartWaypointSettings* settings();
	void setup(CartWaypointSettings*);

	static const int Red = 0;
	static const int Blue = 1;
	static const int Green = 2;
	Point relative[3];

	void setNext(int line, Point absolute);
	void clrNext(int line);
	Point getPos() const;
	Point getNext(int line) const;
	bool hasNext(int line) const;

	struct Redirection {
		Signal::Condition condition;
		int line = 0;
	};

	minivec<Redirection> redirections;

	int redirect(int line, minimap<Signal,&Signal::key> signals);
};

struct CartWaypointSettings {
	Point dir; // original direction
	Point relative[3];
	minivec<CartWaypoint::Redirection> redirections;
	CartWaypointSettings() = default;
	CartWaypointSettings(CartWaypoint& waypoint);
};
