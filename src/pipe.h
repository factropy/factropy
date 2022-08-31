#pragma once

// Pipe components transport fluid. They automatically link together with adjacent
// pipes to form a network holding a single fluid type.

// Fluid modelling is fairly simplistic so far; the network tracks the fluid level
// as a percentage of $totalNetworkCapacity and each Pipe just assumes it has
// ($localCapacity * $levelPercentage) fluid available.

struct Pipe;
struct PipeNetwork;
struct PipeSettings;

#include "fluid.h"
#include "entity.h"
#include "slabmap.h"
#include <set>

struct Pipe {
	uint id;
	static void reset();
	static void tick();
	static void saveAll(const char* name);
	static void loadAll(const char* name);

	static inline slabmap<Pipe,&Pipe::id> all;
	static Pipe& create(uint id);
	static Pipe& get(uint id);

	static localvec<uint> servicing(Box box);
	static localvec<uint> servicing(Box box, const localvec<Entity*>& candidates);
	static inline hashset<uint> changed;

	PipeNetwork* network;
	uint cacheFid;
	int cacheTally;
	uint partner;
	bool underground;
	bool underwater;
	bool managed;
	bool transmit;
	miniset<uint> connections;

	static inline hashset<uint> transmitters;

	enum Valve {
		Overflow = 0,
		TopUp,
	};

	bool valve;
	float overflow;
	uint filter;
	uint src;
	uint dst;

	void destroy();
	PipeSettings* settings();
	void setup(PipeSettings*);
	void manage();
	void unmanage();
	localvec<Point> pipeConnections();
	Amount contents();
	bool connect(uint pid, localvec<uint>& affected);
	bool disconnect(uint pid, localvec<uint>& affected);
	void findPartner(localvec<uint>& affected);
	Box undergroundRange();
	Box underwaterRange();
	void flush();
	void valveLinks();
	void valveTick();
	localvec<uint> siblings();
};

struct PipeNetwork {
	static void reset();
	static void tick();
	static inline std::set<PipeNetwork*> all;

	std::set<uint> pipes;
	uint fid;
	int tally;
	Liquid limit;

	PipeNetwork();
	~PipeNetwork();
	void propagateLevels();

	void cacheState();
	Amount inject(Amount amount);
	Amount extract(Amount amount);
	uint count(uint fid);
	uint space(uint fid);
	float level();
	void flush();
	bool valve();
	Pipe& first();
};

struct PipeSettings {
	float overflow = 0;
	uint filter = 0;
	PipeSettings() = default;
	PipeSettings(Pipe& pipe);
};
