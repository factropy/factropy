#pragma once

#include <cassert>
#include <new>
#include <type_traits>
#include <typeinfo>
#include "common.h"
#include "miniset.h"

// A compact std::unordered_set alternative for small sets of trivial types

template <class V>
class miniuset : public minivec<V> {
	static_assert(std::is_trivially_copyable<V>::value, "miniuset<is_trivially_copyable>");

public:
	typedef typename minivec<V>::value_type key_type;
	typedef typename minivec<V>::size_type size_type;
	typedef typename minivec<V>::iterator iterator;

	miniuset() : minivec<V>() {
	}

	miniuset(const miniuset<V>& other) : minivec<V>(other) {
	}

	miniuset(std::span<const V> other) {
		operator=(other);
	}

	miniuset(miniuset<V>&& other) : minivec<V>(other) {
	}

	miniuset(std::initializer_list<V> l) {
		minivec<V>::clear();
		for (auto& v: l) insert(v);
	}

	miniuset<V>& operator=(const miniuset<V>& other) {
		minivec<V>::operator=(other);
		return *this;
	}

	miniuset<V>& operator=(miniuset<V>&& other) {
		minivec<V>::operator=(other);
		return *this;
	}

	miniuset<V>& operator=(std::span<const V> other) {
		minivec<V>::clear();
		for (auto& v: other) insert(v);
		return *this;
	}

	operator miniset<V>() const {
		miniset<V> out(*this);
		std::sort(out.begin(), out.end());
		return out;
	}

	iterator begin() const {
		return minivec<V>::begin();
	}

	iterator end() const {
		return minivec<V>::end();
	}

	iterator find(const key_type& k) const {
		return std::find(begin(), end(), k);
	}

	bool has(const key_type& k) const {
		return find(k) != end();
	}

	bool contains(const key_type& k) const {
		return has(k);
	}

	void insert(const key_type& k) {
		auto it = find(k);
		if (it == end()) minivec<V>::push_back(k);
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
class localuset : public localvec<V> {
	static_assert(std::is_trivially_copyable<V>::value, "localuset<is_trivially_copyable>");

public:
	typedef typename localvec<V>::value_type key_type;
	typedef typename localvec<V>::size_type size_type;
	typedef typename localvec<V>::iterator iterator;

	localuset() : localvec<V>() {
	}

	localuset(const localuset<V>& other) : localvec<V>(other) {
	}

	localuset(std::span<const V> other) {
		operator=(other);
	}

	localuset(localuset<V>&& other) : localvec<V>(other) {
	}

	localuset(std::initializer_list<V> l) {
		localvec<V>::clear();
		for (auto& v: l) insert(v);
	}

	localuset<V>& operator=(const localuset<V>& other) {
		localvec<V>::operator=(other);
		return *this;
	}

	localuset<V>& operator=(localuset<V>&& other) {
		localvec<V>::operator=(other);
		return *this;
	}

	localuset<V>& operator=(std::span<const V> other) {
		localvec<V>::clear();
		for (auto& v: other) insert(v);
		return *this;
	}

	operator localset<V>() const {
		localset<V> out(*this);
		std::sort(out.begin(), out.end());
		return out;
	}

	iterator begin() const {
		return localvec<V>::begin();
	}

	iterator end() const {
		return localvec<V>::end();
	}

	iterator find(const key_type& k) const {
		return std::find(begin(), end(), k);
	}

	bool has(const key_type& k) const {
		return find(k) != end();
	}

	bool contains(const key_type& k) const {
		return has(k);
	}

	void insert(const key_type& k) {
		auto it = find(k);
		if (it == end()) localvec<V>::push_back(k);
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
