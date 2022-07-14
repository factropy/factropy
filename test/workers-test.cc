#include "common.h"
#include "workers.h"
#include "gtest/gtest.h"

namespace {

	TEST(workers, crew) {
		std::atomic<int> count = {0};
		workers<8> crew;
		for (int i = 0; i < 8; i++) {
			crew.job([&]() {
				count++;
			});
		}
		crew.stop();
		EXPECT_EQ(count, 8);
	}
}