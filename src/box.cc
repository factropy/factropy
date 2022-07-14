#include "common.h"
#include "box.h"

// Axis-aligned bounding box

Box::Box() {
	x = 0;
	y = 0;
	z = 0;
	w = 0;
	h = 0;
	d = 0;
}

Box::Box(Point p, Volume v) {
	x = p.x;
	y = p.y;
	z = p.z;
	w = v.w;
	h = v.h;
	d = v.d;
}

Box::Box(Point a, Point b) {
	Point c = a + ((b - a) * 0.5f);
	x = c.x;
	y = c.y;
	z = c.z;
	w = std::abs(b.x - a.x);
	h = std::abs(b.y - a.y);
	d = std::abs(b.z - a.z);
}

Box::Box(std::initializer_list<real> l) {
	auto i = l.begin();
	x = *i++;
	y = *i++;
	z = *i++;
	w = *i++;
	h = *i++;
	d = *i++;
}

bool Box::operator==(const Box& o) const {
	bool xeq = std::abs(x-o.x) < 0.0011;
	bool yeq = std::abs(y-o.y) < 0.0011;
	bool zeq = std::abs(z-o.z) < 0.0011;
	bool weq = std::abs(w-o.w) < 0.0011;
	bool heq = std::abs(h-o.h) < 0.0011;
	bool deq = std::abs(d-o.d) < 0.0011;
	return xeq && yeq && zeq && weq && heq && deq;
}

bool Box::operator!=(const Box& o) const {
	return !operator==(o);
}

Point Box::centroid() const {
	return (Point){x, y, z};
}

Point Box::dimensions() const {
	return (Point){w, h, d};
}

// raylib's box
//BoundingBox Box::bounds() const {
//	return (BoundingBox){
//		(Vector3){(float)(x - w/2), (float)(y - h/2), (float)(z - d/2)},
//		(Vector3){(float)(x + w/2), (float)(y + h/2), (float)(z + d/2)},
//	};
//}

Box Box::translate(const Point& p) const {
	return (Box){x + p.x, y + p.y, z + p.z, w, h, d};
}

Box Box::translate(real xx, real yy, real zz) const {
	return translate(Point(xx,yy,zz));
}

Box Box::grow(const Point& p) const {
	return (Box){x, y, z, w + p.x*2, h + p.y*2, d + p.z*2};
}

Box Box::grow(real n) const {
	return grow(Point(n,n,n));
}

Box Box::grow(real xx, real yy, real zz) const {
	return grow(Point(xx,yy,zz));
}

Box Box::shrink(const Point& p) const {
	return grow((Point){-p.x, -p.y, -p.z});
}

Box Box::shrink(real n) const {
	return shrink(Point(n,n,n));
}

Box Box::shrink(real xx, real yy, real zz) const {
	return shrink(Point(xx,yy,zz));
}

Box Box::side(const Point& dir, real thick) {
	real hh = h/2.0;
	real wh = w/2.0;
	real dh = d/2.0;

	if (dir == Point::South) {
		return Box({x,y,z+dh,w,h,thick});
	}
	if (dir == Point::North) {
		return Box({x,y,z-dh,w,h,thick});
	}
	if (dir == Point::East) {
		return Box({x+wh,y,z,thick,h,d});
	}
	if (dir == Point::West) {
		return Box({x-wh,y,z,thick,h,d});
	}
	if (dir == Point::Up) {
		return Box({x,y+hh,z,w,thick,d});
	}
	if (dir == Point::Down) {
		return Box({x,y-hh,z,w,thick,d});
	}
	ensure(false);
	return *this;
}

bool Box::intersects(const Box& a) const {
	real xMin = x-w/2.0f;
	real yMin = y-h/2.0f;
	real zMin = z-d/2.0f;
	real xMax = x+w/2.0f;
	real yMax = y+h/2.0f;
	real zMax = z+d/2.0f;

	real axMin = a.x-a.w/2.0f;
	real ayMin = a.y-a.h/2.0f;
	real azMin = a.z-a.d/2.0f;
	real axMax = a.x+a.w/2.0f;
	real ayMax = a.y+a.h/2.0f;
	real azMax = a.z+a.d/2.0f;

	return (axMin <= xMax && axMax >= xMin) && (ayMin <= yMax && ayMax >= yMin) && (azMin <= zMax && azMax >= zMin);
}

bool Box::intersects(const Sphere& s) const {
	auto center = s.centroid();

	bool collision = false;

	real dmin = 0;

	real xMin = x-w/2.0f;
	real yMin = y-h/2.0f;
	real zMin = z-d/2.0f;
	real xMax = x+w/2.0f;
	real yMax = y+h/2.0f;
	real zMax = z+d/2.0f;

	if (center.x < xMin) dmin += std::pow(center.x - xMin, 2);
	else if (center.x > xMax) dmin += std::pow(center.x - xMax, 2);

	if (center.y < yMin) dmin += std::pow(center.y - yMin, 2);
	else if (center.y > yMax) dmin += std::pow(center.y - yMax, 2);

	if (center.z < zMin) dmin += std::pow(center.z - zMin, 2);
	else if (center.z > zMax) dmin += std::pow(center.z - zMax, 2);

	if (dmin <= (s.r*s.r)) collision = true;

	return collision;
}

bool Box::intersects(const Ray& r) const {
	return intersectsRay(r.position, r.direction);
}

bool Box::intersectsRay(const Point& pos, const Point& dir) const {
	bool collision = false;

	real xMin = x-w/2.0f;
	real yMin = y-h/2.0f;
	real zMin = z-d/2.0f;
	real xMax = x+w/2.0f;
	real yMax = y+h/2.0f;
	real zMax = z+d/2.0f;

	real t[8];
	t[0] = (xMin - pos.x)/dir.x;
	t[1] = (xMax - pos.x)/dir.x;
	t[2] = (yMin - pos.y)/dir.y;
	t[3] = (yMax - pos.y)/dir.y;
	t[4] = (zMin - pos.z)/dir.z;
	t[5] = (zMax - pos.z)/dir.z;
	t[6] = (real)std::max(std::max(std::min(t[0], t[1]), std::min(t[2], t[3])), std::min(t[4], t[5]));
	t[7] = (real)std::min(std::min(std::max(t[0], t[1]), std::max(t[2], t[3])), std::max(t[4], t[5]));

	collision = !(t[7] < 0 || t[6] > t[7]);

	return collision;
}

bool Box::contains(const Point& p) const {
	real xMin = x-w/2.0f - 0.000001f;
	real yMin = y-h/2.0f - 0.000001f;
	real zMin = z-d/2.0f - 0.000001f;
	real xMax = x+w/2.0f + 0.000001f;
	real yMax = y+h/2.0f + 0.000001f;
	real zMax = z+d/2.0f + 0.000001f;

	bool xc = xMin < p.x && xMax > p.x;
	bool yc = yMin < p.y && yMax > p.y;
	bool zc = zMin < p.z && zMax > p.z;

	return xc && yc && zc;
}

real Box::volume() const {
	return w*d*h;
}

Sphere Box::sphere() const {
	real r = Point(w/2.0f, h/2.0f, d/2.0f).length();
	return Sphere(x, y, z, r);
}
