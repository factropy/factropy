#pragma once

// Vehicle components are ground cars that path-find and drive around the map.
// They can be manualy told to move, or to follow a series of waypoints in patrol
// mode.

struct Vehicle;

#include "entity.h"
#include "path.h"
#include <list>
#include <vector>

struct Vehicle {
	uint id;

	struct Route: Path {
		Vehicle *vehicle;
		Route(Vehicle*);
		std::vector<Point> getNeighbours(Point);
		double calcCost(Point,Point);
		double calcHeuristic(Point);
		bool rayCast(Point,Point);
	};

	struct Waypoint;

	struct DepartCondition {
		virtual bool ready(Waypoint* waypoint, Vehicle *vehicle) = 0;
		virtual ~DepartCondition();
	};

	struct DepartInactivity: DepartCondition {
		int seconds;
		virtual bool ready(Waypoint* waypoint, Vehicle *vehicle);
		virtual ~DepartInactivity();
	};

	struct DepartItem: DepartCondition {
		static const uint Eq;
		static const uint Ne;
		static const uint Lt;
		static const uint Lte;
		static const uint Gt;
		static const uint Gte;

		uint iid;
		uint count;
		uint op;
		virtual bool ready(Waypoint* waypoint, Vehicle *vehicle);
		virtual ~DepartItem();
	};

	struct Waypoint {
		Point position;
		uint stopId;
		std::vector<DepartCondition*> conditions;

		Waypoint(Point pos);
		Waypoint(uint eid);
		~Waypoint();
	};

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Vehicle,&Vehicle::id> all;
	static Vehicle& create(uint id);
	static Vehicle& get(uint id);

	std::list<Point> path;
	std::list<Waypoint*> waypoints;
	Waypoint* waypoint;
	Route *pathRequest = nullptr;
	uint64_t pause = 0;
	bool patrol;
	bool handbrake;

	void destroy();
	void update();
	Waypoint* addWaypoint(Point p);
	Waypoint* addWaypoint(uint eid);

	std::vector<Point> sensors();
	bool moving();
	static bool collide(Spec* spec);
};
