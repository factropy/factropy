#pragma once

// A gridmap is a simple spatial index that breaks an unbounded 2D plane
// up into chunks/tiles of a predefined size. It can index anything that
// has an axis-aligned bounding box.

#include "common.h"
#include "gridwalk.h"
#include <map>

template <auto CHUNK, typename V>
struct gridmap {

	std::map<gridwalk::xy,minivec<V>> cells;

	gridmap() {};

	std::size_t memory() {
		return 0; //sizeof(*this) + cells.memory();
	}

	void insert(const Box& box, V id) {
		for (auto cell: gridwalk(CHUNK, box)) {
			cells[cell].push_back(id);
		}
	}

	void remove(const Box& box, V id) {
		for (auto cell: gridwalk(CHUNK, box.grow(0.1))) {
			auto it = cells.find(cell);
			if (it != cells.end()) {
				auto& v = it->second;
				v.erase(std::remove(v.begin(), v.end(), id), v.end());
				if (!v.size()) cells.erase(it);
			}
		}
	}

	void update(const Box& oldBox, const Box& newBox, V id) {
		if (oldBox == newBox) return;
		if (gridwalk(CHUNK, oldBox) == gridwalk(CHUNK, newBox)) return;
		remove(oldBox, id);
		insert(newBox, id);
	}

	void clear() {
		cells.clear();
	}

	localvec<V> dump(const Box& box) const {
		localvec<V> hits;
		for (auto cell: gridwalk(CHUNK, box)) {
			auto it = cells.find(cell);
			if (it != cells.end()) {
				auto& v = it->second;
				// hits.insert(hits.end(), v.begin(), v.end());
				hits.append(v.data(), v.size());
			}
		}
		return hits;
	}

	localvec<V> search(const Box& box) const {
		localvec<V> hits = dump(box);
		deduplicate(hits);
		return hits;
	}

	localvec<V> search(const Sphere& sphere) const {
		return search((Box){sphere.x, sphere.y, sphere.z, sphere.r*2, sphere.r*2, sphere.r*2});
	}
};
