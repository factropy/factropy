#pragma once

// Logistic carts

struct Cart;
struct CartSettings;

#include "entity.h"
#include "signal.h"
#include "cart-stop.h"
#include "cart-waypoint.h"

struct Cart {
	uint id;
	Entity* en;

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Cart,&Cart::id> all;
	static Cart& create(uint id);
	static Cart& get(uint id);

	void destroy();
	void update();

	CartSettings* settings();
	void setup(CartSettings*);

	enum class State {
		Start = 0,
		Travel,
		Stop,
		Ease,
	};

	State state;
	Point target;
	Point origin;
	Point next;
	int line;
	uint64_t wait;
	uint64_t pause;
	uint64_t surface;
	Signal signal;
	std::string status;
	float fueled;
	float speed;
	bool halt;
	bool lost;
	bool slab;
	bool blocked;

	void start();
	void travel();
	void stop();
	void ease();
	void travelTo(Point point);
	CartStop* getStop(Point point);
	CartWaypoint* getWaypoint(Point point);
	std::vector<Point> sensors();
	Sphere groundSphere();
	Point ahead();
	Point park();
	minimap<Signal,&Signal::key> signals();
	float maxSpeed();
};

struct CartSettings {
	int line;
	Signal signal;
	CartSettings(Cart& cart);
};
