#include "common.h"
#include "sphere.h"

Sphere::Sphere() {
	x = 0;
	y = 0;
	z = 0;
	r = 0;
}

Sphere::Sphere(Point p, real radius) {
	x = p.x;
	y = p.y;
	z = p.z;
	r = radius;
}

Sphere::Sphere(real px, real py, real pz, real radius) {
	x = px;
	y = py;
	z = pz;
	r = radius;
}

Sphere::Sphere(std::initializer_list<real> l) {
	auto i = l.begin();
	x = *i++;
	y = *i++;
	z = *i++;
	r = *i++;
}

Point Sphere::centroid() const {
	return (Point){x, y, z};
}

Box Sphere::box() const {
	return {x, y, z, r*2, r*2, r*2};
}

Sphere Sphere::grow(real n) const {
	real ep = std::numeric_limits<real>::epsilon() * 2;
	return Sphere(centroid(), std::max(r+n, ep));
}

Sphere Sphere::shrink(real n) const {
	return grow(-n);
}

Sphere Sphere::translate(Point p) const {
	return Sphere(centroid()+p, r);
}

Sphere Sphere::translate(real xx, real yy, real zz) const {
	return translate(Point(xx,yy,zz));
}

bool Sphere::intersects(Sphere a) const {
	return centroid().distance(a.centroid()) < r+a.r;
}

bool Sphere::intersects(Box b) const {
	return b.intersects(*this);
}

bool Sphere::intersects(Ray ray) const {
	bool collision = false;
	Point raySpherePos = centroid() - ray.position;
	double distance = raySpherePos.length();
	double vec = raySpherePos.dot(ray.direction);
	double d = r*r - (distance*distance - vec*vec);
	if (d > 0.0) collision = true;
	return collision;
}

bool Sphere::contains(Point p) const {
	return centroid().distance(p) < r;
}
