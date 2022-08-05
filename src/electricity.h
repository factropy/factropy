#pragma once

struct Entity;
struct ElectricityNetwork;
struct ElectricityProducer;
struct ElectricityConsumer;

#include "energy.h"
#include "minimap.h"

struct ElectricityNode {
	Entity* en = nullptr;
	ElectricityNetwork* network = nullptr;
	ElectricityNode() = default;
	ElectricityNode(Entity* ent, ElectricityNetwork* net);
	bool connected();
};

struct ElectricityProducer : ElectricityNode {
	ElectricityProducer() = default;
	ElectricityProducer(Entity* ent, ElectricityNetwork* net);
	void produce();
};

struct ElectricityConsumer : ElectricityNode {
	ElectricityConsumer() = default;
	ElectricityConsumer(Entity* ent, ElectricityNetwork* net);
	Energy consume(Energy e);
};

struct ElectricityNetwork {
	float load = 0.0f;
	float satisfaction = 0.0f;

	// Joules demanded last tick
	Energy demand = 0;
	// Joules supplied last tick
	Energy supply = 0;
	// Wattage theoretical limit this tick from all primary generators
	Energy capacity = 0;
	// Wattage theoretical limit this tick from all secondary buffers
	Energy capacityBuffered = 0;
	// Wattage actual limit this tick from fueled primary generators
	Energy capacityReady = 0;
	// Wattage actual limit this tick from charged secondary buffers
	Energy capacityBufferedReady = 0;
	// Joules stored this tick in secondary buffers
	Energy bufferedLevel = 0;
	// Aggregate size this tick of all secondary buffers
	Energy bufferedLimit = 0;

	minimap<ElectricityProducer,&ElectricityProducer::en> producers;
	minimap<ElectricityConsumer,&ElectricityConsumer::en> consumers;

	void add(Entity* en);
	void drop(Entity* en);
};


