#include "sky.h"
#include "world.h"
#include "flight-path.h"

// The Sky is broken up into a lattice of chunks used by aircraft to
// reserve non-colliding flight paths

Sky sky;

int Sky::chunk() const {
	return 32;
}

int Sky::size() const {
	return world.size()/chunk();
}

Box Sky::Block::box() const {
	real chunk = sky.chunk();
	real half = chunk/2;
	return {(x*chunk)+half,(y*chunk)+half,(z*chunk)+half,chunk,chunk,chunk};
}

Point Sky::Block::centroid() const {
	real chunk = sky.chunk();
	real half = chunk/2;
	return {(x*chunk)+half,(y*chunk)+half,(z*chunk)+half};
}

Sky::Block* Sky::get(int x, int y, int z) {
	auto it = blocks.find({.x = x, .y = y, .z = z});
	return (it == blocks.end()) ? nullptr: (Block*)&*it;
}

Sky::Block* Sky::nearest(Point pos) {
	int x = std::max(-size(), std::min(size(), (int)std::floor(pos.x/chunk())));
	int y = std::max(1, std::min(4, (int)std::floor(pos.y/chunk())));
	int z = std::max(-size(), std::min(size(), (int)std::floor(pos.z/chunk())));
	return get(x, y, z);
}

Sky::Block* Sky::within(Point pos) {
	int x = (int)std::floor(pos.x/chunk());
	int y = (int)std::floor(pos.y/chunk());
	int z = (int)std::floor(pos.z/chunk());

	if (x < -size() || x > size()) return nullptr;
	if (y < 1 || y > 4) return nullptr;
	if (z < -size() || z > size()) return nullptr;

	return nearest(pos);
}

void Sky::reset() {
	blocks.clear();
}

void Sky::init() {
	std::map<gridwalk::xy,float> elevations;

	for (int x = -size(); x <= size(); x++) {
		for (int z = -size(); z <= size(); z++) {
			elevations[{x,z}] = 0;
		}
	}

	for (auto& tile: world.tiles) {
		int x = tile.x/chunk();
		// sky is y-up
		int z = tile.y/chunk();
		elevations[{x,z}] = std::max(elevations[{x,z}], tile.elevation);
	}

	for (int x = -size(); x <= size(); x++) {
		for (int y = 1; y <= 4; y++) {
			for (int z = -size(); z <= size(); z++) {
				Block block = {
					.x = x,
					.y = y,
					.z = z,
					.clear = elevations[{x,z}] < y*chunk(),
				};
				blocks.insert(block);
			}
		}
	}
}

void Sky::save(const char* name) {
}

void Sky::load(const char* name) {
	init();
}

Sky::Path Sky::path(Point origin, Point target, uint rid) {
	Path result, none;

	if (origin.distance(target) > chunk()*2) {
		Point dir = (target-origin).normalize();
		origin = origin + ( dir * (real)chunk());
		target = target + (-dir * (real)chunk());
	}

	Block* start = nearest(origin);
	Block* finish = nearest(target);

	// impossible take off or landing
	if (!start || !finish) {
		//notef("impossible take off or landing");
		return none;
	}

	auto acquire = [&](Point pos) {
		Block* block = within(pos);
		if (!block) return false;
		if (!block->clear) return false;
		if (!block->reserved()) return true;

		// block is reserved, but if the reserver is travelling
		// away from us and far enough ahead it could be ok
		for (const auto& res: block->reservations) {
			if (res.rid == rid) return true;

			if (!FlightPath::all.has(res.rid)) continue;
			auto& flight = FlightPath::get(res.rid);

			real distA = flight.en->pos().distance(origin);

			// too close
			if (distA < chunk()*2) return false;

			Point dir = (flight.destination - flight.origin).normalize();
			real distB = (flight.en->pos() + dir).distance(origin);

			// they're aproaching
			if (distB < distA) return false;
		}

		return true;
	};

	// no available take-off block
	if (!acquire(start->centroid())) {
		//notef("no available take-off block");
		return none;
	}

	// no available landing block
	if (!acquire(finish->centroid())) {
		//notef("no available landing block");
		return none;
	}

	if (start == finish) {
		result.blocks = {start};
		result.waypoints = {start->centroid()};
		return result;
	}

	auto altitude = [&](int y) {
		float edge = (y-1) * chunk();
		float step = chunk()/2;

		Point here = start->centroid();
		Point end = finish->centroid();
		Point dir = (end-here).normalize();

		minivec<Point> waypoints;

		// lowest layer, no rise/fall
		if (y == 1) {
			Point move = dir * step;
			for (Point pos = here; pos.distance(end) > chunk(); pos += move) {
				if (!acquire(pos)) return false;
				waypoints.push(pos);
			}
		}
		// higher layer with rise/fall
		else {
			Point ascent = here + (Point::Up * edge) + (dir * edge);
			Point descent = end + (Point::Up * edge) - (dir * edge);

			if (!acquire(ascent)) return false;
			if (!acquire(descent)) return false;

			// 45 degree rise/fall won't work with given range and height
			if (nearest(ascent) != nearest(descent) && here.distance(descent) < here.distance(ascent)) return false;

			Point rise = (ascent - here).normalize() * step;
			Point fall = (end - descent).normalize() * step;
			Point move = (descent - ascent).normalize() * step;

			for (Point pos = here; pos.distance(ascent) > chunk(); pos += rise) {
				if (!acquire(pos)) return false;
				waypoints.push(pos);
			}

			for (Point pos = ascent; pos.distance(descent) > chunk(); pos += move) {
				if (!acquire(pos)) return false;
				waypoints.push(pos);
			}

			for (Point pos = descent; pos.distance(end) > chunk(); pos += fall) {
				if (!acquire(pos)) return false;
				waypoints.push(pos);
			}
		}

		if (!waypoints.size() || waypoints.back() != end) waypoints.push(end);

		miniset<Block*> check;
		minivec<Block*> blocks;

		for (auto pos: waypoints) {
			auto block = within(pos);
			if (!block) return false;
			if (check.has(block)) continue;
			check.insert(block);
			blocks.push(block);
		}

		if (blocks.back() != finish) blocks.push(finish);

		result.blocks = blocks;
		result.waypoints = waypoints;

		for (auto block: result.blocks) block->reserve(rid);

		return true;
	};

	if (altitude(1)) return result;
	if (altitude(2)) return result;
	if (altitude(3)) return result;
	if (altitude(4)) return result;

	//notef("no path");
	return none;
}

