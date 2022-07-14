#include "common.h"
#include "miniqueue.h"
#include "gtest/gtest.h"

namespace {

	miniqueue<int> mq;

	TEST(miniqueue, enqueue) {
		EXPECT_TRUE(mq.empty());
		EXPECT_EQ(0u, mq.size());
		mq.enqueue(1);
		EXPECT_EQ(1u, mq.size());
	}

	TEST(miniqueue, dequeue) {
		EXPECT_FALSE(mq.empty());
		EXPECT_EQ(mq.dequeue(), 1);
		EXPECT_EQ(0u, mq.size());
	}
}