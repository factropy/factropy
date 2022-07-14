#pragma once

// https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline

#include "point.h"

struct Curve {
	std::vector<Point> nodes;
	Curve();
	Curve(Point posA, Point dirA, Point posB, Point dirB);
	Point point(float t);
	Point origin();
	Point target();
	void clear();
	bool empty();
};
