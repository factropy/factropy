#pragma once

struct Entity;
struct ElectricityNetwork;
struct ElectricityProducer;
struct ElectricityConsumer;

#include "energy.h"
#include "slabmap.h"

struct ElectricityNode {
	uint id = 0;
	Entity* en = nullptr;
	ElectricityNetwork* network = nullptr;
	bool connected();
};

struct ElectricityProducer : ElectricityNode {
	static void reset();
	static inline slabmap<ElectricityProducer,&ElectricityProducer::id> all;
	static ElectricityProducer& create(uint id);
	static ElectricityProducer& get(uint id);
	void destroy();
	void produce();
	void connect();
	void disconnect();
};

struct ElectricityConsumer : ElectricityNode {
	static void reset();
	static inline slabmap<ElectricityConsumer,&ElectricityConsumer::id> all;
	static ElectricityConsumer& create(uint id);
	static ElectricityConsumer& get(uint id);
	void destroy();
	Energy consume(Energy e);
	void connect();
	void disconnect();
};

struct ElectricityBuffer : ElectricityNode {
	Energy level;
	static void reset();
	static inline slabmap<ElectricityBuffer,&ElectricityBuffer::id> all;
	static ElectricityBuffer& create(uint id);
	static ElectricityBuffer& get(uint id);
	void destroy();
	void charge();
	void discharge();
	void connect();
	void disconnect();
};

struct ElectricityNetworkState {
	float load = 0.0f;
	float satisfaction = 0.0f;
	float charge = 0.0f;
	float discharge = 0.0f;

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

	bool lowPower();
	bool brownOut();
	bool noCapacity();
};

struct ElectricityNetwork : ElectricityNetworkState {
	uint id = 0;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline uint sequence = 0;
	static inline bool rebuild = false;

	static inline slabmap<ElectricityNetwork,&ElectricityNetwork::id> all;
	static ElectricityNetwork& create(uint id);
	static ElectricityNetwork& get(uint id);

	miniset<uint> producers;
	miniset<uint> consumers;
	miniset<uint> buffers;
	miniset<uint> poles;

	std::map<Spec*,TimeSeries> production;
	std::map<Spec*,TimeSeries> consumption;

	void add(ElectricityProducer& producer);
	void add(ElectricityConsumer& consumer);
	void add(ElectricityBuffer& buffer);
	void drop(ElectricityProducer& producer);
	void drop(ElectricityConsumer& consumer);
	void drop(ElectricityBuffer& buffer);

	void updatePre();
	void updatePost();

	Energy consume(Spec* en, Energy e);
	void consume(Spec* spec, Energy e, int count);

	static ElectricityNetwork* primary();
	static ElectricityNetworkState aggregate();
};
