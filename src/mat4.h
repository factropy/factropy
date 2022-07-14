#pragma once

// In the days of yore Mat4 extended Raylib's Matrix to C++ land.
// In the days of SDL+glm it's just a thin layer over glm::dvec4.

// Remaining differences:
// * Multiplication order reversed
// * Rotation method arguments reversed

struct Mat4;

#include "glm-ex.h"
#include "point.h"
#include <initializer_list>

struct Mat4 {
	glm::matrix m;
	static const Mat4 identity;

	Mat4 operator*(const Mat4& o) const;

	operator glm::mat4() const {
		return m;
	};

	operator glm::dmat4() const {
		return m;
	};

	Mat4() = default;
	Mat4(const glm::dmat4& o);
	Mat4(const glm::mat4& o);
	Mat4 normalize() const;
	Mat4 invert() const;

	static Mat4 scale(real x, real y, real z);
	static Mat4 scale(real s);
	static Mat4 rotate(const Point& axis, float radians);
	static Mat4 rotateX(float radians);
	static Mat4 rotateY(float radians);
	static Mat4 rotateZ(float radians);
	static Mat4 translate(real x, real y, real z);
	static Mat4 translate(const Point& p);
};
