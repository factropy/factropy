#include "common.h"
#include "gridwalk.h"
#include "gtest/gtest.h"
#include <vector>

namespace {

	Box box = {0,0,0,50,50,50};

	std::vector<gridwalk::xy> expect;
	std::vector<gridwalk::xy> result;

	TEST(gridwalk, a) {
		auto walk = gridwalk(100,box);
		expect = {{-1,-1}, {0,-1}, {-1,0}, {0,0}};
		result = {walk.begin(), walk.end()};
		EXPECT_EQ(expect, result);
	}

	TEST(gridwalk, b) {
		auto walk = gridwalk(10,box);
		expect = {{-3,-3},{-2,-3},{-1,-3},{0,-3},{1,-3},{2,-3},{-3,-2},{-2,-2},{-1,-2},{0,-2},{1,-2},{2,-2},{-3,-1},{-2,-1},{-1,-1},{0,-1},{1,-1},{2,-1},{-3,0},{-2,0},{-1,0},{0,0},{1,0},{2,0},{-3,1},{-2,1},{-1,1},{0,1},{1,1},{2,1},{-3,2},{-2,2},{-1,2},{0,2},{1,2},{2,2}};
		result = {walk.begin(), walk.end()};
		EXPECT_EQ(expect, result);
	}
}