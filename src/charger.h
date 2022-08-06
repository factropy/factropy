#pragma once

// Charger components buffer electricity.

struct Charger;

#include "slabmap.h"
#include "store.h"
#include "electricity.h"

struct Charger {
	uint id;
	Entity* en;
	ElectricityNetwork* network;
	static void tickCharge();
	static void tickDischarge();
	static void reset();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Charger,&Charger::id> all;
	static Charger& create(uint id);
	static Charger& get(uint id);

	Energy energy;
	Energy buffer;

	void updateNetwork();

	void destroy();
	Energy consume(Energy e);
	Energy chargePrimaryRate();
	void chargePrimary();
	void chargeSecondary(float rate);
	Energy chargeSecondaryPredict();
	void dischargeSecondary(float rate);
	Energy dischargeSecondaryPredict();
	float level();
};
