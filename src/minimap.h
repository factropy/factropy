#pragma once

#include <cassert>
#include <new>
#include <type_traits>
#include <typeinfo>
#include "common.h"
#include "minivec.h"

/*
// A compact map for small objects containing an indexable field
// Sorted, but not stable

struct thing {
	uint key;
	float val;
};

minimap<thing,&thing::key> things;
things[1] = 2.0f;
things.insert((object){ .key = 2, .val = 2.0f });
auto& alpha = &things[1];
auto& beta = &things[2];

*/

template <class V, auto ID>
class minimap : public minivec<V> {
	static_assert(std::is_trivially_copyable<V>::value, "minimap<is_trivially_copyable>");

	typedef typename std::remove_reference<decltype(std::declval<V>().*ID)>::type K;

public:
	minimap<V,ID>() : minivec<V>() {
	}

	typedef K key_type;
	typedef typename minivec<V>::value_type value_type;
	typedef typename minivec<V>::size_type size_type;
	typedef typename minivec<V>::iterator iterator;

	iterator begin() const {
		return minivec<V>::begin();
	}

	iterator end() const {
		return minivec<V>::end();
	}

	iterator find(const key_type& k) const {
		return std::find_if(begin(), end(), [&](V v) { return v.*ID == k; });
	}

	bool has(const key_type& k) const {
		return find(k) != end();
	}

	bool contains(const key_type& k) const {
		return has(k);
	}

	void insert(const value_type& v) {
		if (!has(v.*ID)) {
			minivec<V>::push_back(v);
			std::sort(begin(), end(), [](auto a, auto b) { return a.*ID < b.*ID; });
		}
	}

	void erase(const key_type& k) {
		minivec<V>::erase(find(k));
		std::sort(begin(), end(), [](auto a, auto b) { return a.*ID < b.*ID; });
	}

	size_type count(const key_type& k) const {
		return has(k) ? 1: 0;
	}

	V& refer(const K& k) const {
		auto it = find(k);
		ensure(it != end())
		return *it;
	}

	V& operator[](const K& k) {
		auto it = find(k);
		if (it != end()) return *it;

		V v; v.*ID = k;
		insert(v);

		return refer(k);
	}

	minivec<key_type> keys() {
		minivec<key_type> out;
		for (auto pair: *this) out.push(pair.*ID);
		return out;
	}
};
