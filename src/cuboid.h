#pragma once

// Non-axis-aligned bounding boxes

struct Cuboid;

#include "common.h"
#include "mat4.h"
#include "glm-ex.h"
#include <array>

struct Cuboid {
	Box box;
	Point dir;

	Cuboid() = default;
	Cuboid(const Box& box, const Point& dir);

	Cuboid grow(real n) const;
	std::vector<Point> vertices() const;

	bool contains(const Point& point) const;
	bool intersects(const Cuboid& other) const;
	bool intersects(const Box& box) const;
	bool intersects(const Sphere& sphere) const;
	bool intersects(const Ray& ray) const;
	bool intersectsRay(const Point& pos, const Point& dir) const;
};
