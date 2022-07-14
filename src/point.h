#pragma once

// In the days of yore Point extended Raylib's Vector3 to C++ land.
// In the days of SDL+glm it acts a lot like a glm::dvec3, but still
// does a bunch of custom stuff.

// Heads-up:
// * the fuzzy equality operator
// * the std::hash injection
// * .rotation() should probably be in Mat4

#include "common.h"

struct Point;
typedef double real;
#define realfmt "%lf"
#define realfmt3 realfmt "," realfmt "," realfmt

#include "glm-ex.h"
#include "box.h"
#include "sphere.h"
#include "mat4.h"
#include "volume.h"
#include <initializer_list>

struct Point {
	real x = 0.0f;
	real y = 0.0f;
	real z = 0.0f;

	static const Point Zero;
	static const Point North;
	static const Point South;
	static const Point East;
	static const Point West;
	static const Point Up;
	static const Point Down;

	enum Relative {
		Ahead = 0,
		Behind,
		Left,
		Right,
	};

	bool operator==(const Point& o) const;
	bool operator!=(const Point& o) const;
	bool operator<(const Point& o) const;
	Point operator+(const Point& o) const;
	Point operator+(real n) const;
	Point operator-(const Point& o) const;
	Point operator-(real n) const;
	Point operator-() const;
	Point operator*(const Point& o) const;
	Point operator*(real s) const;
	void operator+=(const Point& o);
	void operator-=(const Point& o);

	operator const std::string() const {
		return fmt("{x:%0.3f,y:%0.3f,z:%0.3f}", x, y, z);
	};

	operator glm::point() const {
		return glm::point(x,y,z);
	};

	operator glm::vec3() const {
		return glm::vec3(x,y,z);
	};

	Point() = default;
	Point(std::initializer_list<real>);
	Point(glm::point);
	Point(glm::vec3);
	Point(Volume);
	Point(real xx, real yy, real zz);

	bool valid() const;
	Box box() const;
	Sphere sphere() const;
	real distanceSquared(const Point& p) const;
	real distance(const Point& p) const;
	real degrees(const Point& p) const;
	real radians(const Point& p) const;
	Point round() const;
	Point tileCentroid() const;
	Point floor(real fy = 0.0) const;
	real lineDistance(const Point& a, const Point& b) const;
	Point normalize() const;
	Point cross(const Point&) const;
	real dot(const Point&) const;
	Point scale(real) const;
	Point pivot(const Point& target, real speed) const;
	Point roundCardinal() const;
	Point oppositeCardinal() const;
	Point rotateHorizontal() const;
	Point randomHorizontal() const;
	bool cardinalParallel(const Point&) const;
	bool isCardinal() const;
	Point transform(const Mat4& trx) const;
	real length() const;
	real lengthSquared() const;
	Mat4 rotation() const;
	Mat4 rotationAltAz() const;
	Mat4 translation() const;
	Point nearestPointOnLine(const Point& l0, const Point& l1) const;
	static bool linesCrossOnGround(const Point& a0, const Point& a1, const Point& b0, const Point& b1);
	Relative relative(const Point& dir, const Point& other) const;
};

namespace std {
	template<> struct hash<Point> {
		std::size_t operator()(const Point& p) const noexcept {
			return std::hash<real>{}(p.distanceSquared(Point::Zero));
		}
	};
}
