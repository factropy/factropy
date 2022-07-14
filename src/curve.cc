#include "curve.h"

// https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/spline.hpp>

Curve::Curve() {
	clear();
}

Curve::Curve(Point posA, Point dirA, Point posB, Point dirB) {
	for (int i = 0; i < 5; i++)
		nodes.push_back(posA + (dirA * (real)i));

	Point path = posB-posA;
	Point mid = path.normalize() * (path.length()/2.0);
	nodes.push_back(mid);

	for (int i = 4; i >= 0; --i)
		nodes.push_back(posB - (dirB * (real)i));
}

Point Curve::point(float t)
{
	int n = nodes.size();

	t = (float)n * t;
	t = std::max(t, 5.0f);
	t = std::min(t, (float)n-5.0f);

	// indices of the relevant control points
	int i0 = glm::clamp<int>(t - 1, 0, n - 1);
	int i1 = glm::clamp<int>(t,     0, n - 1);
	int i2 = glm::clamp<int>(t + 1, 0, n - 1);
	int i3 = glm::clamp<int>(t + 2, 0, n - 1);

	// parameter on the local curve interval
	float local = glm::fract(t);
	glm::dvec3 p = glm::catmullRom(
		glm::dvec3(nodes[i0]),
		glm::dvec3(nodes[i1]),
		glm::dvec3(nodes[i2]),
		glm::dvec3(nodes[i3]),
		local
	);
	return p;
}

Point Curve::origin() {
	return nodes.size() ? nodes.front(): Point::Zero;
}

Point Curve::target() {
	return nodes.size() ? nodes.back(): Point::Zero;
}

void Curve::clear() {
	nodes.clear();
}

bool Curve::empty() {
	return origin() == Point::Zero && target() == Point::Zero;
}
