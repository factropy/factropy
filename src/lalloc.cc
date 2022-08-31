#include "common.h"
#include "lalloc.h"
#include <array>
#include <deque>
#include <vector>
#include <thread>

// A simple thread-local pool allocator. See localvec/localset/localmap

#define BLOCK 1024
#define POOL 1024

namespace {
	std::mutex mutex;

	struct Block {
		uint8_t data[BLOCK];
	};

	struct Pool {
		struct {
			uint64_t malloc = 0;
			uint64_t mallocHere = 0;
			uint64_t free = 0;
			uint64_t freeHere = 0;
			uint64_t realloc = 0;
			uint64_t reallocHere = 0;
		} counters;

		std::vector<size_t> queue;
		std::array<bool,POOL> used;
		std::array<Block,POOL> blocks;

		Pool() {
			for (size_t i = 0, l = POOL; i < l; i++)
				queue.push_back(i);
		}

		bool our(void* ptr) {
			auto block = (Block*)ptr;
			return block >= &blocks[0] && block <= &blocks[POOL-1];
		}

		void* malloc(size_t bytes) {
			counters.malloc++;

			if (bytes%8) {
				bytes += (8-(bytes%8));
			}

			if (bytes <= sizeof(Block) && queue.size()) {
				size_t i = queue.back();
				queue.pop_back();
				ensure(!used[i]);
				used[i] = true;
				counters.mallocHere++;
				return &blocks[i];
			}

			return std::malloc(bytes);
		}

		void free(void* ptr) {
			counters.free++;

			if (our(ptr)) {
				auto block = (Block*)ptr;
				size_t i = block - blocks.data();
				ensure(used[i]);
				used[i] = false;
				queue.push_back(i);
				counters.freeHere++;
				return;
			}
			std::free(ptr);
		}

		void* realloc(void* ptr, size_t bytes) {
			counters.realloc++;

			if (our(ptr)) {
				if (bytes <= sizeof(Block)) {
					counters.reallocHere++;
					return ptr;
				}
				void* ptr2 = std::malloc(bytes);
				std::memmove(ptr2, ptr, sizeof(Block));
				free(ptr);
				return ptr2;
			}

			return std::realloc(ptr, bytes);
		}

		bool empty() {
			return queue.size() == POOL;
		}

		void info() {
//			mutex.lock();
//			std::cout << "malloc " << counters.malloc << ' ' << counters.mallocHere << ' ';
//			std::cout << "free " << counters.free << ' ' << counters.freeHere << ' ';
//			std::cout << "realloc " << counters.realloc << ' ' << counters.reallocHere << ' ';
//			std::cout << '\n';
//			mutex.unlock();
		}
	};

	thread_local Pool pool;
}

void* lmalloc(size_t bytes) {
	return pool.malloc(bytes);
}

void lfree(void* ptr) {
	return pool.free(ptr);
}

void* lrealloc(void* ptr, size_t bytes) {
	return pool.realloc(ptr, bytes);
}

void lsanity() {
	ensuref(pool.empty(), "pool dirty");
}

void linfo() {
	pool.info();
}

