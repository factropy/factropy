#include "common.h"
#include "spec.h"
#include "entity.h"
#include "world.h"
#include "sim.h"
#include "enemy.h"
#include "time-series.h"
#include "crew.h"
#include "goal.h"
#include "recipe.h"
#include <cstdlib>
#include <random>

namespace Sim {

	TimeSeries statsTick;
	TimeSeries statsChunk;
	TimeSeries statsEntityPre;
	TimeSeries statsEntityPost;
	TimeSeries statsGhost;
	TimeSeries statsNetworker;
	TimeSeries statsPile;
	TimeSeries statsExplosive;
	TimeSeries statsStore;
	TimeSeries statsArm;
	TimeSeries statsCrafter;
	TimeSeries statsVenter;
	TimeSeries statsEffector;
	TimeSeries statsLauncher;
	TimeSeries statsConveyor;
	TimeSeries statsUnveyor;
	TimeSeries statsLoader;
	TimeSeries statsBalancer;
	TimeSeries statsPath;
	TimeSeries statsVehicle;
	TimeSeries statsCart;
	TimeSeries statsPipe;
	TimeSeries statsDepot;
	TimeSeries statsDrone;
	TimeSeries statsMissile;
	TimeSeries statsExplosion;
	TimeSeries statsTurret;
	TimeSeries statsComputer;
	TimeSeries statsRouter;
	TimeSeries statsEnemy;
	TimeSeries statsZeppelin;
	TimeSeries statsFlightLogistic;
	TimeSeries statsFlightPath;
	TimeSeries statsTube;
	TimeSeries statsTeleporter;
	TimeSeries statsMonorail;
	TimeSeries statsMonocar;
	TimeSeries statsSource;
	TimeSeries statsPowerPole;
	TimeSeries statsCharger;

	OpenSimplex* opensimplex = nullptr;
	std::mutex mutex;
	uint64_t tick = 0;
	int64_t seed = 0;
	thread_local int64_t mySeed = 0;
	thread_local std::mt19937* mt = nullptr;
	Alerts alerts;

	const std::vector<const char*> customIcons {
		ICON_FA_CHECK,
		ICON_FA_EXCLAMATION,
		ICON_FA_EXCLAMATION_CIRCLE,
		ICON_FA_EXCLAMATION_TRIANGLE,
		ICON_FA_WRENCH,
		ICON_FA_COG,
		ICON_FA_TRUCK,
		ICON_FA_TRAIN,
		ICON_FA_ROCKET,
		ICON_FA_WIFI,
		ICON_FA_BOLT,
		ICON_FA_THERMOMETER_EMPTY,
		ICON_FA_THERMOMETER_QUARTER,
		ICON_FA_THERMOMETER_HALF,
		ICON_FA_THERMOMETER_THREE_QUARTERS,
		ICON_FA_THERMOMETER_FULL,
		ICON_FA_BATTERY_EMPTY,
		ICON_FA_BATTERY_QUARTER,
		ICON_FA_BATTERY_HALF,
		ICON_FA_BATTERY_THREE_QUARTERS,
		ICON_FA_BATTERY_FULL,
	};

	void reseedThread() {
		if (!mt || seed != mySeed) {
			delete mt;
			mt = new std::mt19937(seed);
			mySeed = seed;
		}
	}

	float random() {
		int result = choose(std::numeric_limits<int>::max());
		return (float)(result%1000) / 1000.0f;
	}

	int choose(int range) {
		reseedThread();
		range = std::max(1, std::abs(range));
		std::uniform_int_distribution<int> distribution(0, range-1);
		return distribution(*mt);
	}

	std::mt19937* urng() {
		reseedThread();
		return mt;
	}

	void init(int64_t s) {
		seed = s;
		opensimplex = OpenSimplexNew(s);
	}

	void reset() {
		tick = 0;
		seed = 0;
		statsTick.clear();
		statsChunk.clear();
		statsEntityPre.clear();
		statsEntityPost.clear();
		statsGhost.clear();
		statsNetworker.clear();
		statsPile.clear();
		statsExplosive.clear();
		statsStore.clear();
		statsArm.clear();
		statsCrafter.clear();
		statsVenter.clear();
		statsEffector.clear();
		statsLauncher.clear();
		statsConveyor.clear();
		statsUnveyor.clear();
		statsLoader.clear();
		statsBalancer.clear();
		statsPath.clear();
		statsVehicle.clear();
		statsCart.clear();
		statsPipe.clear();
		statsDepot.clear();
		statsDrone.clear();
		statsMissile.clear();
		statsExplosion.clear();
		statsTurret.clear();
		statsComputer.clear();
		statsRouter.clear();
		statsEnemy.clear();
		statsZeppelin.clear();
		statsFlightLogistic.clear();
		statsFlightPath.clear();
		statsTube.clear();
		statsTeleporter.clear();
		statsMonorail.clear();
		statsMonocar.clear();
		statsSource.clear();
		statsPowerPole.clear();
		statsCharger.clear();

		for (auto& item: Item::all) {
			item.production.clear();
			item.consumption.clear();
		}

		for (auto& fluid: Fluid::all) {
			fluid.production.clear();
			fluid.consumption.clear();
		}

		OpenSimplexFree(opensimplex);
		opensimplex = nullptr;
	}

