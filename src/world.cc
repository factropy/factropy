#include "world.h"
#include "flate.h"
#include "log.h"
#include "save.h"
#include "workers.h"
#include "config.h"

World world;

gridwalk World::walk(const Box& box) {
	return gridwalk(1, box);
}

int World::size() {
	return scenario.size;
}

Box World::box() {
	double n = (double)size();
	return Box({0,0,0,n,n,n});
}

uint64_t World::memory() {
	return (size()*size()/8)
		+ tiles.size()*sizeof(Tile)
		+ features.size()*sizeof(Feature)
	;
}

void World::reset() {
	flags.tiles.clear();
	tiles.clear();
	features.clear();
	changes.clear();
	nextHill = 1;
	nextLake = -1;
	ready = false;
	game++;
}

void World::init() {
	ensure(scenario.size && scenario.size%sizeMin == 0);

	flags.tiles.clear();
	flags.tiles.resize(size()*size()/8, 0);

	int jobs = 0;
	channel<bool,-1> done;

	struct TileResult {
		XY at;
		Tile tile;
	};

	channel<TileResult,-1> results;

	jobs++;
	async.job([&]() {
		for (auto res: results) {
			Tile* tile = &tiles.emplace_back();
			tile->x = res.at.x;
			tile->y = res.at.y;
			tile->elevation = res.tile.elevation;
			tile->feature = 0;
			tile->resource = res.tile.resource;
			tile->count = res.tile.count;
			flag(res.at, true);
		}
		done.send(true);
	});

	notef("Generating noise layers...");
	// Generation occurs at startup so we're free to hammer the thread pool
	// without fear of deadlock or slowing down rendering
	for (int x = -size()/2; x < size()/2; x++) {
		jobs++;
		async.job([&,x]() {
			for (int y = -size()/2; y < size()/2; y++) {
				float elevation = (float)Sim::noise2D(x, y, 8, 0.5, 0.002) - 0.5f;

				// floodplain
				if (elevation > 0.0f) {
					if (elevation < 0.2f) {
						elevation = 0.0f;
					} else {
						elevation -= 0.2f;
						elevation *= 2.5f;
					}
				}

				if (elevation < 0.0f) {
					if (elevation > -0.25f) {
						elevation = 0.0f;
					} else {
						elevation += 0.25f;
						elevation *= 2.5f;
					}
				}

				elevation *= 100;

				if (elevation > 0.001 || elevation < -0.001) {
					XY at = {x,y};
					Tile tile;
					tile.elevation = elevation;

					double bump = 1000000;
					float resourceLoad = 0.2f;

					uint mineral = 0;
					float mineralDensity = 0.0f;

					uint deposit = 0;
					float depositDensity = 0.0f;

					float scaleDistance = Point(x, 0, y).length()/4.0f;

					if (tile.hill()) {
						for (auto [iid,mul]: Item::mining) {
							mul *= resourceLoad;
							float density = (float)Sim::noise2D(x+bump, y+bump, 8, 0.4, 0.005) * mul;
							if (density > mineralDensity) {
								mineral = iid;
								mineralDensity = density;
							}
							bump += 1000000;
						}
						tile.resource = mineral;
						tile.count = (uint)(mineralDensity*scaleDistance);
					}

					if (tile.lake()) {
						for (auto [fid,mul]: Fluid::drilling) {
							mul *= resourceLoad;
							float density = (float)Sim::noise2D(x+bump, y+bump, 8, 0.4, 0.005) * mul;
							if (density > depositDensity) {
								deposit = fid;
								depositDensity = density;
							}
							bump += 1000000;
						}
						tile.resource = deposit;
						tile.count = (uint)(depositDensity*scaleDistance);
					}

					results.send({.at = at, .tile = tile});
				}
			}
			done.send(true);
		});
	}

	while (jobs > 1) { done.recv(); jobs--; }

	results.close();
	done.recv();

	detect();
	index();
}

void World::detect() {
	notef("Sorting terrain tiles...");

	std::sort(tiles.begin(), tiles.end(), [](const auto& a, const auto& b) {
		return XY(a.x, a.y) < XY(b.x, b.y);
	});

	ready = true;

	for (uint i = 0; i < tiles.size(); i++) {
		auto tile = &tiles[i];
		ensure(!tile->feature);
		ensure(flag(tile->at()));
		ensure(tile->hill() || tile->lake());
	}

	ensure(!features.size());
	nextHill = 1;
	nextLake = -1;

	notef("Locating hills and lakes...");

	for (uint i = 0; i < tiles.size(); i++) {
		auto tile = &tiles[i];
		if (tile->feature) continue;

		if (tile->hill()) {
			auto& hill = features[nextHill];
			hill.id = nextHill++;
			flood(tile->at(), hill.id);
		}

		if (tile->lake()) {
			auto& lake = features[nextLake];
			lake.id = nextLake--;
			flood(tile->at(), lake.id);
		}

		ensure(tile->feature && features.count(tile->feature));
	}
}

