#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <initializer_list>

namespace glm {
	typedef dvec3 point;
	typedef dmat4 matrix;

	const inline point origin(0,0,0);
	const inline point up(0,1,0);
	const inline point down(0,-1,0);
	const inline point north(0,0,-1);
	const inline point south(0,0,1);
	const inline point east(1,0,0);
	const inline point west(-1,0,0);

	struct ray {
		point position;
		point direction;
		ray() {
			position = glm::origin;
			direction = glm::south;
		}
		ray(point o, point d) {
			position = o;
			direction = d;
		}
	};
}

typedef glm::ray Ray;