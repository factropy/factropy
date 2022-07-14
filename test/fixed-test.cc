#include "common.h"
#include "fixed.h"
#include "gtest/gtest.h"

namespace {

	TEST(fixed, val) {
		fixed f = 1;
		EXPECT_EQ(1*fixed::scale, f.value());
	}

	TEST(fixed, int) {
		fixed f = 1;
		EXPECT_EQ(1, int(f));
	}

	TEST(fixed, float) {
		fixed f = 1;
		EXPECT_EQ(1.0f, float(f));
	}

	TEST(fixed, double) {
		fixed f = 1;
		EXPECT_EQ(1.0f, double(f));
	}

	TEST(fixed, assign) {
		fixed f = 1;
		f = 2;
		EXPECT_EQ(2*fixed::scale, f.value());
	}

	TEST(fixed, passign) {
		fixed f = 1;
		f += 2;
		EXPECT_EQ(3*fixed::scale, f.value());
	}

	TEST(fixed, massign) {
		fixed f = 1;
		f -= 2;
		EXPECT_EQ(-1*fixed::scale, f.value());
	}

	TEST(fixed, add) {
		fixed a = 1;
		fixed b = 2;
		fixed c = a+b;
		EXPECT_EQ(3*fixed::scale, c.value());
	}

	TEST(fixed, sub) {
		fixed a = 1;
		fixed b = 2;
		fixed c = a-b;
		EXPECT_EQ(-1*fixed::scale, c.value());
	}

	TEST(fixed, mul) {
		fixed a = 1;
		fixed b = 2;
		fixed c = a*b;
		EXPECT_EQ(2*fixed::scale, c.value());
	}

	TEST(fixed, div) {
		fixed a = 1;
		fixed b = 2;
		fixed c = a/b;
		EXPECT_EQ(fixed::scale/2, c.value());
	}

	TEST(fixed, mod) {
		fixed a = 1;
		fixed b = 2;
		fixed c = a%b;
		EXPECT_EQ(1*fixed::scale, c.value());
	}

	TEST(fixed, eq) {
		EXPECT_TRUE(fixed(1) == 1);
	}

	TEST(fixed, ne) {
		EXPECT_TRUE(fixed(2) != 1);
	}

	TEST(fixed, lt) {
		EXPECT_TRUE(fixed(-1) < 1);
	}

	TEST(fixed, gt) {
		EXPECT_TRUE(fixed(1) > -1);
	}

	TEST(fixed, lte) {
		EXPECT_TRUE(fixed(1) <= 1);
	}

	TEST(fixed, gte) {
		EXPECT_TRUE(fixed(1) >= 1);
	}

	TEST(fixed, max) {
		EXPECT_EQ(std::max(fixed(2), fixed(1)), 2);
	}

//	TEST(fixed, sin) {
//		const double pi = std::acos(-1);
//		EXPECT_TRUE(std::sin(fixed(pi/6)) == 0.499999);
//	}
}
