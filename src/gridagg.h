#pragma once

#include "common.h"
#include "gridwalk.h"
#include <map>

template <auto CHUNK, typename V>
struct gridagg {

	std::map<gridwalk::xy,V> cells;

	gridagg() {};

	std::size_t memory() {
		std::size_t size = 0;
		for (auto& [cell,item]: cells) {
			size += sizeof(cell) + sizeof(item);
		}
		return size;
	}

	void set(const Box& box, V v) {
		for (auto cell: gridwalk(CHUNK, box)) {
			cells[cell] = v;
		}
	}

	void clear() {
		cells.clear();
	}

	V agg(const Box& box, V def, std::function<V (V, V)> fn) {
		V a;
		bool first = true;
		for (auto cell: gridwalk(CHUNK, box)) {
			if (cells.count(cell)) {
				auto v = cells[cell];
				a = first ? v: fn(a, v);
				first = false;
			}
		}
		return first ? def: a;
	}

	V max(const Box& box, V def) {
		return agg(box, def, [](V a, V b) { return std::max(a, b); });
	}
};
