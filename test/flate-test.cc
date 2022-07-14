#include "../src/common.h"
#include "../src/flate.cc"
#include "gtest/gtest.h"

namespace {
	TEST(flate, a) {
		std::vector<std::string> expect = {"alpha","beta","gamma","delta"};
		deflation out;
		for (auto p: expect) out.push(p);
		out.save("/tmp/flate.test");
		inflation in;
		in.load("/tmp/flate.test");
		auto it = in.parts();
		std::vector<std::string> parts = {it.begin(), it.end()};
		EXPECT_EQ(expect, parts);
	}
}