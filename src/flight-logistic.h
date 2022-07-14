#pragma once

// Logistic flights

struct FlightLogistic;

#include "entity.h"
#include "signal.h"
#include "flight-path.h"
#include "flight-pad.h"

struct FlightLogistic {
	uint id;
	uint dep;
	uint src;
	uint dst;
	Entity* en;
	FlightPath* flight;
	Store* store;

	enum Stage {
		Home = 0,
		ToSrc,
		ToSrcDeparting,
		ToSrcFlight,
		Loading,
		ToDst,
		ToDstDeparting,
		ToDstFlight,
		Unloading,
		ToDep,
		ToDepDeparting,
		ToDepFlight,
	};

	Stage stage;

	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<FlightLogistic,&FlightLogistic::id> all;
	static FlightLogistic& create(uint id);
	static FlightLogistic& get(uint id);

	void destroy();
	void update();
	void releaseAllExcept(uint pid);

	void updateHome();
	void updateToSrc();
	void updateToSrcDeparting();
	void updateToSrcFlight();
	void updateLoading();
	void updateToDst();
	void updateToDstDeparting();
	void updateToDstFlight();
	void updateUnloading();
	void updateToDep();
	void updateToDepDeparting();
	void updateToDepFlight();
};

