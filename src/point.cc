#include "common.h"
#include "box.h"
#include "sim.h"

const Point Point::Zero  = glm::origin;
const Point Point::North = glm::north;
const Point Point::South = glm::south;
const Point Point::East  = glm::east;
const Point Point::West  = glm::west;
const Point Point::Up    = glm::up;
const Point Point::Down  = glm::down;

Point::Point(std::initializer_list<real> l) {
	auto i = l.begin();
	x = *i++;
	y = *i++;
	z = *i++;
}

Point::Point(real xx, real yy, real zz) {
	x = xx;
	y = yy;
	z = zz;
}

Point::Point(glm::point vec) {
	x = vec.x;
	y = vec.y;
	z = vec.z;
}

Point::Point(glm::vec3 vec) {
	x = vec.x;
	y = vec.y;
	z = vec.z;
}

Point::Point(Volume vol) {
	x = vol.w;
	y = vol.h;
	z = vol.d;
}

bool Point::valid() const {
	return !std::isnan(x) && !std::isnan(y) && !std::isnan(y);
}

bool Point::operator==(const Point& o) const {
	bool xeq = std::abs(x-o.x) < 0.0011;
	bool yeq = std::abs(y-o.y) < 0.0011;
	bool zeq = std::abs(z-o.z) < 0.0011;
	return xeq && yeq && zeq;
}

bool Point::operator!=(const Point& o) const {
	return !operator==(o);
}

bool Point::operator<(const Point& o) const {
	bool xeq = std::abs(x-o.x) < 0.0011;
	bool zeq = std::abs(z-o.z) < 0.0011;
	if (!xeq) {
		return x < o.x;
	}
	if (!zeq) {
		return z < o.z;
	}
	return y < o.y;
}

Point Point::operator+(const Point& o) const {
	return {x+o.x, y+o.y, z+o.z};
}

Point Point::operator+(real n) const {
	return {x+n, y+n, z+n};
}

Point Point::operator-(const Point& o) const {
	return {x-o.x, y-o.y, z-o.z};
}

Point Point::operator-(real n) const {
	return {x-n, y-n, z-n};
}

Point Point::operator-() const {
	return {-x, -y, -z};
}

Point Point::operator*(const Point& o) const {
	return {x*o.x, y*o.y, z*o.z};
}

Point Point::operator*(real s) const {
	return {x*s, y*s, z*s};
}

void Point::operator+=(const Point& o) {
	*this = *this+o;
}

void Point::operator-=(const Point& o) {
	*this = *this-o;
}

Box Point::box() const {
	real ep = std::numeric_limits<real>::epsilon() * 2;
	return (Box){x, y, z, ep, ep, ep};
}

Sphere Point::sphere() const {
	real ep = std::numeric_limits<real>::epsilon() * 2;
	return Sphere(x, y, z, ep);
}

real Point::distanceSquared(const Point& p) const {
	real dx = p.x - x;
	real dy = p.y - y;
	real dz = p.z - z;
	return dx*dx + dy*dy + dz*dz;
}

real Point::distance(const Point& p) const {
	return std::sqrt(distanceSquared(p));
}

real Point::degrees(const Point& p) const {
	return glm::degrees(radians(p));
}

real Point::radians(const Point& p) const {
	Point a = normalize();
	Point b = p.normalize();
	if (a == b) return 0.0f;
	if (a == -b) return glm::radians(180.0f);
	return std::acos(a.dot(b));
}

Point Point::round() const {
	return (Point){
		std::round(x),
		std::round(y),
		std::round(z),
	};
}

Point Point::tileCentroid() const {
	return (Point){
		std::floor(x) + 0.5f,
		std::floor(y) + 0.5f,
		std::floor(z) + 0.5f,
	};
}

Point Point::floor(real fy) const {
	return (Point){x,fy,z};
}

