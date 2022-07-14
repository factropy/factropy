#pragma once
#include <set>
#include "box.h"
#include "sim.h"
#include "route.h"
#include "minivec.h"
#include "minimap.h"

// The Sky is broken up into a lattice of chunks used by aircraft to
// reserve non-colliding flight paths

struct Sky {
	int chunk() const;
	int size() const;

	struct Block {
		int x = 0;
		int y = 0;
		int z = 0;

		struct Reservation {
			uint rid = 0;
			uint64_t until = 0;
		};

		minimap<Reservation,&Reservation::rid> reservations;

		bool clear = false;

		Box box() const;
		Point centroid() const;

		bool operator==(const Block& o) const {
			return x == o.x && y == o.y && z == o.z;
		}

		bool operator!=(const Block& o) const {
			return !operator==(o);
		}

		bool operator<(const Block& o) const {
			return x < o.x || (x == o.x && y < o.y) || (x == o.x && y == o.y && z < o.z);
		}

		void reserve(uint rid) {
			reservations[rid].until = Sim::tick+60;
		}

		void purge() {
			minivec<uint> drop;
			for (auto res: reservations) {
				if (res.until <= Sim::tick) drop.push(res.rid);
			}
			for (auto rid: drop) {
				reservations.erase(rid);
			}
		}

		bool reserved() {
			purge();
			for (auto res: reservations) {
				if (res.until > Sim::tick) return true;
			}
			return false;
		}

		bool reserved(uint rid) {
			purge();
			for (auto res: reservations) {
				if (res.rid == rid && res.until > Sim::tick) return true;
			}
			return false;
		}
	};

	std::set<Block> blocks;

	Block* get(int x, int y, int z);
	Block* nearest(Point pos);
	Block* within(Point pos);

	void init();
	void reset();

	struct Path {
		minivec<Block*> blocks;
		minivec<Point> waypoints;
	};

	Path path(Point origin, Point target, uint rid);

	void save(const char* path);
	void load(const char* path);
};

extern Sky sky;
