#pragma once

#include "hashset.h"
#include "miniset.h"

// An activeset load-balances operations over multiple ticks.

template <typename V, uint width = 60>
struct activeset {
	std::array<std::set<V>,width> asleep;
	uint offset = 0;
	std::set<V> awake;
	std::set<V> pausing;

	typedef decltype(asleep[0].begin()) iterator;

	activeset() {
	}

	iterator begin() {
		return awake.begin();
	}

	iterator end() {
		return awake.end();
	}

	void insert(V v) {
		pausing.insert(v);
	}

	void erase(V v) {
		awake.erase(v);
		pausing.erase(v);
		for (uint i = 0; i < width; i++) {
			asleep[i].erase(v);
		}
	}

	void clear() {
		offset = 0;
		awake.clear();
		pausing.clear();
		for (uint i = 0; i < width; i++) {
			asleep[i].clear();
		}
	}

	uint size() {
		uint count = pausing.size();
		for (uint i = 0; i < width; i++) {
			count += asleep[i].size();
		}
		return count;
	}

	uint smallest() {
		uint index = 0;
		uint size = asleep[0].size();
		for (uint i = 1; i < width; i++) {
			if (asleep[i].size() < size) {
				index = i;
				size = asleep[i].size();
			}
		}
		return index;
	};

	void pause(V v) {
		pausing.insert(v);
	}

	void tick() {
		for (auto v: pausing) asleep[smallest()].insert(v);
		pausing.clear();

		awake.clear();
		awake.insert(asleep[offset].begin(), asleep[offset].end());
		asleep[offset].clear();

		if (++offset == width) offset = 0;
	}
};