void World::flood(const XY& first, int id) {
	ensure(id);

	std::vector<uint8_t> done(size()*size()/8, 0);
	std::vector<Tile*> q;

	auto spill = [&](XY at) {
		if (!within(at)) return;

		if (flag(at, done.data())) return;
		flag(at, true, done.data());

		auto tile = get(at);
		if (tile) q.push_back(tile);
	};

	spill(first);

	ensure(q.size());

	Tile* ftile = get(first);
	ensure(ftile);

	while (q.size()) {
		auto tile = q.back();
		q.pop_back();

		if (tile->feature) continue;

		bool hill = ftile->hill() && tile->hill();
		bool lake = ftile->lake() && tile->lake();

		if (hill) ensure(!lake);
		if (lake) ensure(!hill);

		if (hill || lake) {
			auto at = tile->at();
			tile->feature = id;
			spill({at.x-1,at.y-1});
			spill({at.x-1,at.y+0});
			spill({at.x-1,at.y+1});
			spill({at.x+0,at.y-1});
			spill({at.x+0,at.y+1});
			spill({at.x+1,at.y-1});
			spill({at.x+1,at.y+0});
			spill({at.x+1,at.y+1});
		}
	}
}

void World::index() {
	for (uint i = 0; i < tiles.size(); i++) {
		auto tile = &tiles[i];
		ensure(tile->feature && features.count(tile->feature));
		features[tile->feature].tiles.push_back(i);
	}

	for (auto& [_,ft]: features) {
		ensure(ft.id != 0);
		ensure(ft.tiles.size() > 0);

		for (auto off: ft.tiles) {
			auto tile = &tiles[off];
			if (!tile->resource) continue;

			if (ft.id > 0) {
				ft.mining.stacks[tile->resource].size += tile->count;
			}
			if (ft.id < 0) {
				ft.drilling.amounts[tile->resource].size += tile->count;
			}
		}

		for (auto& stack: ft.mining.stacks) {
			ft.mining.offsets[stack.iid].size = 0;
		}

		for (auto& amount: ft.drilling.amounts) {
			ft.drilling.offsets[amount.fid].size = 0;
		}
	}
}

void World::save(const char* name, channel<bool,3>* tickets) {
	auto tilesSave = tiles;
	auto path = std::string(name);

	Fluid* oil = Fluid::names["oil"];
	ensure(oil);

	async.job([=]() {
		deflation def;
		def.push(fmt("%u", (uint)tilesSave.size()));

		for (auto& tile: tilesSave) {
			def.push(fmt("%f %d %d %s %u",
				tile.elevation,
				tile.x,
				tile.y,
				tile.resource ? (tile.elevation < 0 ? oil->name: Item::get(tile.resource)->name): "_",
				tile.count
			));
		}

		def.save(path + "/world");
		tickets->recv();
	});
}

void World::load(const char* name) {
	auto path = std::string(name);

	inflation inf;
	auto lines = inf.load(path + "/world").parts();
	auto it = lines.begin();
	auto line = *it++;
	char tmp[128];

	auto linetmp = [&]() {
		ensure(line.size() < sizeof(tmp));
		memmove(tmp, line.data(), line.size());
		tmp[line.size()] = 0;
	};

	linetmp();
	uint n = 0;
	int w = 0;

	for (;;) {
		if (2 == std::sscanf(tmp, "%u %d", &n, &w)) break;
		if (1 == std::sscanf(tmp, "%u", &n)) break;
		ensuref(false, "world invalid");
	}

	// original hard coded value in old saves
	if (!w) w = 4096;

	notef("Found %u tiles with map width %dm x %dm", n, w, w);

	scenario.size = w;
	flags.tiles.clear();
	flags.tiles.resize(size()*size()/8, 0);

	Fluid* oil = Fluid::names["oil"];
	ensure(oil);

	for (uint i = 0; i < n; i++) {
		line = *it++;
		linetmp();
		char *ptr = tmp;

		// much faster than sscanf
		float elevation = strtof(ptr, &ptr);
		ensure(isspace(*ptr));
		int x = strtol(++ptr, &ptr, 10);
		ensure(isspace(*ptr));
		int y = strtol(++ptr, &ptr, 10);
		ensure(isspace(*ptr));
		char* start = ++ptr;
		while (!isspace(*ptr)) ptr++;
		char *end = ptr;
		auto name = std::string(start, end-start);
		uint resource = name == "_" ? 0: (elevation < 0 ? oil->id: Item::byName(name)->id);
		ensure(isspace(*ptr));
		uint count = strtoul(++ptr, &ptr, 10);
		ensure(!*ptr);

		if (elevation < -0.001 || elevation > 0.001) {
			Tile* tile = &tiles.emplace_back();
			tile->elevation = elevation;
			tile->x = x;
			tile->y = y;
			tile->feature = 0;
			tile->resource = resource;
			tile->count = count;
			flag({x,y}, true);
		}
	}

	detect();
	index();
}

