#include "common.h"
#include "hashset.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <chrono>
#include <unordered_set>

namespace {

//	double bench(std::function<void(void)> fn) {
//		auto start = std::chrono::steady_clock::now();
//		fn();
//		auto finish = std::chrono::steady_clock::now();
//		return std::chrono::duration<double,std::milli>(finish-start).count();
//	}

	hashset<int> hs;

	void populate() {
		hs.clear();
		for (uint i = 0; i < 10; i++) {
			hs.insert(i);
		}
	}

	TEST(hashset, insert) {
		populate();
		EXPECT_EQ(10u, hs.size());
	}

	TEST(hashset, erase) {
		populate();

		for (uint i = 0; i < 5; i++) {
			hs.erase(i*2);
		}

		EXPECT_EQ(5u, hs.size());
		EXPECT_TRUE(hs.erase(*hs.begin()));
		EXPECT_EQ(4u, hs.size());
		EXPECT_TRUE(hs.erase(*hs.begin()));
		EXPECT_EQ(3u, hs.size());
		EXPECT_TRUE(hs.erase(*hs.begin()));
		EXPECT_EQ(2u, hs.size());
		EXPECT_TRUE(hs.erase(*hs.begin()));
		EXPECT_EQ(1u, hs.size());
		EXPECT_TRUE(hs.erase(*hs.begin()));
		EXPECT_EQ(0u, hs.size());
	}

/*	TEST(hashset, bench) {
		std::unordered_set<int> a;
		hashset<int> b;

		std::printf("unordered_set insert %0.1f\n", bench([&]() {
			for (int i = 0; i < 10000000; i++) {
				a.insert(i);
			}
		}));

		std::printf("hashset insert %0.1f\n", bench([&]() {
			for (int i = 0; i < 10000000; i++) {
				b.insert(i);
			}
		}));

		std::printf("unordered_set iterate %0.1f\n", bench([&]() {
			for (int i = 0; i < 100; i++) {
				int t = 0;
				for (auto n: a) {
					t += n;
				}
			}
		}));

		std::printf("hashset iterate %0.1f\n", bench([&]() {
			for (int i = 0; i < 100; i++) {
				int t = 0;
				for (auto n: b) {
					t += n;
				}
			}
		}));

		std::printf("unordered_set lookup %0.1f\n", bench([&]() {
			for (int i = 0; i < 10000000; i++) {
				a.count(i);
			}
		}));

		std::printf("hashset lookup %0.1f\n", bench([&]() {
			for (int i = 0; i < 10000000; i++) {
				b.count(i);
			}
		}));

		std::printf("unordered_set erase %0.1f\n", bench([&]() {
			for (int i = 0; i < 10000000; i++) {
				a.erase(i);
			}
		}));

		std::printf("hashset erase %0.1f\n", bench([&]() {
			for (int i = 0; i < 10000000; i++) {
				b.erase(i);
			}
		}));
	}*/
}