#pragma once

struct Sphere;
#include "glm-ex.h"
#include "point.h"
#include "box.h"
#include <initializer_list>

struct Sphere {
	real x, y, z, r;

	Sphere();
	Sphere(Point p, real radius);
	Sphere(real x, real y, real z, real radius);
	Sphere(std::initializer_list<real>);

	operator const std::string() const {
		return fmt("{x:%0.2f,y:%0.2f,z:%0.2f,r:%0.2f}", x, y, z, r);
	};

	Point centroid() const;
	Box box() const;
	Sphere grow(real n) const;
	Sphere shrink(real n) const;
	Sphere translate(const Point p) const;
	Sphere translate(real x, real y, real z) const;
	bool intersects(Sphere a) const;
	bool intersects(Box b) const;
	bool intersects(Ray ray) const;
	bool contains(Point p) const;
};
