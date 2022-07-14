#include "rail.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/spline.hpp>

namespace {
	struct Segment {
		Point a = Point::Zero;
		Point b = Point::Zero;

		Point dir() {
			return (b-a).normalize();
		};

		double length() {
			return (b-a).length();
		};
	};
}

Rail::Rail() {
	clear();
}

Rail::Rail(Point posA, Point dirA, Point posB, Point dirB) {
	real dist = (posB-posA).length()/3.0;

	p0 = posA + dirA;
	p1 = posA + dirA*dist;
	p2 = posB - dirB*dist;
	p3 = posB - dirB;
}

Point Rail::point(real t)
{
	Point tmp[4] = {p0,p1,p2,p3};
	for (int i = 3; i > 0; --i) {
		for (int k = 0; k < i; k++)
			tmp[k] = tmp[k] + ((tmp[k+1] - tmp[k]) * t);
	}
	return tmp[0];
}

Point Rail::origin() {
	return p0;
}

Point Rail::target() {
	return p3;
}

void Rail::clear() {
	p0 = Point::Zero;
	p1 = Point::Zero;
	p2 = Point::Zero;
	p3 = Point::Zero;
}

bool Rail::empty() {
	return origin() == Point::Zero && target() == Point::Zero;
}

bool Rail::valid(real limit, real degrees, real* length) {
	if (empty()) return false;

	double sum = 0;
	bool def = true;

	Segment last = {point(0.0), point(0.01)};

	for (int i = 2; i < 100; i++) {
		real t = (real)i/100.0;
		Segment next = {last.b, point(t)};
		sum += next.length();

		auto angle = next.dir().degrees(last.dir());

		if (angle > degrees || angle < -degrees) def = false;
		if (!def && !length) return false;

		last = next;
	}

	if (length) *length = sum;
	return def && sum < (limit+0.000001);
}

real Rail::length() {
	real len = 0.0;
	valid(1000000,90,&len);
	return len;
}

minivec<Point> Rail::steps(real step) {
	minivec<Point> steps;

	real len = length();
	real inc = 1.0/(len/step);

	steps.push(point(0.0));

	for (int i = 1, l = (int)std::floor(len/step); i < l-1; i++) {
		steps.push(point((real)i*inc));
	}

	steps.push(point(1.0));

	return steps;
}

bool Rail::straight() {
	auto dir1 = (p1-p0).normalize();
	auto dir2 = (p2-p0).normalize();
	auto dir3 = (p3-p0).normalize();
	return dir1 == dir2 && dir2 == dir3;
}

bool Rail::operator==(const Rail& o) const {
	bool p0eq = p0 == o.p0;
	bool p1eq = p1 == o.p1;
	bool p2eq = p2 == o.p2;
	bool p3eq = p3 == o.p3;
	return p0eq && p1eq && p2eq && p3eq;
}

bool Rail::operator!=(const Rail& o) const {
	return !operator==(o);
}

bool Rail::operator<(const Rail& o) const {
	bool p0eq = p0 == o.p0;
	bool p1eq = p1 == o.p1;
	bool p2eq = p2 == o.p2;
	if (!p0eq) return p0 < o.p0;
	if (!p1eq) return p1 < o.p1;
	if (!p2eq) return p2 < o.p2;
	return p3 < o.p3;
}
