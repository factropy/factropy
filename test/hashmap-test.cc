#include "common.h"
#include "hashmap.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <chrono>

namespace {

	hashmap<int,int> hm;

	TEST(hashmap, count) {
		EXPECT_EQ(hm.size(), 0U);
	}

	void populate() {
		for (int i = 0; i < 10; i++) {
			hm[i] = i*2;
		}
	};

	TEST(hashmap, insert) {
		populate();
		EXPECT_EQ(hm.size(), 10U);
	}

	TEST(hashmap, erase) {
		hm.erase(2);
		EXPECT_EQ(hm.size(), 9U);

		for (auto [k,v]: hm) {
			notef("%d, %d", k, v);
		}
	}
}
