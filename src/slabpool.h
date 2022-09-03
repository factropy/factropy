#pragma once

#include <vector>
#include <functional>
#include <cassert>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <bitset>
#include "common.h"

// An object pool using slab allocation

template <class V, uint slabSize = 1024>
class slabpool {
public:

	typedef uint size_type;

	size_type entries = 0;

	class slabpage {

		bool flags[slabSize];
		char buffer[sizeof(V) * slabSize];

		V& cell(uint i) const {
			// because buffer is really a const char*
			const char* p = buffer + (sizeof(V) * i);
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
			clear();
		}

		bool used(uint i) const {
			assert(i < slabSize);
			return flags[i];
		}

		void rawUse(uint i) {
			assert(!used(i));
			flags[i] = true;
			std::memset((void*)&cell(i), 0, sizeof(V));
		}

		void use(uint i) {
			rawUse(i);
			new (&cell(i)) V();
		}

		uint cellOf(const V* ptr) const {
			const char* p = reinterpret_cast<const char*>(ptr);
			V* a = &cell(0);
			V* b = &cell(slabSize-1);
			return ptr >= a && ptr <= b ? (p-buffer)/sizeof(V): slabSize;
		}

		void drop(uint i) {
			assert(used(i));
			flags[i] = false;
			std::destroy_at(&cell(i));
		}

		void clear() {
			for (uint i = 0; i < slabSize; i++) {
				if (flags[i]) drop(i);
			}
		}

		V& refer(uint i) const {
			assert(used(i));
			return cell(i);
		}
	};

	std::vector<slabpage*> slabs;

	struct slabslot {
		uint slab;
		uint cell;

		slabslot() {
			slab = 0;
			cell = 0;
		}

		slabslot(uint s, uint c) {
			slab = s;
			cell = c;
		}
	};

	std::vector<slabslot> queue;

	std::size_t memory() {
		return (slabs.size() * sizeof(slabpage))
			+ (queue.size() * sizeof(slabslot));
	}

	slabpool<V,slabSize>() {
	}

	~slabpool<V,slabSize>() {
		clear();
		shrink_to_fit();
	}

	void clear() {
		entries = 0;
		queue.clear();
		for (uint s = 0; s < slabs.size(); s++) {
			slabs[s]->clear();
			for (int i = slabSize-1; i >= 0; i--) {
				queue.push_back(slabslot(s, (uint)i));
			}
		}
	}

	void shrink_to_fit() {
		if (!size()) {
			for (auto slab: slabs) delete(slab);
			slabs.clear();
			queue.clear();
		}
	}

	bool empty() const {
		return !entries;
	}

	size_type size() const {
		return entries;
	}

	slabslot requestSlotRaw() {
		if (!queue.size()) {
			slabs.push_back(new slabpage());
			for (int i = slabSize-1; i >= 0; i--) {
				queue.push_back(slabslot(slabs.size()-1, (uint)i));
			}
		}

		slabslot next = queue.back();
		queue.pop_back();

		slabs[next.slab]->rawUse(next.cell);
		entries++;

		return next;
	}

	slabslot requestSlot() {
		slabslot slot = requestSlotRaw();
		V& item = referSlot(slot);
		new (&item) V();
		return slot;
	}

	void releaseSlot(slabslot slot) {
		slabpage* slab = slabs[slot.slab];
		ensure(slab->used(slot.cell));
		slab->drop(slot.cell);
		assert(entries > 0);
		entries--;
		queue.push_back(slot);
	}

	V& referSlot(slabslot slot) const {
		return slabs[slot.slab]->refer(slot.cell);
	}

	V& request() {
		return referSlot(requestSlot());
	}

	template <class... Args>
	V& emplace(Args&&... args) {
		V& item = referSlot(requestSlotRaw());
		new (&item) V(std::forward<Args>(args)...);
		return item;
	}

	void release(const V* v) {
		for (uint i = 0; i < slabs.size(); i++) {
			uint cell = slabs[i]->cellOf(v);
			if (cell < slabSize) {
				releaseSlot(slabslot(i, cell));
				break;
			}
		}
		ensure(0);
	}

	void release(const V& r) {
		release(&r);
	}

	class iterator {
		uint si;
		uint ci;
		bool end;
		slabpool<V,slabSize> *sm;

	public:
		typedef V value_type;
		typedef std::ptrdiff_t difference_type;
		typedef V* pointer;
		typedef V& reference;
		typedef std::input_iterator_tag iterator_category;

		explicit iterator(slabpool<V,slabSize> *ssm, uint ssi, uint cci, bool eend) {
			sm = ssm;
			si = ssi;
			ci = cci;
			end = eend;
		}

		slabslot slot() const {
			return slabslot(si, ci);
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
