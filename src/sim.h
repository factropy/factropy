#pragma once

#include "common.h"
#include "opensimplex.h"
#include "time-series.h"
#include "point.h"
#include "message.h"
#include <mutex>
#include <functional>
#include <random>

namespace Sim {

	extern TimeSeries statsTick;
	extern TimeSeries statsChunk;
	extern TimeSeries statsElectricityDemand;
	extern TimeSeries statsElectricitySupply;
	extern TimeSeries statsEntityPre;
	extern TimeSeries statsEntityPost;
	extern TimeSeries statsGhost;
	extern TimeSeries statsNetworker;
	extern TimeSeries statsPile;
	extern TimeSeries statsExplosive;
	extern TimeSeries statsStore;
	extern TimeSeries statsArm;
	extern TimeSeries statsCrafter;
	extern TimeSeries statsVenter;
	extern TimeSeries statsEffector;
	extern TimeSeries statsLauncher;
	extern TimeSeries statsConveyor;
	extern TimeSeries statsUnveyor;
	extern TimeSeries statsLoader;
	extern TimeSeries statsBalancer;
	extern TimeSeries statsRopeway;
	extern TimeSeries statsPath;
	extern TimeSeries statsVehicle;
	extern TimeSeries statsCart;
	extern TimeSeries statsPipe;
	extern TimeSeries statsDepot;
	extern TimeSeries statsDrone;
	extern TimeSeries statsMissile;
	extern TimeSeries statsExplosion;
	extern TimeSeries statsTurret;
	extern TimeSeries statsComputer;
	extern TimeSeries statsRouter;
	extern TimeSeries statsEnemy;
	extern TimeSeries statsZeppelin;
	extern TimeSeries statsFlightLogistic;
	extern TimeSeries statsFlightPath;
	extern TimeSeries statsTube;
	extern TimeSeries statsTeleporter;
	extern TimeSeries statsMonorail;
	extern TimeSeries statsMonocar;
	extern TimeSeries statsSource;

	extern OpenSimplex* opensimplex;
	extern uint64_t tick;
	extern int64_t seed;

	extern channel<bool,3> saveTickets;

	struct Alerts {
		uint active = 0;
		uint customNotice = 0;
		uint customWarning = 0;
		uint customCritical = 0;
		uint vehiclesBlocked = 0;
		uint monocarsBlocked = 0;
		uint entitiesDamaged = 0;
	};

	extern Alerts alerts;
	extern const std::vector<const char*> customIcons;

	void reset();
	void save();
	void load();

	void reseed(int64_t seed);
	float random();
	int choose(int range);
	std::mt19937* urng();

	typedef std::function<void(void)> lockCallback;
	void locked(lockCallback cb);

	// decrease persistence to make coastline smoother
	// increase frequency to make lakes smaller
	double noise2D(double x, double y, int layers, double persistence, double frequency);

	bool rayCast(Point a, Point b, float clearance, std::function<bool(uint)> collide);

	float windSpeed(Point p);

	bool save(const char *path, Point camPos, Point camDir, uint directing);
	std::tuple<Point,Point,uint> load(const char *path);

	void update();
}
