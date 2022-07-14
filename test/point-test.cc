#include "common.h"
#include "point.h"
#include "gtest/gtest.h"

namespace {
	TEST(point, ahead) {
		EXPECT_EQ(Point::Ahead, (Point::South*10).relative(Point::South, Point::South*11));
	}
	TEST(point, behind) {
		EXPECT_EQ(Point::Behind, (Point::South).relative(Point::South, Point::North));
	}
	TEST(point, ahead2) {
		EXPECT_EQ(Point::Ahead, (Point::East).relative(Point::East, Point::East*2));
	}
	TEST(point, behind2) {
		EXPECT_EQ(Point::Behind, (Point::West).relative(Point::West, Point::East));
	}
	TEST(point, left) {
		EXPECT_EQ(Point::Left, (Point::South).relative(Point::South, Point::East+Point::South));
	}
	TEST(point, right) {
		EXPECT_EQ(Point::Right, (Point::South).relative(Point::South, Point::West+Point::South));
	}
}