bool World::within(const XY& at) {
	return (at.x >= -size()/2 && at.x < size()/2 && at.y >= -size()/2 && at.y < size()/2);
}

void World::flatten(const Box& b) {
	for (auto at: walk(b)) {
		Tile* tile = get(at);
		if (!tile) continue;
		if (tile->land()) continue;

		bool logged = false;
		for (auto& change: changes) {
			logged = logged || (change.at == at && change.tick == Sim::tick);
		}

		if (!logged) {
			changes.push_back({
				.at = tile->at(),
				.tick = Sim::tick,
			});
		}

		if (tile->feature) {
			ensure(features.count(tile->feature));
			ensure(tile->resource);
			if (tile->feature > 0) {
				auto& hill = features[tile->feature];
				ensure(hill.mining.stacks[tile->resource].size >= tile->count);
				hill.mining.stacks[tile->resource].size -= tile->count;
				discard_if(hill.tiles, [&](uint off) { return &tiles[off] == tile; });
			}
			if (tile->feature < 0) {
				auto& lake = features[tile->feature];
				ensure(lake.drilling.amounts[tile->resource].size >= tile->count);
				lake.drilling.amounts[tile->resource].size -= tile->count;
				discard_if(lake.tiles, [&](uint off) { return &tiles[off] == tile; });
			}
			tile->feature = 0;
		}

		tile->elevation = 0;
		tile->resource = 0;
		tile->count = 0;
	}
}

float World::blast(const Sphere& s) {
	Point c = s.centroid().floor(0.0);

	auto lower = [&](float r, float e) {
		auto s = Sphere(c, r);
		int changed = 0;

		for (auto at: walk(s.box())) {
			Tile* tile = get(at);
			if (!tile) continue;
			if (tile->elevation < e) continue;
			if (!s.intersects(Sphere(Point(at.x, 0, at.y), 0.5f))) continue;

			bool logged = false;
			for (auto& change: changes) {
				logged = logged || (change.at == at && change.tick == Sim::tick);
			}

			if (!logged) {
				changes.push_back({
					.at = tile->at(),
					.tick = Sim::tick,
				});
			}

			changed++;

			if (e < 0.25) {
				if (tile->feature) {
					ensure(features.count(tile->feature));
					ensure(tile->resource);
					if (tile->feature > 0) {
						auto& hill = features[tile->feature];
						ensure(hill.mining.stacks[tile->resource].size >= tile->count);
						hill.mining.stacks[tile->resource].size -= tile->count;
						discard_if(hill.tiles, [&](uint off) { return &tiles[off] == tile; });
					}
					if (tile->feature < 0) {
						auto& lake = features[tile->feature];
						ensure(lake.drilling.amounts[tile->resource].size >= tile->count);
						lake.drilling.amounts[tile->resource].size -= tile->count;
						discard_if(lake.tiles, [&](uint off) { return &tiles[off] == tile; });
					}
					tile->feature = 0;
				}
				tile->elevation = 0;
				tile->resource = 0;
				tile->count = 0;
			}
			else {
				tile->elevation = e + Sim::random()*0.25;
			}
		}

		return changed;
	};

	lower(s.r, 0.0);

	for (int i = s.r+1, l = 1000, e = 1; i < l; i++, e++) {
		if (!lower(i, e)) return i;
	}

	return 1000;
}

bool World::flag(const XY& at, const uint8_t* buf) {
	if (!buf) buf = flags.tiles.data();
	uint slot = (at.x+size()/2) * size() + (at.y+size()/2);
	uint cell = slot/8;
	uint shift = slot%8;
	return buf[cell] & (1<<shift) ? true:false;
}

