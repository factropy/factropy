#pragma once

struct Cylinder;
#include "glm-ex.h"
#include "point.h"
#include "box.h"
#include <initializer_list>

struct Cylinder {
	real x, y, z, r, h;

	Cylinder();
	Cylinder(Point p, real radius, real height);
	Cylinder(real x, real y, real z, real radius, real height);
	Cylinder(std::initializer_list<real>);

	operator const std::string() const {
		return fmt("{x:%0.2f,y:%0.2f,z:%0.2f,r:%0.2f,h:%0.2f}", x, y, z, r, h);
	};

	Point centroid() const;
	Box box() const;
	bool intersects(Cylinder b) const;
	bool intersects(Box b) const;
	bool contains(Point p) const;
};

