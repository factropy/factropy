#include "../src/common.h"
#include "../src/catenate.h"
#include "gtest/gtest.h"

namespace {
	TEST(catenate, concatenate1) {
		std::vector<std::string> parts = {"alpha", "beta", "gamma"};
		EXPECT_EQ("alpha,beta,gamma", concatenate(parts.begin(), parts.end(), ","));
	}

	TEST(catenate, discatenate1) {
		auto dis = discatenate("alpha,beta,gamma", ",");
		std::vector<std::string> parts = {dis.begin(), dis.end()};
		std::vector<std::string> expect = {"alpha", "beta", "gamma"};
		EXPECT_EQ(expect, parts);
	}

	TEST(catenate, discatenate2) {
		auto dis = discatenate(",alpha,beta,gamma,", ",");
		std::vector<std::string> parts = {dis.begin(), dis.end()};
		std::vector<std::string> expect = {"", "alpha", "beta", "gamma", ""};
		EXPECT_EQ(expect, parts);
	}

	TEST(catenate, discatenate3) {
		auto dis = discatenate("", ",");
		std::vector<std::string> parts = {dis.begin(), dis.end()};
		std::vector<std::string> expect = {""};
		EXPECT_EQ(expect, parts);
	}

	TEST(catenate, catenate1) {
		auto dis = discatenate("alpha,beta,gamma", ",");
		auto con = concatenate(dis.begin(), dis.end(), ";");
		EXPECT_EQ("alpha;beta;gamma", con);
	}
}