real Point::lineDistance(const Point& a, const Point& b) const {
	real ad = distance(a);
	real bd = distance(b);

	// https://gamedev.stackexchange.com/questions/72528/how-can-i-project-a-3d-point-onto-a-3d-line
	auto ap = *this-a;
	auto ab = b-a;
	auto i = a + (ab * (ap.dot(ab) / ab.dot(ab)));

//	Vector3 ap = Vector3Subtract(*this, a);
//	Vector3 ab = Vector3Subtract(b, a);
//	Vector3 i = Vector3Add(a, Vector3Scale(ab, Vector3DotProduct(ap,ab) / Vector3DotProduct(ab,ab)));
	real d = distance(i);

	real dl = a.distance(b);

	if (dl < bd) {
		return ad;
	}

	if (dl < ad) {
		return bd;
	}

	return d;
}

Point Point::normalize() const {
	Point r = *this;
	if (*this == Point::Zero) return Point::Zero;

	auto lenSq = lengthSquared();
	if (lenSq < 1.000001f && lenSq > 0.999999f) return *this;

	auto len = std::sqrt(lenSq);
	ensuref(len > 0.0, "normalize fail %s %f %f", std::string(r), lenSq, len);

	auto ilen = 1.0f/len;
	r.x *= ilen;
	r.y *= ilen;
	r.z *= ilen;

	return r;
}

Point Point::cross(const Point& b) const {
	return { y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x };
}

real Point::dot(const Point& b) const {
	return x*b.x + y*b.y + z*b.z;
}

Point Point::scale(real s) const {
	return *this * s;
}

Point Point::pivot(const Point& t, real speed) const {
	Mat4 r;
	Point target = t.normalize();
	// https://gamedev.stackexchange.com/questions/15070/orienting-a-model-to-face-a-target

	Point ahead = normalize();
	Point behind = -ahead;

	if (target == behind) {
		//r = Mat4::rotate(Vector3Perpendicular(ahead), 180.0f*DEG2RAD);
		r = Mat4::rotate(ahead == Point::Up ? Point::North: Point::Up, speed);
	}
	else
	if (target == ahead) {
		r = Mat4::identity;
	}
	else {
		Point axis = ahead.cross(target);
		real angle = std::acos(ahead.dot(target));
		real sign = angle < 0 ? -1.0f: 1.0f;
		real delta = std::abs(angle) < speed ? angle: speed*sign;
		r = Mat4::rotate(axis, delta);
	}
	return transform(r).normalize();
}

Point Point::roundCardinal() const {
	Point p = normalize();

	Point c = North;
	real d = p.distance(North);

	real ds = p.distance(South);
	if (ds < d) {
		c = South;
		d = ds;
	}

	real de = p.distance(East);
	if (de < d) {
		c = East;
		d = de;
	}

	real dw = p.distance(West);
	if (dw < d) {
		c = West;
		d = dw;
	}

	return c;
}

Point Point::rotateHorizontal() const {
	Point p = roundCardinal();

	if (p == North) return East;
	if (p == East) return South;
	if (p == South) return West;
	if (p == West) return North;

	return p;
}

Point Point::oppositeCardinal() const {
	return rotateHorizontal().rotateHorizontal();
}

bool Point::cardinalParallel(const Point& p) const {
	return
		((*this == North || *this == South) && (p == North || p == South)) ||
		((*this == East  || *this == West ) && (p == East  || p == West ));
}

bool Point::isCardinal() const {
	return *this == North || *this == South || *this == East || *this == West;
}

Point Point::randomHorizontal() const {
	real angle = Sim::random()*glm::radians(360.0f);
	return transform(Mat4::rotateY(angle));
}

Point Point::transform(const Mat4& m) const {
	glm::dvec4 v = glm::dmat4(m) * glm::dvec4(x,y,z,1);
	return {v.x,v.y,v.z};
}

real Point::length() const {
	return std::sqrt(x*x + y*y + z*z);
}

real Point::lengthSquared() const {
	return x*x + y*y + z*z;
}

Mat4 Point::rotation() const {
	return rotationAltAz();
}

namespace {
	const double southRads = std::atan2(Point::South.z, Point::South.x);
	const double eastRads = std::atan2(Point::East.y, Point::East.z);
}