	void locked(lockCallback cb) {
		const std::lock_guard<std::mutex> lock(mutex);
		cb();
	}

	// decrease persistence to make coastline smoother
	// increase frequency to make lakes smaller
	double noise2D(double x, double y, int layers, double persistence, double frequency) {
		double amp = 1.0;
		double ampSum = 0.0;
		double result = 0.0;

		for (int i = 0; i < layers; i++) {
			result += OpenSimplexNoise(opensimplex, x*frequency, y*frequency) * amp;
			ampSum += amp;
			amp *= persistence;
			frequency *= 2;
		}

		double noise = result / ampSum;
		noise -= 0.5;
		noise *= 1.5;
		noise += 0.5;
		return std::clamp(noise, 0.0, 1.0);
	}

	bool rayCast(Point a, Point b, float clearance, std::function<bool(uint)> collide) {

		Point n = (b-a).normalize();

		for (Point c = a; c.distance(b) > 1.0f; c += n) {
			for (auto en: Entity::intersecting(c.box().grow(clearance))) {
				if (collide(en->id)) return false;
			}
		}

		return true;
	}

	float windSpeed(Point p) {
		return std::max((real)1.0, p.y);
	}

	void update() {
		ensure(Entity::mutating);

		alerts.customNotice = 0;
		alerts.customWarning = 0;
		alerts.customCritical = 0;
		alerts.vehiclesBlocked = 0;
		alerts.monocarsBlocked = 0;
		alerts.entitiesDamaged = 0;

		tick++;

		for (auto& item: Item::all) {
			item.production.set(Sim::tick, 0);
			item.consumption.set(Sim::tick, 0);
			item.supplies.set(Sim::tick, 0);
		}

		for (auto& fluid: Fluid::all) {
			fluid.production.set(Sim::tick, 0);
			fluid.consumption.set(Sim::tick, 0);
		}

		statsEntityPre.track(tick, Entity::preTick);
		statsGhost.track(tick, Ghost::tick);
		statsNetworker.track(tick, Networker::tick);
		statsPowerPole.track(tick, PowerPole::tick);
		statsCharger.track(tick, Charger::tick);
		statsPile.track(tick, Pile::tick);
		statsExplosive.track(tick, Explosive::tick);

		Entity::mutating = false;

		// GroupA: components that can run concurrently with the pathfinder and each other
		trigger groupA;

		crew.job([&]() {
			statsStore.track(tick, Store::tick);
			statsPipe.track(tick, Pipe::tick);
			groupA.now();
		});

		crew.job([&]() {
			statsConveyor.track(tick, Conveyor::tick);
			groupA.now();
		});

		groupA.wait(2);

		Entity::mutating = true;

		// Components that cannot run concurrently with anything

		statsSource.track(tick, Source::tick);
		statsBalancer.track(tick, Balancer::tick);
		statsUnveyor.track(tick, Unveyor::tick);
		statsArm.track(tick, Arm::tick);
		statsLoader.track(tick, Loader::tick);
		statsTube.track(tick, Tube::tick);
		statsMonocar.track(tick, Monocar::tick);
		statsMonorail.track(tick, Monorail::tick);
		statsTeleporter.track(tick, Teleporter::tick);
		statsEffector.track(tick, Effector::tick);
		statsCrafter.track(tick, Crafter::tick);
		statsVenter.track(tick, Venter::tick);
		statsLauncher.track(tick, Launcher::tick);
		statsVehicle.track(tick, Vehicle::tick);
		statsFlightPath.track(tick, FlightPath::tick);
		statsZeppelin.track(tick, Zeppelin::tick);
		statsFlightLogistic.track(tick, FlightLogistic::tick);
		statsCart.track(tick, Cart::tick);
		statsDrone.track(tick, Drone::tick);
		statsTurret.track(tick, Turret::tick);
		statsComputer.track(tick, Computer::tick);
		statsRouter.track(tick, Router::tick);
		statsExplosion.track(tick, Explosion::tick);
		statsMissile.track(tick, Missile::tick);
		statsDepot.track(tick, Depot::tick);

		statsEnemy.track(tick, Enemy::tick);
		statsEntityPost.track(tick, Entity::postTick);

		alerts.entitiesDamaged = Entity::damaged.size();

		alerts.active = 0
			+ alerts.customNotice
			+ alerts.customWarning
			+ alerts.customCritical
			+ (alerts.vehiclesBlocked ? 1:0)
			+ (alerts.monocarsBlocked ? 1:0)
			+ (alerts.entitiesDamaged ? 1:0)
		;

		Goal::tick();
		Recipe::tick();
		Message::tick();

		for (auto& item: Item::all) {
			item.production.update(Sim::tick);
			item.consumption.update(Sim::tick);
			item.supplies.update(Sim::tick);
		}

		for (auto& fluid: Fluid::all) {
			fluid.production.update(Sim::tick);
			fluid.consumption.update(Sim::tick);
		}

		Signal::gcLabels();
	}
}

