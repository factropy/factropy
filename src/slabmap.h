#pragma once

#include <vector>
#include <functional>
#include <cassert>
#include <new>
#include <type_traits>
#include <typeinfo>
#include "common.h"
#include "slabpool.h"
#include "minivec.h"
#include "hashset.h"

// A hash-table/slab-allocator that:
// - stores objects that contain their own key as a hashable field
// - allocates object memory in pages (iteration locality)
// - vectors for hash buckets (lookup locality, higher load factors)

template <class V, auto ID, uint slabSize = 1024>
class slabmap {
private:

	typedef typename std::remove_reference<decltype(std::declval<V>().*ID)>::type K;

	slabpool<V,slabSize> pool;

	typedef typename slabpool<V,slabSize>::slabslot slabslot;

	struct entry {
		K key;
		slabslot slot;

		bool operator==(const entry& o) const {
			return key == o.key;
		}
	};

	struct hasher {
		std::size_t operator()(const entry& e) const noexcept {
			return std::hash<K>{}(e.key);
		}
	};

	hashset<entry,hasher> index;

public:

	slabmap<V,ID,slabSize>() {
	}

	~slabmap<V,ID,slabSize>() {
		clear();
	}

	std::size_t memory() {
		return pool.memory() + index.memory();
	}

	void clear() {
		pool.clear();
		index.clear();
	}

	bool has(const K& k) const {
		return index.has((entry){.key = k});
	}

	bool contains(const K& k) const {
		return has(k);
	}

	uint count(const K& k) const {
		return has(k) ? 1: 0;
	}

	std::size_t size() const {
		return pool.size();
	}

	bool erase(const K& k) {
		auto it = index.find((entry){.key = k});
		if (it != index.end()) {
			pool.releaseSlot((*it).slot);
			index.erase(it);
			return true;
		}
		return false;
	}

	V& refer(const K& k) const {
		auto it = index.find((entry){.key = k});
		ensure(it != index.end());
		return pool.referSlot((*it).slot);
	}

	V& operator[](const K& k) {
		auto it = index.find((entry){.key = k});
		if (it != index.end()) {
			return pool.referSlot((*it).slot);
		}

		slabslot ss = pool.requestSlot();
		V& v = pool.referSlot(ss);
		v.*ID = k;

		index.insert((entry){
			.key = k,
			.slot = ss,
		});

		return v;
	}

	typedef typename slabpool<V,slabSize>::iterator iterator;

	iterator begin() {
		return pool.begin();
	}

	iterator end() {
		return pool.end();
	}

	V* point(const K& k) {
		auto it = index.find((entry){.key = k});
		return it != index.end() ? &pool.referSlot((*it).slot): nullptr;
	}

	V& front() {
		return *begin();
	}
};
