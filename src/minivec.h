#pragma once

#include <cassert>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <initializer_list>
#include <span>
#include "common.h"
#include "lalloc.h"

// A single-pointer std::vector alternative for trivial types.

struct minialloc {
	virtual void* malloc(size_t bytes) = 0;
	virtual void* realloc(void* ptr, size_t bytes) = 0;
	virtual void free(void* ptr) = 0;
};

struct minialloc_def : minialloc {
	void* malloc(size_t bytes) {
		return std::malloc(bytes);
	}
	void* realloc(void* ptr, size_t bytes) {
		return std::realloc(ptr, bytes);
	}
	void free(void* ptr) {
		return std::free(ptr);
	}
};

struct minialloc_local : minialloc {
	void* malloc(size_t bytes) {
		return lmalloc(bytes);
	}
	void* realloc(void* ptr, size_t bytes) {
		return lrealloc(ptr, bytes);
	}
	void free(void* ptr) {
		return lfree(ptr);
	}
};

template <class V, class A = minialloc_def>
class minivec {
	static_assert(std::is_trivially_copyable<V>::value, "minivec<is_trivially_copyable>");

	struct header {
		uint top;
		uint cap;
		V array[];
	};

	header* head = nullptr;

public:
	uint size() const {
		return head ? head->top: 0;
	}

	uint capacity() const {
		return head ? head->cap: 0;
	}

	uint max_size() const {
		return std::numeric_limits<uint>::max();
	}

protected:
	void setSize(int n) {
		if (!head && !n) return;
		assert(head);
		head->top = n;
	}

	void setCapacity(int n) {
		if (!head && !n) return;
		assert(head);
		head->cap = n;
	}

	V* cell(uint i) const {
		return &head->array[i];
	}

public:
	typedef uint size_type;
	typedef V value_type;

	minivec<V,A>() {
	}

	minivec<V,A>(const minivec<V,A>& other) : minivec<V,A>() {
		operator=(other);
	}

	minivec<V,A>(minivec<V,A>&& other) : minivec<V,A>() {
		operator=(other);
	}

	minivec<V,A>(std::initializer_list<V> l) : minivec<V,A>() {
		clear();
		for (auto v: l) {
			push_back(v);
		}
	}

	minivec<V,A>(size_type n) : minivec<V,A>() {
		clear();
		resize(n);
	}

	template <typename IT>
	minivec<V,A>(IT a, IT b) {
		clear();
		while (a != b) {
			push_back(*a++);
		}
	}

	minivec<V,A>& operator=(const minivec<V,A>& other) {
		clear();
		append(other);
		return *this;
	}

	minivec<V,A>& operator=(minivec<V,A>&& other) {
		clear();
		A().free(head);
		head = other.head;
		other.head = nullptr;
		return *this;
	}

	operator std::span<const V>() const {
		return std::span<V>(data(), size());
	}

	virtual ~minivec<V,A>() {
		clear();
		A().free(head);
		head = nullptr;
	}

	std::size_t memory() {
		return sizeof(header) + (capacity() * sizeof(V));
	}

	void clear() {
		if (head) {
			for (uint i = 0; i < size(); i++) {
				std::destroy_at(cell(i));
			}
			setSize(0);
		}
	}

	void reserve(uint n) {
		if (!head && n > 0) {
			n = std::max(4U, n);
			head = (header*)A().malloc(sizeof(header) + (n * sizeof(V)));
			setSize(0);
			setCapacity(n);
		}
		else
		if (head && n > capacity()) {
			setCapacity(std::max(4U, capacity()));
			while (n > capacity()) setCapacity(capacity()*2);
			head = (header*)A().realloc((void*)head, sizeof(header) + (capacity() * sizeof(V)));
		}
	}

	void shrink_to_fit() {
		if (!size()) {
			A().free(head);
			head = nullptr;
		}
	}

	void resize(uint n) {
		reserve(n);
		while (size() < n) {
			*cell(size()) = V();
			setSize(size()+1);
		}
	}

	V& at(uint i) const {
		assert(capacity() > i);
		return *cell(i);
	}

	V& operator[](uint i) const {
		return at(i);
	}

	V& front() const {
		return at(0);
	}

	V& back() const {
		return at(size()-1);
	}

	V* data() const {
		return head ? cell(0): nullptr;
	}

	bool empty() const {
		return size() == 0;
	}

	void push_back(V s = V()) {
		uint n = size();
		reserve(n+1);
		*cell(n) = s;
		setSize(n+1);
	}

	void pop_back() {
		assert(size());
		setSize(size()-1);
		std::destroy_at(cell(size()));
	}

	void push_front(V s = V()) {
		uint n = size();
		reserve(n+1);
		std::memmove((void*)cell(1), (void*)cell(0), n * sizeof(V));
		*cell(0) = s;
		setSize(n+1);
	}

	void pop_front() {
		assert(size());
		erase(begin());
	}

	void push(V v = V()) {
		push_back(v);
	}

	V pop() {
		V v = back();
		pop_back();
		return v;
	}

	void shove(V v = V()) {
		push_front(v);
	}

	V shift() {
		V v = front();
		pop_front();
		return v;
	}

	bool has(V v) {
		return std::find(begin(), end(), v) != end();
	}

	bool operator==(const minivec<V,A>& other) {
		if (size() != other.size()) return false;
		for (uint i = 0; i < size(); i++) if (at(i) != other[i]) return false;
		return true;
	}