Mat4 Point::rotationAltAz() const {
	auto dir = normalize();

	if (dir == Zero || dir == South) {
		return Mat4::identity;
	}
	else
	if (dir == North) {
		return Mat4::rotate(Up, glm::radians(180.0));
	}
	else
	if (dir == East) {
		return Mat4::rotate(Up, glm::radians(90.0));
	}
	else
	if (dir == West) {
		return Mat4::rotate(Up, glm::radians(270.0));
	}

	// azimuth
	auto angleAz = [](Point dir) {
		if (dir == Point::South) return 0.0;
		if (dir == -Point::South) return glm::radians(180.0);
		return southRads - std::atan2(dir.z, dir.x);
	};
	Mat4 r = Mat4::rotate(Point::Up, angleAz(Point(x,0,z).normalize()));

	if (y < -0.0001 || y > 0.0001) {
		// altitude
		auto angleAlt = [](Point dir) {
			if (dir == Point::East) return 0.0;
			if (dir == -Point::East) return glm::radians(180.0);
			return eastRads - std::atan2(dir.y, dir.z);
		};
		real l = std::sqrt(x*x + z*z);
		r = Mat4::rotate(Point::East, angleAlt(Point(0,y,l).normalize())) * r;
	}

	return r;
}

Mat4 Point::translation() const {
	return Mat4::translate(*this);
}

Point Point::nearestPointOnLine(const Point& start, const Point& end) const {
	auto line = (end - start);
	real len = line.length();
	auto nline = line.normalize();

	auto v = *this - start;
	real d = v.dot(nline);
	d = std::max((real)0.0, std::min(len, d));
	return start + nline * d;
}

bool
Point::linesCrossOnGround(const Point& a0, const Point& a1, const Point& b0, const Point& b1) //, real *i_x, real *i_y)
{
	// https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
	real ax0 = a0.x;
	real ay0 = a0.z;
	real ax1 = a1.x;
	real ay1 = a1.z;
	real bx0 = b0.x;
	real by0 = b0.z;
	real bx1 = b1.x;
	real by1 = b1.z;

	real s02_x, s02_y, s10_x, s10_y, s32_x, s32_y, s_numer, t_numer, denom; //, t;

	s10_x = ax1 - ax0;
	s10_y = ay1 - ay0;
	s32_x = bx1 - bx0;
	s32_y = by1 - by0;

	denom = s10_x * s32_y - s32_x * s10_y;
	if (denom == 0)
		return 0; // Collinear
	bool denomPositive = denom > 0;

	s02_x = ax0 - bx0;
	s02_y = ay0 - by0;
	s_numer = s10_x * s02_y - s10_y * s02_x;

	if ((s_numer < 0) == denomPositive)
		return 0; // No collision

	t_numer = s32_x * s02_y - s32_y * s02_x;

	if ((t_numer < 0) == denomPositive)
		return 0; // No collision

	if (((s_numer > denom) == denomPositive) || ((t_numer > denom) == denomPositive))
		return 0; // No collision

	// Collision detected
	//t = t_numer / denom;
	//if (i_x != NULL)
	//    *i_x = ax0 + (t * s10_x);
	//if (i_y != NULL)
	//    *i_y = ay0 + (t * s10_y);

	return 1;
}

Point::Relative Point::relative(const Point& dir, const Point& o) const {
	Point other = o;
	other.y = y;

	auto angle = [](Point dir) {
		if (dir == Point::North) {
			return (real) 180.0f;
		}
		if (dir == Point::East) {
			return (real) 90.0f;
		}
		if (dir == Point::West) {
			return (real) 270.0f;
		}
		if (dir == Point::South) {
			return (real) 0.0f;
		}
		return (real) glm::degrees(std::acos(Point::South.dot(dir)));
	};

	real dirAhead = angle(dir);
	real dirOther = angle((other - *this).normalize());
	real dirRel = dirOther - dirAhead;
	if (dirRel < 0) dirRel += 360;
	if (dirRel > 359) dirRel = 0;

	int deg = dirRel;

	if (deg > 45 && deg < 135) {
		return Left;
	}
	if (deg >= 135 && deg <= 225) {
		return Behind;
	}
	if (deg > 225 && deg < 315) {
		return Right;
	}
	return Ahead;
}


