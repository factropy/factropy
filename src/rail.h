#pragma once

#include "point.h"
#include "minivec.h"

struct Rail {
	Point p0 = Point::Zero;
	Point p1 = Point::Zero;
	Point p2 = Point::Zero;
	Point p3 = Point::Zero;
	Rail();
	Rail(Point posA, Point dirA, Point posB, Point dirB);
	Point point(real t);
	Point origin();
	Point target();
	void clear();
	bool empty();
	bool valid(real limit, real degrees, real* length = nullptr);
	real length();
	minivec<Point> steps(real step);
	bool straight();

	bool operator==(const Rail& o) const;
	bool operator!=(const Rail& o) const;
	bool operator<(const Rail& o) const;
};

