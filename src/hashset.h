#pragma once

#include "common.h"
#include <cassert>

// A std::unordered_set alternative using vectors for buckets

template <typename K, class H = std::hash<K>>
class hashset {

	uint entries = 0;

	struct power2prime {
		uint power2;
		uint prime;
	};

	static inline std::vector<power2prime> widths = {
		{ .power2 = 8, .prime = 7 },
		{ .power2 = 16, .prime = 13 },
		{ .power2 = 32, .prime = 31 },
		{ .power2 = 64, .prime = 61,} ,
		{ .power2 = 128, .prime = 127 },
		{ .power2 = 256, .prime = 251 },
		{ .power2 = 512, .prime = 509 },
		{ .power2 = 1024, .prime = 1021 },
		{ .power2 = 2048, .prime = 2039 },
		{ .power2 = 4096, .prime = 4093 },
		{ .power2 = 8192, .prime = 8191 },
		{ .power2 = 16384, .prime = 16381 },
		{ .power2 = 32768, .prime = 32749 },
		{ .power2 = 65536, .prime = 65521 },
		{ .power2 = 131072, .prime = 131071 },
		{ .power2 = 262144, .prime = 262139 },
		{ .power2 = 524288, .prime = 524287 },
		{ .power2 = 1048576, .prime = 1048573 },
	};

	// offset into widths; actual index size is always .prime
	uint width = 0;

	std::vector<std::vector<K>> index;

	H hash;

	std::size_t chain(const K& k) const {
		assert(index.size() > 0);
		return hash(k) % index.size();
	}

public:

	hashset() {
	}

	~hashset() {
		clear();
	}

	std::size_t memory() {
		std::size_t size = index.size() * sizeof(index[0]);
		for (auto& chain: index)
			size += chain.size() * sizeof(K);
		return size;
	}

	// buckets are vectors, so interating a bit on collision is cheap
	const float load = std::max(16.0f, 128.0f / (float)sizeof(K));

	void reindex() {
		assert(width >= 0 && width < widths.size());
		float current = (float)std::max(1u, entries)/(float)widths[width].power2;

		if (current > load && widths.size()-1 > width) {
			width++;
		}

		if (current < (load*0.5f) && width > 0) {
			width--;
		}

		if (widths[width].prime > index.size()) {
			std::vector<std::vector<K>> next(widths[width].prime);
			for (auto& bucket: index) for (auto& k: bucket) {
				next[hash(k) % next.size()].push_back(k);
			}
			std::swap(index, next);
		}
	}

	void clear() {
		index.clear();
		entries = 0;
		width = 0;
	}

	void shrink_to_fit() {
		if (!size()) index.shrink_to_fit();
	}

	std::size_t size() const {
		return entries;
	}

	class iterator {
	public:
		uint i;
		uint j;
		const hashset<K,H> *hs;

		typedef K value_type;
		typedef std::ptrdiff_t difference_type;
		typedef K* pointer;
		typedef K& reference;
		typedef std::input_iterator_tag iterator_category;

		explicit iterator(const hashset<K,H> *hhs, uint ii, uint jj = 0) {
			hs = hhs;
			i = std::min(ii, (uint)hs->index.size());
			j = jj;
		}

		const K& operator*() const {
			return hs->index[i][j];
		}

		bool operator==(const iterator& other) const {
			return i == other.i && j == other.j;
		}

		bool operator!=(const iterator& other) const {
			return i != other.i || j != other.j;
		}

		iterator& operator++() {
			if (i < hs->index.size()) {
				j++;
				while (i < hs->index.size() && j >= hs->index[i].size()) {
					i++;
					j = 0;
				}
				return *this;
			}
			i = hs->index.size();
			j = 0;
			return *this;
		}

		iterator operator++(int) {
			iterator tmp(*this);
			++*this;
			return tmp;
		}
	};

	iterator begin() const {
		for (uint i = 0; i < index.size(); i++) {
			if (index[i].size())
				return iterator(this, i);
		}
		return end();
	}

	iterator end() const {
		return iterator(this, index.size());
	}

	iterator find(const K& k) const {
		if (index.size()) {
			int i = chain(k);
			auto& bucket = index[i];
			for (uint j = 0; j < bucket.size(); j++) {
				if (bucket[j] == k)
					return iterator(this, i, j);
			}
		}
		return end();
	}

	bool has(const K& k) const {
		return find(k) != end();
	}

	bool contains(const K& k) const {
		return has(k);
	}

	uint count(const K& k) const {
		return has(k) ? 1: 0;
	}

	bool erase(iterator it) {
		if (it != end()) {
			auto& bucket = index[it.i];
			bucket.erase(bucket.begin()+it.j);
			entries--;
			reindex();
			return true;
		}
		return false;
	}

	bool erase(const K& k) {
		return erase(find(k));
	}

	bool insert(const K& k) {
		if (!index.size()) reindex();
		auto& bucket = index[chain(k)];
		for (auto& bk: bucket) {
			if (bk == k) return false;
		}
		bucket.push_back(k);
		entries++;
		reindex();
		return true;
	}

};
