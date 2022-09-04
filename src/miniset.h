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

	miniset() : minivec<V>() {
	}

	miniset(const miniset<V>& other) : minivec<V>(other) {
	}

	miniset(std::span<const V> other) {
		operator=(other);
	}

	miniset(miniset<V>&& other) : minivec<V>(other) {
	}

	miniset(std::initializer_list<V> l) {
		minivec<V>::clear();
		for (auto& v: l) insert(v);
	}

	miniset<V>& operator=(const miniset<V>& other) {
		minivec<V>::operator=(other);
		return *this;
	}

	miniset<V>& operator=(miniset<V>&& other) {
		minivec<V>::operator=(other);
		return *this;
	}

	miniset<V>& operator=(std::span<const V> other) {
		minivec<V>::clear();
		for (auto& v: other) insert(v);
		return *this;
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
		// extra std::sort step.
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

template <class V>
class localset : public localvec<V> {
	static_assert(std::is_trivially_copyable<V>::value, "localset<is_trivially_copyable>");

public:
	typedef typename localvec<V>::value_type key_type;
	typedef typename localvec<V>::size_type size_type;
	typedef typename localvec<V>::iterator iterator;

	localset() : localvec<V>() {
	}

	localset(const localset<V>& other) : localvec<V>(other) {
	}

	localset(std::span<const V> other) {
		operator=(other);
	}

	localset(localset<V>&& other) : localvec<V>(other) {
	}

	localset(std::initializer_list<V> l) {
		localvec<V>::clear();
		for (auto& v: l) insert(v);
	}

	localset<V>& operator=(const localset<V>& other) {
		localvec<V>::operator=(other);
		return *this;
	}

	localset<V>& operator=(localset<V>&& other) {
		localvec<V>::operator=(other);
		return *this;
	}

	localset<V>& operator=(std::span<const V> other) {
		localvec<V>::clear();
		for (auto& v: other) insert(v);
		return *this;
	}

	iterator begin() const {
		return localvec<V>::begin();
	}

	iterator end() const {
		return localvec<V>::end();
	}

	iterator find(const key_type& k) const {
		// Could use std::lower_bound for a binary search here because the
		// elements are ordered, but the idea (platform-dependent assumption)
		// behind localset is that small vectors are often fastest to simply
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
		// extra std::sort step.
		auto lower = std::lower_bound(begin(), end(), k);
		if (lower == end()) {
			localvec<V>::push_back(k);
			return;
		}
		if (*lower == k) {
			return;
		}
		localvec<V>::insert(lower, k);
	}

	void erase(const key_type& k) {
		localvec<V>::erase(find(k));
	}

	size_type size() const {
		return localvec<V>::size();
	}

	size_type count(const key_type& k) const {
		return has(k) ? 1: 0;
	}
};


