#pragma once

// Axis-aligned bounding box

struct Box;
#include "point.h"
#include "sphere.h"
#include "cylinder.h"
#include "volume.h"
#include <initializer_list>

struct Box {
	real x, y, z, w, h, d;

	Box();
	Box(Point p, Volume v);
	Box(Point a, Point b);
	Box(std::initializer_list<real>);

	operator const std::string() const {
		return fmt("{x:%0.2f,y:%0.2f,z:%0.2f,w:%0.2f,h:%0.2f,d:%0.2f}", x, y, z, w, h, d);
	};

	bool operator==(const Box& o) const;
	bool operator!=(const Box& o) const;

	Point centroid() const;
	Point dimensions() const;
	//BoundingBox bounds() const;
	Box translate(const Point& p) const;
	Box translate(real x, real y, real z) const;
	Box grow(const Point& p) const;
	Box grow(real n) const;
	Box grow(real x, real y, real z) const;
	Box shrink(const Point& p) const;
	Box shrink(real n) const;
	Box shrink(real x, real y, real z) const;
	Box side(const Point& dir, real thick);
	bool intersects(const Box& b) const;
	bool intersects(const Sphere& s) const;
	bool intersects(const Cylinder& c) const;
	bool intersects(const Ray& r) const;
	bool intersectsRay(const Point& pos, const Point& dir) const;
	bool contains(const Point& p) const;
	real volume() const;
	Sphere sphere() const;
};
