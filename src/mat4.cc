#include "common.h"
#include "mat4.h"
#include "glm-ex.h"
#include <cstring>

const Mat4 Mat4::identity = glm::matrix(1.0);

Mat4::Mat4(const glm::dmat4& o) {
	m = o;
}

Mat4::Mat4(const glm::mat4& o) {
	m = o;
}

Mat4 Mat4::scale(real x, real y, real z) {
	return Mat4(glm::scale(glm::point(x,y,z)));
}

Mat4 Mat4::scale(real s) {
	return scale(s, s, s);
}

Mat4 Mat4::rotate(const Point& axs, float radians) {
	return Mat4(glm::rotate(radians, glm::vec3(axs)));
}

Mat4 Mat4::rotateX(float radians) {
	return rotate(Point::East, radians);
}

Mat4 Mat4::rotateY(float radians) {
	return rotate(Point::Up, radians);
}

Mat4 Mat4::rotateZ(float radians) {
	return rotate(Point::South, radians);
}

Mat4 Mat4::translate(real x, real y, real z) {
	return glm::translate(glm::point(x,y,z));
}

Mat4 Mat4::translate(const Point& p) {
	return translate(p.x, p.y, p.z);
}

Mat4 Mat4::normalize() const {
	return Mat4(m / glm::determinant(m));
}

Mat4 Mat4::invert() const {
	return Mat4(glm::inverse(m));
}

Mat4 Mat4::operator*(const Mat4& o) const {
	return Mat4(o.m * m);
}
