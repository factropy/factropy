#include "common.h"
#include <thread>
#include "pulse.h"
#include "gtest/gtest.h"

namespace {

	pulser pulse;

	std::thread threads[8];
	std::atomic<int> ready = {0};
	std::atomic<int> count = {0};

	TEST(channel, mt) {
		for (int i = 0; i < 8; i++) {
			threads[i] = std::thread([&]() {
				ready++;
				pulse.wait();
				count++;
			});
		}

		while (ready < 8) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		EXPECT_EQ(ready, 8);

		pulse.now();

		for (int i = 0; i < 8; i++) {
			threads[i].join();
		}

		EXPECT_EQ(count, 8);
	}
}