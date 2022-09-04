#pragma once

#include <vector>
#include <cassert>
#include <new>
#include "common.h"

template <class V, uint slabSize = 1024>
class slabarray {
public:

	typedef std::size_t K;

	class slabpage {

		#define vsize (((sizeof(V) + alignof(V) - 1) / alignof(V)) * alignof(V))

		bool flags[slabSize];

		// prevents C++ automatically calling V destructors
		char buffer[vsize * slabSize];

		V& cell(uint i) const {
			// because buffer is really a const char*
			const char* p = buffer + (vsize * i);
			// remove the const qualifier
			return *const_cast<V*>(reinterpret_cast<const V*>(p));
		}

	public:
		slabpage() {
			for (uint i = 0; i < slabSize; i++) {
				flags[i] = false;
			}
		}

		~slabpage() {
			for (uint i = 0; i < slabSize; i++) {
				if (flags[i]) drop(i);
			}
		}

		bool used(uint i) const {
			assert(i < slabSize);
			return flags[i];
		}

		V& use(uint i) {
			assert(i < slabSize);
			if (!flags[i]) {
				flags[i] = true;
				std::memset((void*)&cell(i), 0, vsize);
				new (&cell(i)) V();
			}
			return cell(i);
		}

		void drop(uint i) {
			assert(i < slabSize);
			if (flags[i]) {
				flags[i] = false;
				std::destroy_at(&cell(i));
			}
		}

		V& refer(uint i) {
			assert(used(i));
			return cell(i);
		}
	};

	std::vector<slabpage*> slabs;

	slabarray() {
	}

	~slabarray() {
		clear();
	}

	void clear() {
		for (auto& slab: slabs) delete(slab);
		slabs.clear();
	}

	void at(const K& k, K* slab, K* slot) {
		if (k < slabSize) {
			*slab = 0;
			*slot = k;
		}
		else {
			*slab = k/slabSize;
			*slot = k%slabSize;
		}
	}

	bool has(const K& k) {
		K slab, slot; at(k, &slab, &slot);

		if (slabs.size() > slab) {
			return slabs[slab]->used(slot);
		}

		return false;
	}

	V& refer(const K& k) {
		K slab, slot; at(k, &slab, &slot);
		ensure(slabs.size() > slab);
		return slabs[slab]->refer(slot);
	}

	V& operator[](const K& k) {
		K slab, slot; at(k, &slab, &slot);

		while (slabs.size() <= slab) {
			slabs.push_back(new slabpage());
		}

		return slabs[slab]->use(slot);
	}

	void erase(const K& k) {
		K slab, slot; at(k, &slab, &slot);

		if (slabs.size() > slab) {
			slabs[slab]->drop(slot);
		}
	}

	class iterator {
		uint si;
		uint ci;
		bool end;
		slabarray<V,slabSize> *sm;

	public:
		typedef V value_type;
		typedef std::ptrdiff_t difference_type;
		typedef V* pointer;
		typedef V& reference;
		typedef std::input_iterator_tag iterator_category;

		explicit iterator(slabarray<V,slabSize> *ssm, uint ssi, uint cci, bool eend) {
			sm = ssm;
			si = ssi;
			ci = cci;
			end = eend;
		}

		V& operator*() const {
			return sm->slabs[si]->refer(ci);
		}

		bool operator==(const iterator& other) const {
			if (end && !other.end) return false;
			if (other.end && !end) return false;
			if (end && other.end) return true;
			return si == other.si && ci == other.ci;
		}

		bool operator!=(const iterator& other) const {
			if (end && !other.end) return true;
			if (other.end && !end) return true;
			if (end && other.end) return false;
			return si != other.si || ci != other.ci;
		}

		iterator& operator++() {
			while (!end) {
				ci++;
				if (ci == slabSize) {
					ci = 0;
					si++;
				}
				if (sm->slabs.size() <= si) {
					end = true;
					break;
				}
				if (sm->slabs[si]->used(ci)) {
					break;
				}
			}
			return *this;
		}

		iterator operator++(int) {
			iterator tmp(*this);
			++*this;
			return tmp;
		};
	};

	iterator begin() {
		uint si = 0;
		uint ci = 0;
		bool end = false;
		for (;;) {
			if (ci == slabSize) {
				ci = 0;
				si++;
			}
			if (slabs.size() <= si) {
				end = true;
				break;
			}
			if (slabs[si]->used(ci)) {
				break;
			}
			ci++;
		}
		return iterator(this, si, ci, end);
	}

	iterator end() {
		return iterator(this, 0, 0, true);
	}
};