	bool operator!=(const minivec<V,A>& other) {
		return !operator==(other);
	}

	class iterator {
	public:
		uint ii;
		const minivec* mv;

		typedef V value_type;
		typedef size_type difference_type;
		typedef V* pointer;
		typedef V& reference;
		typedef std::input_iterator_tag iterator_category;

		explicit iterator(const minivec* mmv, uint iii) {
			mv = mmv;
			ii = std::min(mv->size(), iii);
		}

		bool valid() const {
			return ii >= 0 && ii <= mv->size();
		}

		V& operator*() const {
			assert(valid());
			return *mv->cell(ii);
		}

		V* operator->() const {
			assert(valid());
			return mv->cell(ii);
		}

		bool operator==(const iterator& other) const {
			assert(valid() && other.valid());
			return ii == other.ii;
		}

		bool operator!=(const iterator& other) const {
			assert(valid() && other.valid());
			return ii != other.ii;
		}

		bool operator<(const iterator& other) const {
			assert(valid() && other.valid());
			return ii < other.ii;
		}

		bool operator>(const iterator& other) const {
			assert(valid() && other.valid());
			return ii > other.ii;
		}

		iterator& operator+=(difference_type d) {
			ii += d;
			ii = std::min(mv->size(), ii);
			assert(valid());
			return *this;
		}

		iterator operator+(const iterator& other) const {
			return iterator(mv, ii+other.ii);
		}

		iterator operator+(difference_type d) const {
			return iterator(mv, ii+d);
		}

		iterator& operator-=(difference_type d) {
			ii = d > ii ? 0: ii-d;
			assert(valid());
			return *this;
		}

		iterator operator-(const iterator& other) const {
			return operator-(other.ii);
		}

		iterator operator-(difference_type d) const {
			return iterator(mv, d > ii ? 0: ii-d);
		}

		difference_type operator-(const iterator& other) {
			return ii - other.ii;
		}

		iterator& operator++() {
			ii = ii < mv->size() ? ii+1: mv->size();
			return *this;
		}

		iterator operator++(int) {
			iterator tmp(*this);
			++*this;
			return tmp;
		};

		iterator& operator--() {
			--ii;
			return *this;
		}

		iterator operator--(int) {
			iterator tmp(*this);
			--*this;
			return tmp;
		};
	};

	iterator begin() const {
		return iterator(this, 0);
	}

	iterator end() const {
		return iterator(this, size());
	}

	iterator erase(iterator it, iterator ie) {

		uint low = it.ii;
		uint high = ie.ii;

		if (low > high) {
			high = it.ii;
			low = ie.ii;
		}

		if (high > low && low < size() && high <= size()) {

			for (uint i = low; i < high; i++) {
				std::destroy_at(cell(i));
			}

			uint moveDown = (size() - low - 1) * sizeof(V);
			std::memmove((void*)cell(low), (void*)cell(high), moveDown);

			setSize(size()-(high-low));
		}
		return it;
	}

	iterator erase(iterator it) {
		return erase(it, iterator(this, it.ii+1));
	}

	iterator erase(uint i) {
		return erase(iterator(this, i));
	}

	iterator insert(iterator it, uint n, const V& v) {
		reserve(size()+n);
		uint low = it.ii;
		uint high = low+n;
		uint moveUp = (size() - low) * sizeof(V);
		std::memmove((void*)cell(high), (void*)cell(low), moveUp);
		for (uint i = low; i < high; i++) {
			*cell(i) = v;
		}
		setSize(size()+n);
		return it;
	}

	iterator insert(iterator it, const V& v) {
		return insert(it, 1, v);
	}

	void append(const V* from, size_t items) {
		uint a = items;
		if (!a) return;
		uint s = size();
		uint b = a*sizeof(V);
		reserve(s+a);
		std::memmove((void*)cell(s), (void*)from, b);
		setSize(s+a);
	}

	void append(const minivec<V,A>& other) {
		append(other.data(), other.size());
	}

	void append(std::span<const V> items) {
		append(items.data(), items.size());
	}
};

template <class V>
class localvec : public minivec<V,minialloc_local> {
public:
	localvec<V>() {
	}

	localvec<V>(const localvec<V>& other) : localvec<V>() {
		operator=(other);
	}

	localvec<V>(localvec<V>&& other) : localvec<V>() {
		operator=(other);
	}

	localvec<V>(std::initializer_list<V> l) : localvec<V>() {
		minivec<V,minialloc_local>::clear();
		for (auto v: l) {
			minivec<V,minialloc_local>::push_back(v);
		}
	}

	localvec<V>(uint n) : localvec<V>() {
		minivec<V,minialloc_local>::clear();
		minivec<V,minialloc_local>::resize(n);
	}

	template <typename IT>
	localvec<V>(IT a, IT b) {
		minivec<V,minialloc_local>::clear();
		while (a != b) {
			minivec<V,minialloc_local>::push_back(*a++);
		}
	}

	localvec<V>& operator=(const localvec<V>& other) {
		minivec<V,minialloc_local>::operator=(other);
		return *this;
	}

	localvec<V>& operator=(localvec<V>&& other) {
		minivec<V,minialloc_local>::operator=(other);
		return *this;
	}

	operator std::span<const V>() const {
		return std::span<V>(minivec<V,minialloc_local>::data(), minivec<V,minialloc_local>::size());
	}
};