bool World::flag(const XY& at, bool state, uint8_t* buf) {
	if (!buf) buf = flags.tiles.data();
	uint slot = (at.x+size()/2) * size() + (at.y+size()/2);
	uint cell = slot/8;
	uint shift = slot%8;
	if (state) {
		buf[cell] |= (1<<shift);
	} else {
		buf[cell] &= ~(1<<shift);
	}
	return state;
}

World::Tile* World::get(const XY& at) {
	ensure(ready);

	thread_local struct {
		XY at = {0,0};
		uint offset = 0;
		bool ok = false;
		uint game = 0;
	} cache;

	if (cache.game != game) {
		cache.ok = false;
		cache.game = game;
	}

	if (within(at) && flag(at)) {
		if (cache.ok && cache.at == at) {
			return &tiles[cache.offset];
		}

		// walk() usually ascends tile by tile
		if (cache.ok && cache.offset+1 < tiles.size() && (tiles.begin()+cache.offset+1)->at() == at) {
			cache.at = at;
			cache.offset++;
			return &tiles[cache.offset];
		}

		auto low = tiles.begin();
		auto high = tiles.end();

		if (cache.ok && cache.at < at) {
			low = low+cache.offset;
		}

		if (cache.ok && cache.at > at) {
			high = high+cache.offset;
		}

		Tile key;
		key.x = at.x;
		key.y = at.y;

		auto it = std::lower_bound(low, high, key, [](const auto& a, const auto& b) {
			return a.at() < b.at();
		});

		// lower_bound returns the location where the item *should* be.
		// It's safe to assume !end() means the tile was found because:
		// - the within() and flag() checks above say the tile must exist
		// - the tiles vector must not contain duplicates
		if (it != high) {
			ensure(it->at() == at);
			cache.at = at;
			cache.ok = true;
			cache.offset = it - tiles.begin();
			return &tiles[cache.offset];
		}
	}
	return nullptr;
}

World::Tile* World::get(const Point& p) {
	return get((XY){(int)std::floor(p.x), (int)std::floor(p.z)});
}

World::Region World::region(const Box& b) {
	Region region;
	region.box = b;

	for (auto at: walk(region.box)) {
		auto tile = get(at);
		if (!tile) continue;
		region.tiles.push_back(*tile);
	}

	std::sort(region.tiles.begin(), region.tiles.end(), [](const auto& a, const auto& b) {
		return a.at() < b.at();
	});

	return region;
}

World::Tile* World::Region::get(const XY& at) {
	if (world.within(at) && world.flag(at)) {
		if (cache.ok && cache.at == at) {
			return &tiles[cache.offset];
		}

		// walk() usually ascends tile by tile
		if (cache.ok && cache.offset+1 < tiles.size() && (tiles.begin()+cache.offset+1)->at() == at) {
			cache.at = at;
			cache.offset++;
			return &tiles[cache.offset];
		}

		auto low = tiles.begin();
		auto high = tiles.end();

		if (cache.ok && cache.at < at) {
			low = low+cache.offset;
		}

		if (cache.ok && cache.at > at) {
			high = high+cache.offset;
		}

		Tile key;
		key.x = at.x;
		key.y = at.y;

		auto it = std::lower_bound(low, high, key, [](const auto& a, const auto& b) {
			return a.at() < b.at();
		});

		if (it != high && it->at() == at) {
			cache.at = at;
			cache.ok = true;
			cache.offset = it - tiles.begin();
			return &tiles[cache.offset];
		}
	}
	return nullptr;
}

float World::elevation(const XY& at) {
	auto tile = get(at);
	return tile ? tile->elevation: 0;
}

float World::Region::elevation(const XY& at) {
	auto tile = get(at);
	return tile ? tile->elevation: 0;
}

uint World::resource(const XY& at) {
	auto tile = get(at);
	return tile ? tile->resource: 0;
}

uint World::Region::resource(const XY& at) {
	auto tile = get(at);
	return tile ? tile->resource: 0;
}

bool World::isLand(const XY& at) {
	return within(at) && !isLake(at) && !isHill(at);
}

bool World::Region::isLand(const XY& at) {
	return world.within(at) && !isLake(at) && !isHill(at);
}

bool World::isLand(const Box& b) {
	for (auto at: walk(b)) {
		if (!isLand(at)) return false;
	}
	return true;
}

bool World::isLake(const XY& at) {
	return within(at) && elevation(at) < -0.001;
}

bool World::Region::isLake(const XY& at) {
	return world.within(at) && elevation(at) < -0.001;
}

bool World::isLake(const Box& b) {
	for (auto at: walk(b)) {
		if (!isLake(at)) return false;
	}
	return true;
}

