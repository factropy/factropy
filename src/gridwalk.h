#pragma once

// A gridwalk(er) is an iterator for an unbounded 2D plane broken up
// into chunks/tiles of a predefined size. They can iterate from anything
// that has an axis-aligned bounding box.

#include "common.h"
#include "box.h"
#include "point.h"
#include <initializer_list>

struct gridwalk {
	Box box = Point::Zero.box();
	int chunk = 1;

	gridwalk(int cchunk, Box bbox) {
		chunk = cchunk;
		box = bbox;
	}

	struct xy {
		int x = 0;
		int y = 0;

		xy() = default;

		xy(int xx, int yy) {
			x = xx;
			y = yy;
		}

		xy(std::initializer_list<int>& l) {
			auto i = l.begin();
			x = *i++;
			y = *i++;
		}

		xy(const Point& p) {
			x = (int)std::floor(p.x);
			y = (int)std::floor(p.z);
		}

		bool operator==(const xy &o) const {
			return x == o.x && y == o.y;
		}

		bool operator!=(const xy &o) const {
			return x != o.x || y != o.y;
		}

		bool operator<(const xy &o) const {
			return x < o.x || (x == o.x && y < o.y);
		}

		bool operator>(const xy &o) const {
			return !operator<(o) && !operator==(o);
		}

		bool operator<=(const xy &o) const {
			return operator<(o) || operator==(o);
		}

		Point centroid() const {
			return Point((real)x+0.5,0,(real)y+0.5);
		}
	};

	struct iterator {
		int cx0, cy0;
		int cx1, cy1;
		int cx, cy;

		typedef xy value_type;
		typedef std::ptrdiff_t difference_type;
		typedef xy* pointer;
		typedef xy& reference;
		typedef std::input_iterator_tag iterator_category;

		explicit iterator(int chunk, Box box) {
			Point half = Point(box.w, box.h, box.d) * 0.5f;
			Point min = box.centroid() - half;
			Point max = box.centroid() + half;

			cx0 = (int)std::floor(min.x/(float)chunk);
			cy0 = (int)std::floor(min.z/(float)chunk);
			cx1 = (int)std::ceil(max.x/(float)chunk);
			cy1 = (int)std::ceil(max.z/(float)chunk);

			cx = cx0;
			cy = cy0;
		}

		xy operator*() const {
			return {cx,cy};
		}

		bool operator==(const iterator& other) const {
			return cx == other.cx && cy == other.cy;
		}

		bool operator!=(const iterator& other) const {
			return cx != other.cx || cy != other.cy;
		}

		iterator& operator++() {
			cx++;
			if (cx >= cx1) {
				cx = cx0;
				cy++;
			}
			if (cy >= cy1) {
				cx = cx0;
				cy = cy1;
			}
			return *this;
		}
	};

	iterator begin() const {
		return iterator(chunk, box);
	}

	iterator end() const {
		auto it = iterator(chunk, box);
		it.cx = it.cx0;
		it.cy = it.cy1;
		return it;
	}

	std::vector<xy> spiral() {
		std::vector<xy> steps = {begin(), end()};
		auto centroid = box.centroid();
		std::sort(steps.begin(), steps.end(), [&](auto a, auto b) {
			auto ap = Point(a.x*chunk, 0, a.y*chunk) + Point(chunk/2,0,chunk/2);
			auto bp = Point(b.x*chunk, 0, b.y*chunk) + Point(chunk/2,0,chunk/2);
			return ap.distanceSquared(centroid) < bp.distanceSquared(centroid);
		});
		return steps;
	}

	bool operator==(const gridwalk &o) const {
		if (chunk != o.chunk) return false;
		auto a = begin();
		auto b = o.begin();
		while (a != end() && b != o.end() && *a == *b) { ++a; ++b; }
		return a == end() && b == o.end();
	}
};

namespace std {
	template<> struct hash<gridwalk::xy> {
		std::size_t operator()(gridwalk::xy const& at) const noexcept {
			return std::hash<uint>{}(at.x) + std::hash<uint>{}(at.y);
		}
	};
}
