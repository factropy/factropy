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
things[1].val = 2.0f;
things.insert((object){ .key = 2, .val = 2.0f });
auto& alpha = &things[1];
auto& beta = &things[2];

*/

template <class V, auto ID, class A = minialloc_def>
class minimap : public minivec<V,A> {
	static_assert(std::is_trivially_copyable<V>::value, "minimap<is_trivially_copyable>");

	typedef typename std::remove_reference<decltype(std::declval<V>().*ID)>::type K;

public:
	minimap<V,ID,A>() : minivec<V,A>() {
	}

	minimap<V,ID,A>(const minimap<V,ID,A>& other) : minivec<V,A>(other) {
	}

	minimap<V,ID,A>(minimap<V,ID,A>&& other) : minivec<V,A>(other) {
	}

	minimap<V,ID,A>(std::initializer_list<V> l) : minivec<V,A>(l) {
		minivec<V,A>::clear();
		for (auto& v: l) insert(v);
	}

	minimap<V,ID,A>& operator=(const minimap<V,ID,A>& other) {
		minivec<V,A>::operator=(other);
		return *this;
	}

	minimap<V,ID,A>& operator=(minimap<V,ID,A>&& other) {
		minivec<V,A>::operator=(other);
		return *this;
	}

	typedef K key_type;
	typedef typename minivec<V,A>::value_type value_type;
	typedef typename minivec<V,A>::size_type size_type;
	typedef typename minivec<V,A>::iterator iterator;

	iterator begin() const {
		return minivec<V,A>::begin();
	}

	iterator end() const {
		return minivec<V,A>::end();
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
			minivec<V,A>::push_back(v);
			std::sort(begin(), end(), [](auto a, auto b) { return a.*ID < b.*ID; });
		}
	}

	void erase(const key_type& k) {
		minivec<V,A>::erase(find(k));
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

template <class V, auto ID>
class localmap : public minimap<V,ID,minialloc_local> {
public:
	localmap<V,ID>() : minimap<V,ID,minialloc_local>() {
	}

	localmap<V,ID>(const localmap<V,ID>& other) : minimap<V,ID,minialloc_local>(other) {
	}

	localmap<V,ID>(localmap<V,ID>&& other) : minimap<V,ID,minialloc_local>(other) {
	}

	localmap<V,ID>(std::initializer_list<V> l) : minimap<V,ID,minialloc_local>(l) {
	}

	localmap<V,ID>& operator=(const localmap<V,ID>& other) {
		minimap<V,ID,minialloc_local>::operator=(other);
		return *this;
	}

	localmap<V,ID>& operator=(localmap<V,ID>&& other) {
		minimap<V,ID,minialloc_local>::operator=(other);
		return *this;
	}
};