bool World::isHill(const XY& at) {
	return within(at) && elevation(at) > 0.001;
}

bool World::Region::isHill(const XY& at) {
	return world.within(at) && elevation(at) > 0.001;
}

bool World::isHill(const Box& b) {
	for (auto at: walk(b)) {
		if (!isHill(at)) return false;
	}
	return true;
}

float World::hillPlatform(const Box& box) {
	float h = -1.0f;
	for (auto xy: walk(box)) {
		if (!isHill(xy)) return 0.0f;
		float e = elevation(xy);
		h = h < 0.0f ? e: std::min(e, h);
	}
	return std::round(std::max(h, 0.0f));
}

Stack World::mine(const Box& b, uint iid) {
	for (auto at: walk(b)) {
		auto tile = get(at);
		if (tile && tile->hill()) {
			auto& hill = features[tile->feature];
			ensure(hill.id > 0);

			if (hill.mining.offsets.has(iid)) {
				auto& offset = hill.mining.offsets[iid].size;

				while (offset < hill.tiles.size()) {
					auto tile = &tiles[hill.tiles[offset]];

					if (tile->resource == iid && tile->count > 0) {
						ensure(hill.mining.stacks[iid].size > 0);
						hill.mining.stacks[iid].size--;
						tile->count--;
						return {iid,1};
					}

					offset++;
				}
			}
		}
	}
	return {0,0};
}

uint World::countMine(const Box& b, uint iid) {
	uint sum = 0;
	localset<int> seen;
	for (auto at: walk(b)) {
		auto tile = get(at);
		if (tile && tile->hill()) {
			auto& hill = features[tile->feature];
			ensure(hill.id > 0);
			if (seen.has(hill.id)) continue;
			seen.insert(hill.id);
			if (hill.mining.stacks.has(iid)) {
				sum += hill.mining.stacks[iid].size;
			}
		}
	}
	return sum;
}

bool World::canMine(const Box& b, uint iid) {
	return countMine(b, iid) > 0;
}

Amount World::drill(const Box& b, uint fid) {
	for (auto at: walk(b)) {
		auto tile = get(at);
		if (tile && tile->lake()) {
			auto& lake = features[tile->feature];
			ensure(lake.id < 0);

			if (lake.drilling.offsets.has(fid)) {
				auto& offset = lake.drilling.offsets[fid].size;

				while (offset < lake.tiles.size()) {
					auto tile = &tiles[lake.tiles[offset]];

					if (tile->resource == fid && tile->count > 0) {
						ensure(lake.drilling.amounts[fid].size > 0);
						lake.drilling.amounts[fid].size--;
						tile->count--;
						return {fid,100};
					}

					offset++;
				}
			}
		}
	}
	return {0,0};
}

uint World::countDrill(const Box& b, uint fid) {
	uint sum = 0;
	localset<int> seen;
	for (auto at: walk(b)) {
		auto tile = get(at);
		if (tile && tile->lake()) {
			auto& lake = features[tile->feature];
			ensure(lake.id < 0);
			if (seen.has(lake.id)) continue;
			seen.insert(lake.id);
			if (lake.drilling.amounts.has(fid)) {
				sum += lake.drilling.amounts[fid].size;
			}
		}
	}
	return sum;
}

bool World::canDrill(const Box& b, uint fid) {
	return countDrill(b, fid) > 0;
}

localvec<Stack> World::minables(Box b) {
	localmap<Stack,&Stack::iid> stacks;
	localset<int> seen;

	for (auto at: walk(b)) {
		auto tile = get(at);
		if (!tile) continue;
		if (!tile->hill()) continue;
		if (!tile->feature) continue; // flattened

		auto& hill = features[tile->feature];
		ensure(hill.id > 0);

		if (seen.has(hill.id)) continue;
		seen.insert(hill.id);

		for (auto& stack: hill.mining.stacks) {
			stacks[stack.iid].size += stack.size;
		}
	}

	return {stacks.begin(), stacks.end()};
}

localvec<Amount> World::drillables(Box b) {
	localmap<Amount,&Amount::fid> amounts;
	localset<int> seen;

	for (auto at: walk(b)) {
		auto tile = get(at);
		if (!tile) continue;
		if (!tile->lake()) continue;
		if (!tile->feature) continue; // flattened

		auto& lake = features[tile->feature];
		ensure(lake.id < 0);

		if (seen.has(lake.id)) continue;
		seen.insert(lake.id);

		for (auto& amount: lake.drilling.amounts) {
			amounts[amount.fid].size += amount.size;
		}
	}

	return {amounts.begin(), amounts.end()};
}


