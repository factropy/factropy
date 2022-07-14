#include "common.h"
#include "miniset.h"
#include "gtest/gtest.h"

namespace {

	miniset<int> mm;

	void populate() {
		mm.clear();
		for (int i = 9; i >= 0; --i) {
			mm.insert(i);
		}
	}

	TEST(miniset, insert) {
		populate();
		EXPECT_EQ(10u, mm.size());
		EXPECT_EQ(0, *(mm.begin()+0));
		EXPECT_EQ(9, *(mm.begin()+9));
	}

	TEST(miniset, erase) {
		populate();

		for (uint i = 0; i < 5; i++) {
			mm.erase(i*2);
		}

		EXPECT_EQ(5u, mm.size());

		EXPECT_TRUE(mm.has(1));
		EXPECT_FALSE(mm.has(2));

		EXPECT_EQ(mm.find(1), mm.begin());
		EXPECT_NE(mm.find(1), mm.end());

		while (mm.size()) {
			mm.erase(*mm.begin());
		}

		EXPECT_EQ(0u, mm.size());
	}
}