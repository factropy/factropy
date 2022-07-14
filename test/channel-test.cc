#include "common.h"
#include <thread>
#include "channel.h"
#include "gtest/gtest.h"

namespace {

	channel<int,1> ch;

	TEST(channel, io) {
		ch.send(1);
		EXPECT_EQ(ch.recv(), 1);
	}

	channel<int> job;
	channel<int> res;

	std::thread threads[8];

	TEST(channel, mt) {
		for (int i = 0; i < 8; i++) {
			threads[i] = std::thread([&]() {
				res.send(job.recv());
			});
		}

		for (int i = 0; i < 8; i++) {
			job.send(i);
		}

		std::set<int> results;

		for (int i = 0; i < 8; i++) {
			results.insert(res.recv());
		}

		for (int i = 0; i < 8; i++) {
			EXPECT_EQ(results.count(i), 1u);
		}

		for (int i = 0; i < 8; i++) {
			threads[i].join();
		}
	}
}