#include "frustum.h"
#include "point.h"
#include "sphere.h"
#include "box.h"

namespace {

	void planeNormalize(glm::dvec4& plane) {
		double magnitude = std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
		plane.x /= magnitude;
		plane.y /= magnitude;
		plane.z /= magnitude;
		plane.w /= magnitude;
	}

	double planeDistance(const glm::dvec4& plane, const Point& point) {
		return (plane.x * point.x + plane.y * point.y + plane.z * point.z + plane.w);
	}
}

Frustum::Frustum() {
	planes[Right] = glm::dvec4(0,0,0,0);
	planes[Left] = glm::dvec4(0,0,0,0);
	planes[Top] = glm::dvec4(0,0,0,0);
	planes[Bottom] = glm::dvec4(0,0,0,0);
	planes[Front] = glm::dvec4(0,0,0,0);
	planes[Back] = glm::dvec4(0,0,0,0);
}


Frustum::Frustum(const Mat4& cam) {
	double m0  = cam.m[0][0];
	double m1  = cam.m[0][1];
	double m2  = cam.m[0][2];
	double m3  = cam.m[0][3];
	double m4  = cam.m[1][0];
	double m5  = cam.m[1][1];
	double m6  = cam.m[1][2];
	double m7  = cam.m[1][3];
	double m8  = cam.m[2][0];
	double m9  = cam.m[2][1];
	double m10 = cam.m[2][2];
	double m11 = cam.m[2][3];
	double m12 = cam.m[3][0];
	double m13 = cam.m[3][1];
	double m14 = cam.m[3][2];
	double m15 = cam.m[3][3];

	planes[Right] = glm::dvec4(m3 - m0, m7 - m4, m11 - m8, m15 - m12);
	planeNormalize(planes[Right]);

	planes[Left] = glm::dvec4(m3 + m0, m7 + m4, m11 + m8, m15 + m12);
	planeNormalize(planes[Left]);

	planes[Top] = glm::dvec4(m3 - m1, m7 - m5, m11 - m9, m15 - m13);
	planeNormalize(planes[Top]);

	planes[Bottom] = glm::dvec4(m3 + m1, m7 + m5, m11 + m9, m15 + m13);
	planeNormalize(planes[Bottom]);

	planes[Back] = glm::dvec4(m3 - m2, m7 - m6, m11 - m10, m15 - m14);
	planeNormalize(planes[Back]);

	planes[Front] = glm::dvec4(m3 + m2, m7 + m6, m11 + m10, m15 + m14);
	planeNormalize(planes[Front]);
}

bool Frustum::contains(const Point& point) const {
	if (planeDistance(planes[Back],   point) < 0) return false;
	if (planeDistance(planes[Front],  point) < 0) return false;
	if (planeDistance(planes[Bottom], point) < 0) return false;
	if (planeDistance(planes[Top],    point) < 0) return false;
	if (planeDistance(planes[Right],  point) < 0) return false;
	if (planeDistance(planes[Left],   point) < 0) return false;
	return true;
}

bool Frustum::intersects(const Sphere& sphere) const {
	Point point = sphere.centroid();
	if (planeDistance(planes[Back],   point) < -sphere.r) return false;
	if (planeDistance(planes[Front],  point) < -sphere.r) return false;
	if (planeDistance(planes[Bottom], point) < -sphere.r) return false;
	if (planeDistance(planes[Top],    point) < -sphere.r) return false;
	if (planeDistance(planes[Right],  point) < -sphere.r) return false;
	if (planeDistance(planes[Left],   point) < -sphere.r) return false;
	return true;
}

bool Frustum::intersects(const Box& box) const {
	Point min = Point(box.x-box.w/2, box.y-box.h/2, box.z-box.d/2);
	Point max = Point(box.x+box.w/2, box.y+box.h/2, box.z+box.d/2);

	if (contains(Point(min.x, min.y, min.z))) return true;
	if (contains(Point(min.x, max.y, min.z))) return true;
	if (contains(Point(max.x, max.y, min.z))) return true;
	if (contains(Point(max.x, min.y, min.z))) return true;
	if (contains(Point(min.x, min.y, max.z))) return true;
	if (contains(Point(min.x, max.y, max.z))) return true;
	if (contains(Point(max.x, max.y, max.z))) return true;
	if (contains(Point(max.x, min.y, max.z))) return true;

	for (auto& plane: planes) {
		bool inside = false;
		inside = inside || planeDistance(plane, Point(min.x, min.y, min.z)) >= 0;
		inside = inside || planeDistance(plane, Point(max.x, min.y, min.z)) >= 0;
		inside = inside || planeDistance(plane, Point(max.x, max.y, min.z)) >= 0;
		inside = inside || planeDistance(plane, Point(min.x, max.y, min.z)) >= 0;
		inside = inside || planeDistance(plane, Point(min.x, min.y, max.z)) >= 0;
		inside = inside || planeDistance(plane, Point(max.x, min.y, max.z)) >= 0;
		inside = inside || planeDistance(plane, Point(max.x, max.y, max.z)) >= 0;
		inside = inside || planeDistance(plane, Point(min.x, max.y, max.z)) >= 0;
		if (!inside) return false;
	}

	return true;
}
