#include "common.h"
#include "minimap.h"
#include "gtest/gtest.h"

namespace {

	struct item {
		uint id = 0;
		uint val = 0;
	};

	minimap<item,&item::id> mm;

	void populate() {
		mm.clear();
		for (uint i = 0; i < 10; i++) {
			mm.insert((item){.id = i, .val = i*2});
		}
	}

	TEST(minimap, insert) {
		populate();
		EXPECT_EQ(10u, mm.size());
	}

	TEST(minimap, erase) {
		populate();
		for (uint i = 0; i < 5; i++) {
			mm.erase(i*2);
		}
		EXPECT_EQ(5u, mm.size());
	}
}