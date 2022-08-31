#include "cuboid.h"
#include "point.h"
#include "sphere.h"
#include "box.h"
#include "glm-ex.h"

// Non-axis-aligned bounding boxes

Cuboid::Cuboid(const Box& bbox, const Point& ddir) {
	box = bbox;
	dir = ddir;
}

Cuboid Cuboid::grow(real n) const {
	return Cuboid(box.grow(n), dir);
}

bool Cuboid::contains(const Point& point) const {
	auto trx = (dir.rotationAltAz() * box.centroid().translation()).invert();
	Point position = point.transform(trx);
	return Box({0,0,0,box.w,box.h,box.d}).contains(position);
}

namespace {
	void computeVertices(Point* vertices, const Point& pos, const Point& size, const Mat4& rot) {
		auto max = size*0.5f;
		auto min = -max;
		vertices[0] = pos + min.transform(rot);
		vertices[1] = pos + Point(max.x, min.y, min.z).transform(rot);
		vertices[2] = pos + Point(min.x, max.y, min.z).transform(rot);
		vertices[3] = pos + Point(max.x, max.y, min.z).transform(rot);
		vertices[4] = pos + Point(min.x, min.y, max.z).transform(rot);
		vertices[5] = pos + Point(max.x, min.y, max.z).transform(rot);
		vertices[6] = pos + Point(min.x, max.y, max.z).transform(rot);
		vertices[7] = pos + max.transform(rot);
	}
}

localvec<Point> Cuboid::vertices() const {
	localvec<Point> vertices(8);
	computeVertices(vertices.data(), box.centroid(), box.dimensions(), dir.rotationAltAz());
	return vertices;
}

bool Cuboid::intersects(const Cuboid& other) const {
//	if (dir == other.dir) return box.intersects(other.box);

	struct Obb {
		Point vertices[8];
		Point axisX, axisY, axisZ;

		Obb(const Point& pos, const Point& size, const Point& dir) {
			auto rot = dir.rotationAltAz();
			computeVertices(vertices, pos, size, rot);
			axisY = Point::Up.transform(rot);
			axisX = Point::East.transform(rot);
			axisZ = Point::South.transform(rot);
		};

		bool intersects(const Obb& other) {
			const Obb& a = *this;
			const Obb& b = other;

			return !(
				   separated(a.vertices, b.vertices, a.axisX)
				|| separated(a.vertices, b.vertices, a.axisY)
				|| separated(a.vertices, b.vertices, a.axisZ)
				|| separated(a.vertices, b.vertices, b.axisX)
				|| separated(a.vertices, b.vertices, b.axisY)
				|| separated(a.vertices, b.vertices, b.axisZ)
				|| separated(a.vertices, b.vertices, a.axisX.cross(b.axisX))
				|| separated(a.vertices, b.vertices, a.axisX.cross(b.axisY))
				|| separated(a.vertices, b.vertices, a.axisX.cross(b.axisZ))
				|| separated(a.vertices, b.vertices, a.axisY.cross(b.axisX))
				|| separated(a.vertices, b.vertices, a.axisY.cross(b.axisY))
				|| separated(a.vertices, b.vertices, a.axisY.cross(b.axisZ))
				|| separated(a.vertices, b.vertices, a.axisZ.cross(b.axisX))
				|| separated(a.vertices, b.vertices, a.axisZ.cross(b.axisY))
				|| separated(a.vertices, b.vertices, a.axisZ.cross(b.axisZ))
			);
		};

		static bool separated(const Point* vertsA, const Point* vertsB, const Point& axis) {
			if (axis == Point::Zero) return false;

			real aMin = 0; //std::numeric_limits<real>::max();
			real aMax = 0; //-aMin; //std::numeric_limits<real>::min();
			real bMin = 0; //std::numeric_limits<real>::max();
			real bMax = 0; //-bMin; //std::numeric_limits<real>::min();

			for (uint i = 0; i < 8; i++) {
				auto aDist = vertsA[i].dot(axis);
				aMin = !i || aDist < aMin ? aDist : aMin;
				aMax = !i || aDist > aMax ? aDist : aMax;
				auto bDist = vertsB[i].dot(axis);
				bMin = !i || bDist < bMin ? bDist : bMin;
				bMax = !i || bDist > bMax ? bDist : bMax;
			}

			auto longSpan = std::max(aMax, bMax) - std::min(aMin, bMin);
			auto sumSpan = aMax - aMin + bMax - bMin;
			return longSpan >= sumSpan;
		}
	};

	Obb a = Obb(box.centroid(), box.dimensions(), dir);
	Obb b = Obb(other.box.centroid(), other.box.dimensions(), other.dir);

	return a.intersects(b);
}

bool Cuboid::intersects(const Box& box) const {
	return intersects(Cuboid(box, Point::South));
}

bool Cuboid::intersects(const Sphere& sphere) const {
	auto trx = (dir.rotationAltAz() * box.centroid().translation()).invert();
	Point position = sphere.centroid().transform(trx);
	return Box({0,0,0,box.w,box.h,box.d}).intersects(Sphere(position, sphere.r));
}

bool Cuboid::intersects(const Ray& ray) const {
	auto trx = (dir.rotationAltAz() * box.centroid().translation()).invert();
	auto position = Point(ray.position).transform(trx);
	auto direction = Point(ray.direction).transform(dir.rotationAltAz().invert()).normalize();
	return Box({0,0,0,box.w,box.h,box.d}).intersects((Ray){position,direction});
}

bool Cuboid::intersectsRay(const Point& rpos, const Point& rdir) const {
	auto trx = (dir.rotationAltAz() * box.centroid().translation()).invert();
	auto position = Point(rpos).transform(trx);
	auto direction = Point(rdir).transform(dir.rotationAltAz().invert()).normalize();
	return Box({0,0,0,box.w,box.h,box.d}).intersectsRay(position, direction);
}

