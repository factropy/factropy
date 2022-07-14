#include "common.h"
#include "cuboid.h"
#include "gtest/gtest.h"

namespace {

	Box unitCube(Point pos) {
		return Box(pos - Point(0.5, 0.5, 0.5), pos + Point(0.5, 0.5, 0.5));
	}

	TEST(cuboid, intersect1) {
		auto a = Cuboid(Box(unitCube(Point(0.0,0.0,0.0))), Point::South);
		auto b = Cuboid(Box(unitCube(Point(0.25,0.25,0.25))), Point(1,2,3).normalize());
		EXPECT_TRUE(a.intersects(b));
	}

	TEST(cuboid, intersect2) {
		auto a = Cuboid(Box(unitCube(Point(0.0,0.0,0.0))), Point::South);
		auto b = Cuboid(Box(unitCube(Point(-0.25,0.25,0.25))), Point(1,2,3).normalize());
		EXPECT_TRUE(a.intersects(b));
	}

	TEST(cuboid, intersect3) {
		auto a = Cuboid(Box(unitCube(Point(0.0,0.0,0.0))), Point::South);
		auto b = Cuboid(Box(unitCube(Point(0.25,-0.25,0.25))), Point(1,2,3).normalize());
		EXPECT_TRUE(a.intersects(b));
	}

	TEST(cuboid, intersect4) {
		auto a = Cuboid(Box(unitCube(Point(0.0,0.0,0.0))), Point::South);
		auto b = Cuboid(Box(unitCube(Point(0.25,0.25,-0.25))), Point(1,2,3).normalize());
		EXPECT_TRUE(a.intersects(b));
	}

	TEST(cuboid, intersect5) {
		auto a = Cuboid(Box(unitCube(Point(0.0,0.0,0.0))), Point::South);
		auto b = Cuboid(Box(unitCube(Point(2.0,2.0,2.0))), Point(2,1,3).normalize());
		EXPECT_FALSE(a.intersects(b));
	}

	TEST(cuboid, intersect6) {
		auto a = Cuboid(Box(unitCube(Point(0.0,0.0,0.0))), Point::South);
		auto b = Cuboid(Box(unitCube(Point(-2.0,2.0,2.0))), Point(2,1,3).normalize());
		EXPECT_FALSE(a.intersects(b));
	}

	TEST(cuboid, intersect7) {
		auto a = Cuboid(Box(unitCube(Point(0.0,0.0,0.0))), Point::South);
		auto b = Cuboid(Box(unitCube(Point(2.0,-2.0,2.0))), Point(2,1,3).normalize());
		EXPECT_FALSE(a.intersects(b));
	}

	TEST(cuboid, intersect8) {
		auto a = Cuboid(Box(unitCube(Point(0.0,0.0,0.0))), Point::South);
		auto b = Cuboid(Box(unitCube(Point(2.0,2.0,-2.0))), Point(2,1,3).normalize());
		EXPECT_FALSE(a.intersects(b));
	}
}
