#include "common.h"
#include "cylinder.h"

Cylinder::Cylinder() {
	x = 0;
	y = 0;
	z = 0;
	r = 0;
	h = 0;
}

Cylinder::Cylinder(Point p, real radius, real height) {
	x = p.x;
	y = p.y;
	z = p.z;
	r = radius;
	h = height;
}

Cylinder::Cylinder(real px, real py, real pz, real radius, real height) {
	x = px;
	y = py;
	z = pz;
	r = radius;
	h = height;
}

Cylinder::Cylinder(std::initializer_list<real> l) {
	auto i = l.begin();
	x = *i++;
	y = *i++;
	z = *i++;
	r = *i++;
	h = *i++;
}

Point Cylinder::centroid() const {
	return (Point){x, y, z};
}

Box Cylinder::box() const {
	return {x, y, z, r*2, h, r*2};
}

bool Cylinder::intersects(Cylinder b) const {
	Point aa = centroid() + Point::Up*(h/2);
	Point ab = centroid() - Point::Up*(h/2);
	Point ba = b.centroid() + Point::Up*(b.h/2);
	Point bb = b.centroid() - Point::Up*(b.h/2);
	bool above = aa.y < bb.y;
	bool below = ab.y > ba.y;
	bool overlapV = !above && !below;
	bool overlapH = aa.floor(0).distance(ba.floor(0)) < r;
	return overlapV && overlapH;
}

bool Cylinder::intersects(Box b) const {
	Point aa = centroid() + Point::Up*(h/2);
	Point ab = centroid() - Point::Up*(h/2);
	Point ba = b.centroid() + Point::Up*(b.h/2);
	Point bb = b.centroid() - Point::Up*(b.h/2);
	bool above = aa.y < bb.y;
	bool below = ab.y > ba.y;
	bool overlapV = !above && !below;
	bool overlapH = Sphere(ab.floor(b.y), r).intersects(b);
	return overlapV && overlapH;
}

bool Cylinder::contains(Point p) const {
	Point aa = centroid() + Point::Up*(h/2);
	Point ab = centroid() - Point::Up*(h/2);
	bool above = aa.y < p.y;
	bool below = ab.y > p.y;
	bool overlapV = !above && !below;
	bool overlapH = aa.floor(p.y).distance(p) < r;
	return overlapV && overlapH;
}

