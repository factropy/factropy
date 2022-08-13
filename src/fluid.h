#pragma once

struct Fluid;
struct Amount;

#include "item.h"
#include "mass.h"
#include "energy.h"
#include "color.h"
#include "time-series.h"
#include "slabarray.h"

struct Fluid {
	uint id;
	static void saveAll(const char *path);
	static void loadAll(const char *path);
	static void reset();

	static inline uint sequence;
	static uint next();

	static inline std::map<std::string,Fluid*> names;
	static inline slabarray<Fluid,1024> all;
	static Fluid* create(uint id, std::string name);
	static Fluid* byName(std::string name);
	static Fluid* get(uint id);

	static inline std::map<uint,float> drilling;

	std::string name;
	std::string title;
	Color color;
	Liquid liquid;
	Energy thermal;
	uint64_t produced;
	uint64_t consumed;
	TimeSeriesSum production;
	TimeSeriesSum consumption;
	bool raw;
	std::string order;

	static inline bool sort(const Fluid* a, const Fluid* b) {
		return a->order < b->order
			|| (a->order == b->order && a->title < b->title)
			|| (a->order == b->order && a->title == b->title && a->name < b->name);
	}

	Fluid();
	Fluid(uint id, std::string name);
	~Fluid();

	bool manufacturable();
	void produce(int count);
	void consume(int count);
};

struct Amount {
	uint fid;
	uint size;

	Amount();
	Amount(std::initializer_list<uint>);
};
