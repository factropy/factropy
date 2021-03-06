#pragma once

#include <cassert>
#include <new>
#include <type_traits>
#include <typeinfo>
#include "common.h"
#include "minivec.h"

// A compact std::set alternative for small sets of trivial types

template <class V>
class miniset : public minivec<V> {
	static_assert(std::is_trivially_copyable<V>::value, "miniset<is_trivially_copyable>");

public:
	typedef typename minivec<V>::value_type key_type;
	typedef typename minivec<V>::size_type size_type;
	typedef typename minivec<V>::iterator iterator;

	miniset<V>() : minivec<V>() {
	}

	miniset<V>(const miniset<V>& other) : minivec<V>(other) {
	}

	miniset<V>(std::initializer_list<V> l) : minivec<V>(l) {
	}

	iterator begin() const {
		return minivec<V>::begin();
	}

	iterator end() const {
		return minivec<V>::end();
	}

	iterator find(const key_type& k) const {
		// Could use std::lower_bound for a binary search here because the
		// elements are ordered, but the idea (platform-dependent assumption)
		// behind miniset is that small vectors are often fastest to simply
		// scan sequentially. If this is untrue for a certain use case then
		// it is time to switch back to a std::set anyway.
		return std::find(begin(), end(), k);
	}

	bool has(const key_type& k) const {
		return find(k) != end();
	}

	bool contains(const key_type& k) const {
		return has(k);
	}

	void insert(const key_type& k) {
		// Unlike find() std::lower_bound is necessary here to avoid an
		// extra std::sort step. Unclear whether miniset is better than a
		// theoretical "miniunordered_set", but as minivec tries to be
		// consistent with std::vector behaviour, so miniset aims to be
		// consistent with std::set behaviour.
		auto lower = std::lower_bound(begin(), end(), k);
		if (lower == end()) {
			minivec<V>::push_back(k);
			return;
		}
		if (*lower == k) {
			return;
		}
		minivec<V>::insert(lower, k);
	}

	void erase(const key_type& k) {
		minivec<V>::erase(find(k));
	}

	size_type size() const {
		return minivec<V>::size();
	}

	size_type count(const key_type& k) const {
		return has(k) ? 1: 0;
	}
};