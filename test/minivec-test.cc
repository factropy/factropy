#include "common.h"
#include "minivec.h"
#include "gtest/gtest.h"

namespace {

	minivec<int> mv;

	void populate() {
		mv.clear();
		for (uint i = 0; i < 10; i++) {
			mv.push_back(i);
		}
	}

	TEST(minivec, push) {
		populate();
		EXPECT_EQ(10u, mv.size());
		EXPECT_EQ(0, mv[0]);
		EXPECT_EQ(9, mv[9]);
	}

	TEST(minivec, pop) {
		populate();
		mv.pop_back();
		EXPECT_EQ(9u, mv.size());
		EXPECT_EQ(0, mv[0]);
		EXPECT_EQ(8, mv[mv.size()-1]);
	}

	TEST(minivec, shove) {
		populate();
		mv.push_front(100);
		EXPECT_EQ(11u, mv.size());
		EXPECT_EQ(100, mv[0]);
		EXPECT_EQ(9, mv[10]);
	}

	TEST(minivec, shift) {
		populate();
		mv.pop_front();
		EXPECT_EQ(9u, mv.size());
		EXPECT_EQ(1, mv[0]);
		EXPECT_EQ(9, mv[mv.size()-1]);
	}

	TEST(minivec, erase) {
		populate();
		auto fn = [&](auto v) { return v == 5; };
		mv.erase(std::remove_if(mv.begin(), mv.end(), fn), mv.end());

		EXPECT_EQ(9u, mv.size());
		EXPECT_EQ(4, mv[4]);
		EXPECT_EQ(6, mv[5]);
	}

	TEST(minivec, insert) {
		populate();
		mv.insert(mv.begin()+2, -1);

		EXPECT_EQ(11u, mv.size());
		EXPECT_EQ(1, mv[1]);
		EXPECT_EQ(-1, mv[2]);
		EXPECT_EQ(2, mv[3]);
		EXPECT_EQ(9, mv[10]);

		mv.insert(mv.begin()+mv.size(), 3, 1);
		EXPECT_EQ(14u, mv.size());
		EXPECT_EQ(9, mv[10]);
		EXPECT_EQ(1, mv[11]);
		EXPECT_EQ(1, mv[12]);
		EXPECT_EQ(1, mv[13]);
	}

	TEST(minivec, discard) {
		populate();
		discard_if(mv, [&](auto n) { return n%2!=0; });
		EXPECT_EQ(5u, mv.size());
		EXPECT_EQ(0, mv[0]);
		EXPECT_EQ(2, mv[1]);
		EXPECT_EQ(4, mv[2]);
		EXPECT_EQ(6, mv[3]);
		EXPECT_EQ(8, mv[4]);
	}
}