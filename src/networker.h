#pragma once

// Networker components connect hubs and leaves into Wifi networks

struct Networker;
struct NetworkerSettings;

#include "slabmap.h"
#include "hashset.h"
#include "signal.h"

struct Networker {
	uint id;
	Entity* en;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Networker,&Networker::id> all;
	static Networker& create(uint id);
	static Networker& get(uint id);

	struct Network {
		std::string ssid;
		hashset<uint> hubs;
		hashset<uint> nodes;
		// cumulative signals from all nodes this tick
		minimap<Signal,&Signal::key> signals;
		miniset<uint> logisticStores;

		template <class S>
		int read(const S& s) {
			auto it = signals.find(s);
			return it == signals.end() ? 0: it->value;
		}
	};

	static inline std::vector<Network*> networks;
	static inline gridmap<64,Networker*> gridRanges;
	static inline hashset<uint> allHubs;
	static inline bool rebuild = false;

	struct Interface {
		std::string ssid;
		Network* network = nullptr;
		// transmit-next-tick queues
		minimap<Signal,&Signal::key> signals;

		void write(const Stack& stack);
		void write(const Amount& amount);
		void write(const Signal& signal);
	};

	std::vector<Interface> interfaces;
	bool managed;

	void destroy();
	void connect();

	bool isHub();
	bool isConnected();
	Point aerial();
	float range();
	void publish();
	Interface& input();
	Interface& output();
	bool used();
	void manage();
	void unmanage();

	std::vector<std::string> scan();
	static std::vector<std::string> ssids();

	NetworkerSettings* settings();
	void setup(NetworkerSettings*);

	std::vector<uint> links();
};

struct NetworkerSettings {
	std::vector<std::string> ssids;
	NetworkerSettings(Networker& networker);
};