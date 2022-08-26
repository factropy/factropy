#pragma once

// Arm components move items between Stores and Conveyors.

struct Arm;
struct ArmSettings;

#include "slabmap.h"
#include "miniset.h"
#include "item.h"
#include "signal.h"
#include "entity.h"
#include <map>

struct Arm {
	uint id;
	Entity* en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);
	static std::size_t memory();

	static inline slabmap<Arm,&Arm::id> all;
	static Arm& create(uint id);
	static Arm& get(uint id);

	static inline float speedFactor = 1.0f;

	// state machine
	enum Stage {
		Input = 1,
		ToInput,
		Output,
		ToOutput,
		Parked,
		Parking,
		Unparking,
	};

	uint iid;
	uint inputId;
	uint inputStoreId;
	uint outputId;
	uint outputStoreId;
	bool inputNear;
	bool inputFar;
	bool outputNear;
	bool outputFar;
	float orientation;
	float speed;
	enum Stage stage;
	uint64_t pause;
	miniset<uint> filter;

	enum class Monitor {
		InputStore = 0,
		OutputStore,
		Network,
	};

	Monitor monitor;
	Signal::Condition condition;

	ArmSettings* settings();
	void setup(ArmSettings*);

	void destroy();
	void update();
	void updateProximity();
	bool updateInput();
	bool updateOutput();
	bool updateReady();
	bool checkCondition();
	Point input();
	Point output();
	uint source();
	uint target();
	Stack transferStoreToStore(Store& dst, Store& src);
	Stack transferStoreToBelt(Store& src);
	Stack transferBeltToStore(Store& dst, Stack stack);
};

struct ArmSettings {
	miniset<uint> filter;
	Arm::Monitor monitor;
	Signal::Condition condition;
	bool inputNear = false;
	bool inputFar = false;
	bool outputNear = false;
	bool outputFar = false;
	ArmSettings() = default;
	ArmSettings(Arm& arm);
};
