#pragma once

struct Frustum;

#include "common.h"
#include "mat4.h"
#include "glm-ex.h"
#include <array>

struct Frustum {

	static inline int Back = 0;
	static inline int Front = 1;
	static inline int Bottom = 2;
	static inline int Top = 3;
	static inline int Right = 4;
	static inline int Left = 5;

	std::array<glm::dvec4,6> planes;

	Frustum();
	Frustum(const Mat4& cam);

	bool contains(const Point& point) const;
	bool intersects(const Sphere& sphere) const;
	bool intersects(const Box& box) const;
};