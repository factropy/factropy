#include "common.h"
#include "chunk.h"
#include "point.h"
#include "mesh.h"
#include "part.h"
#include "sim.h"
#include "flate.h"
#include <fstream>
#include <filesystem>

// Chunks are large terrain meshes with several levels of detail

std::size_t Chunk::memory() {
	std::size_t mem = 0;
	const std::lock_guard<std::mutex> lock(looking);

	for (auto& [xy,chunk]: all) {
		mem += sizeof(Chunk);
		mem += chunk->heightmap->memory();
		mem += chunk->heightmapLD->memory();
		mem += chunk->heightmapVLD->memory();
		mem += chunk->drawOnHill.memory();
	}

	mem += Terrain::normals.size() * sizeof(Terrain::normals[0]);

	return mem;
}

void Chunk::reset() {
	for (auto [_,chunk]: all) delete chunk;
	all.clear();
	deleted.clear();
	generating.clear();
	sequence = 0;
}

uint Chunk::prepare() {
	uint jobs = 0;

	Sim::locked([&]() {
		const std::lock_guard<std::mutex> lock(looking);

		if (sequence) {
			jobs = generating.size();
			return;
		}

		for (auto at: gridwalk(size, world.box()).spiral()) {
			Generation generation = {
				.at = at,
				.id = sequence++,
				.chunk = new Chunk(at.x, at.y),
			};

			generating.push_back(generation);
			generation.chunk->version = Sim::tick;

			async.job([generation]() {
				auto chunk = generation.chunk;
				auto at = generation.at;

				chunk->generate();
				chunk->tickLastViewed = 0;
				chunk->tickLastViewedLD = 0;
				chunk->tickLastViewedVLD = 0;

				const std::lock_guard<std::mutex> lock(looking);

				if (!all.count(at)) {
					all[at] = chunk;
				}
				else
				if (all[at]->version <= chunk->version) {
					deleted[all[at]] = Sim::tick+60;
					all[at] = chunk;
				}
				else {
					delete chunk;
				}

				generating.erase(generation.id);
			});
		}

		jobs = generating.size();
	});
	return jobs;
}

void Chunk::tick() {
	Sim::locked([&]() {
		const std::lock_guard<std::mutex> lock(looking);

		// last frame
		miniset<Chunk*> dropChunks;
		for (auto [chunk,tick]: deleted) {
			if (tick <= Sim::tick) {
				dropChunks.insert(chunk);
			}
		}
		for (auto chunk: dropChunks) {
			deleted.erase(chunk);
			delete chunk;
		}

		minivec<XY> jobs;

		auto enqueue = [&](XY at) {
			for (auto& gen: generating) {
				if (gen.at == at && gen.chunk->version >= Sim::tick) return;
			}
			for (auto& jat: jobs) {
				if (jat == at) return;
			}
			jobs.push_back(at);
		};

		for (auto& change: world.changes) {
			int x = change.at.x/size;
			if (change.at.x < 0) x--;
			int y = change.at.y/size;
			if (change.at.y < 0) y--;
			XY at = {x,y};

			// when the tile is right on the edge of a mesh,
			/// ensure both sides get renegerated
			for (auto tat: world.walk(at.centroid().box().grow(1))) {
				if (!all.count(tat)) continue;
				enqueue(at);
			}
		}

		for (auto at: jobs) {
			Generation generation = {
				.at = at,
				.id = sequence++,
				.chunk = new Chunk(at.x, at.y),
			};

			generating.push_back(generation);
			generation.chunk->version = Sim::tick;

			async.job([generation]() {
				auto chunk = generation.chunk;
				auto at = generation.at;

				chunk->generate();
				chunk->tickLastViewed = chunk->version;
				chunk->tickLastViewedLD = chunk->version;
				chunk->tickLastViewedVLD = chunk->version;

				const std::lock_guard<std::mutex> lock(looking);

				if (!all.count(at)) {
					all[at] = chunk;
				}
				else
				if (all[at]->version <= chunk->version) {
					deleted[all[at]] = Sim::tick+60;
					all[at] = chunk;
				}
				else {
					delete chunk;
				}

				generating.erase(generation.id);
			});
		}

		world.changes.clear();
	});
}

Chunk::Chunk(int xx, int yy) {
	x = xx;
	y = yy;
	loadedToVRAM = false;
	loadedToVRAMLD = false;
	loadedToVRAMVLD = false;
	generated = false;
	hasWater = false;
	tickLastViewed = 0;
	tickLastViewedLD = 0;
	tickLastViewedVLD = 0;

	// grow, so there's no gap
	region = world.region(box().grow(1));
}

Chunk::~Chunk() {
	delete heightmap;
	delete heightmapLD;
	delete heightmapVLD;
}

Point Chunk::centroid() {
	return Point(x*size, 0, y*size) + Point(size/2,0,size/2);
}

Box Chunk::box() {
	return centroid().box().grow(size/2);
}

Chunk* Chunk::request(int x, int y) {
	int xt = x*size+(size/2);
	int yt = y*size+(size/2);
	if (!world.within({xt,yt})) return nullptr;

	const std::lock_guard<std::mutex> lock(looking);

	Chunk* found = nullptr;
	XY xy = {x,y};

	if (all.count(xy)) {
		found = all[xy];
	}

	return found;
}

Mat4 Chunk::transformation(Point offset) {
	return (Point((real)(x*size)+0.5f, -50.0, (real)(y*size)+0.5f) + offset).translation();
}

Mat4 Chunk::transformationLD(Point offset) {
	return Mat4::scale(2.0f, 1.0f, 2.0f) * transformation(offset);
}

Mat4 Chunk::transformationVLD(Point offset) {
	return Mat4::scale(4.0f, 1.0f, 4.0f) * transformation(offset);
}

Mat4 Chunk::transformationWater(Point offset) {
	return (Point((real)(x*size)+(size/2)+0.5f, -3.0, (real)(y*size)+(size/2)+0.5f) + offset).translation();
}

void Chunk::generate() {

	for (int ty = 0; ty < size; ty++) {
		for (int tx = 0; tx < size; tx++) {
			double offset = 1000000;

			float hint = (float)Sim::noise2D(x*size+tx+offset, y*size+ty+offset, 8, 0.9, 0.9);
			offset += 1000000;

			XY at = {x*size+tx, y*size+ty};
			auto tile = region.get(at);

			bool hill = tile ? tile->hill(): false;
			bool lake = tile ? tile->lake(): false;
			bool mineral = hill ? tile->resource: 0;

			// minables visible on hills
			if (hill && mineral && hint > 0.75) {
				drawOnHill.insert({tx,ty});
			}

			// tell camera to draw an animated water layer for the chunk
			hasWater = hasWater || lake;
		}
	}

	Terrain terrain(this);
	heightmap = terrain.hd();
	heightmapLD = terrain.ld();
	heightmapVLD = terrain.vld();
}

void Chunk::autoload() {
	// HD mesh moved out of view for a while
	if (Sim::tick > 90 && tickLastViewed < Sim::tick-90 && loadedToVRAM) {
		heightmap->unload();
		loadedToVRAM = false;
	}

	// LD mesh moved out of view for a while
	if (Sim::tick > 90 && tickLastViewedLD < Sim::tick-90 && loadedToVRAMLD) {
		heightmapLD->unload();
		loadedToVRAMLD = false;
	}

	// VLD mesh moved out of view for a while
	if (Sim::tick > 90 && tickLastViewedVLD < Sim::tick-90 && loadedToVRAMVLD) {
		heightmapVLD->unload();
		loadedToVRAMVLD = false;
	}

	// HD mesh moved into view
	if (Sim::tick > 90 && tickLastViewed >= Sim::tick-90 && !loadedToVRAM) {
		heightmap->load();
		loadedToVRAM = true;
	}

	// LD mesh moved into view
	if (Sim::tick > 90 && tickLastViewedLD >= Sim::tick-90 && !loadedToVRAMLD) {
		heightmapLD->load();
		loadedToVRAMLD = true;
	}

	// VLD mesh moved into view
	if (Sim::tick > 90 && tickLastViewedVLD >= Sim::tick-90 && !loadedToVRAMVLD) {
		heightmapVLD->load();
		loadedToVRAMVLD = true;
	}

	// HD mesh moved into view at startup
	if (Sim::tick < 90 && tickLastViewed > 0 && !loadedToVRAM) {
		heightmap->load();
		loadedToVRAM = true;
	}

	// LD mesh moved into view at startup
	if (Sim::tick < 90 && tickLastViewedLD > 0 && !loadedToVRAMLD) {
		heightmapLD->load();
		loadedToVRAMLD = true;
	}

	// VLD mesh moved into view at startup
	if (Sim::tick < 90 && tickLastViewedVLD > 0 && !loadedToVRAMVLD) {
		heightmapVLD->load();
		loadedToVRAMVLD = true;
	}
}

bool Chunk::discardable() {
	if (Sim::tick < 600) return false;
	bool stagnant = (!tickLastViewed || tickLastViewed < Sim::tick-600) && !loadedToVRAM;
	bool stagnantLD = (!tickLastViewedLD || tickLastViewedLD < Sim::tick-600) && !loadedToVRAMLD;
	bool stagnantVLD = (!tickLastViewedVLD || tickLastViewedVLD < Sim::tick-600) && !loadedToVRAMVLD;
	return stagnant && stagnantLD && stagnantVLD;
}

Chunk::Terrain::Terrain(Chunk* cchunk) {
	chunk = cchunk;
}

std::vector<glm::vec3> Chunk::Terrain::noiseData(int x, int y) {
	int edge = size+1;
	std::vector<glm::vec3> out(edge*edge);

	ensure(size == 128);
	ensure(World::sizeMax/size/2 == 32);

	x += World::sizeMax/size/2;
	y += World::sizeMax/size/2;

	for (int ty = 0; ty < edge; ty++) {
		for (int tx = 0; tx < edge; tx++) {
			int n = (y*size + ty) * World::sizeMax + (x*size + tx);
			out[ty*edge+tx] = normals[n];
		}
	}

	return out;
}

void Chunk::Terrain::noiseGen() {
	int edge = World::sizeMax;

	normals.resize((edge+1)*(edge+1));
	trigger done;

	for (int ty = 0; ty <= edge; ty++) {
		async.job([&,ty]() {
			for (int tx = 0; tx <= edge; tx++) {
				float bx = (float)Sim::noise2D(tx+1000, ty+1000, layers, persistenceA, frequencyA) + 0.5
					+ (float)Sim::noise2D(tx+1000, ty+1000, layers, persistenceB, frequencyB) + 0.5;
				float by = (float)Sim::noise2D(tx+2000, ty+2000, layers, persistenceA, frequencyA) + 0.5
					+ (float)Sim::noise2D(tx+2000, ty+2000, layers, persistenceB, frequencyB) + 0.5;
				float bz = (float)Sim::noise2D(tx+3000, ty+3000, layers, persistenceA, frequencyA) + 0.5
					+ (float)Sim::noise2D(tx+3000, ty+3000, layers, persistenceB, frequencyB) + 0.5;
				normals[ty*edge+tx] = glm::normalize(glm::vec3(bx, by, bz));
			}
			done.now();
		});
	}
	done.wait(edge);

	struct n3 {
		int8_t x, y, z;

		bool operator==(const n3& o) const {
			return x == o.x && y == o.y && z == o.z;
		}

		bool operator<(const n3& o) const {
			return x < o.x || (x == o.x && y < o.y) || (x == o.x && y == o.y && z < o.z);
		}
	};

	std::vector<n3> raw;

	for (auto& normal: normals) {
		raw.push_back({
			.x = (int8_t)std::round(normal.x*noiseScale),
			.y = (int8_t)std::round(normal.y*noiseScale),
			.z = (int8_t)std::round(normal.z*noiseScale),
		});
	}

	std::vector<n3> all = {raw.begin(), raw.end()};
	std::sort(all.begin(), all.end());
	all.erase(std::unique(all.begin(), all.end()), all.end());

	ensure(all.size() < 128);

	std::map<n3,int8_t> index;

	for (int i = 0, l = all.size(); i < l; i++) {
		index[all[i]] = i;
	}

	std::vector<int8_t> indexed;

	for (auto& v: raw) {
		indexed.push_back(index[v]);
	}

	auto blurtA = std::ofstream("assets/terrain.noise.a", std::ios::binary);
	blurtA.write((const char*)all.data(), all.size()*sizeof(n3));
	blurtA.flush();
	blurtA.close();

	deflation def;
	def.data = {indexed.begin(), indexed.end()};
	def.save("assets/terrain.noise.b");
}

void Chunk::Terrain::noiseLoad() {
	ensuref(std::filesystem::exists("assets/terrain.noise.a"), "missing assets/terrain.noise.a");
	ensuref(std::filesystem::exists("assets/terrain.noise.b"), "missing assets/terrain.noise.b");

	int edge = World::sizeMax;
	normals.clear();

	struct n3 {
		int8_t x, y, z;
	};

	std::vector<n3> inA(std::filesystem::file_size("assets/terrain.noise.a")/sizeof(n3));

	auto slurpA = std::ifstream("assets/terrain.noise.a", std::ios::binary);
	slurpA.read((char*)inA.data(), inA.size()*sizeof(n3));
	slurpA.close();

	inflation inf;
	inf.load("assets/terrain.noise.b");
	ensure((int)inf.data.size() == (edge+1)*(edge+1));

	for (auto& idx: inf.data) {
		ensure(idx >= 0 && idx < (int)inA.size());
		glm::vec3 n = {((float)inA[idx].x)/noiseScale, ((float)inA[idx].y)/noiseScale, ((float)inA[idx].z)/noiseScale};
		normals.push_back(glm::normalize(n));
	}
}

Mesh* Chunk::Terrain::hd() {
	int size = Chunk::size;

	// When generating a Mesh from a heightmap each pixel becomes a vertex on a tile centroid.
	// Without the +1 there would be gaps on the +X and +Y edges.
	int edge = size+1;

	std::vector<float> elevations;
	elevations.resize(edge*edge);

	for (int y = 0; y < edge; y++) {
		for (int x = 0; x < edge; x++) {
			elevations[y*edge+x] = chunk->region.elevation((XY){chunk->x*Chunk::size+x, chunk->y*Chunk::size+y});
		}
	}

	Mesh* mesh = new MeshHeightMap(edge, elevations.data(), 50.0f);
	terrainSmooth(mesh, 1.0, Chunk::size, edge, elevations, noiseData(chunk->x,chunk->y), darkness);

	return mesh;
}

Mesh* Chunk::Terrain::ld() {
	int size = Chunk::size/2;

	// When generating a Mesh from a heightmap each pixel becomes a vertex on a tile centroid.
	// Without the +1 there would be gaps on the +X and +Y edges.
	int edge = size+1;

	std::vector<float> elevations;
	elevations.resize(edge*edge);

	for (int y = 0; y < edge; y++) {
		for (int x = 0; x < edge; x++) {
			elevations[y*edge+x] = chunk->region.elevation((XY){chunk->x*Chunk::size+(x*2), chunk->y*Chunk::size+(y*2)});
		}
	}

	Mesh* mesh = new MeshHeightMap(edge, elevations.data(), 50.0f);
	terrainSmooth(mesh, 0.5, Chunk::size, edge, elevations, noiseData(chunk->x,chunk->y), darknessLD);

	return mesh;
}

Mesh* Chunk::Terrain::vld() {
	int size = Chunk::size/4;

	// When generating a Mesh from a heightmap each pixel becomes a vertex on a tile centroid.
	// Without the +1 there would be gaps on the +X and +Y edges.
	int edge = size+1;

	std::vector<float> elevations;
	elevations.resize(edge*edge);

	for (int y = 0; y < edge; y++) {
		for (int x = 0; x < edge; x++) {
			elevations[y*edge+x] = chunk->region.elevation((XY){chunk->x*Chunk::size+(x*4), chunk->y*Chunk::size+(y*4)});
		}
	}

	Mesh* mesh = new MeshHeightMap(edge, elevations.data(), 50.0f);
	terrainSmooth(mesh, 0.25, Chunk::size, edge, elevations, noiseData(chunk->x,chunk->y), darknessVLD);

	return mesh;
}

void Chunk::terrainSmooth(Mesh* mesh, float scale, int size, int edge, const std::vector<float>& elevation, const std::vector<glm::vec3>& noise, float darkness) {
	for (int i = 0, l = mesh->vertices.size(); i < l; i++) {
		Point v = mesh->vertices[i];
		Point n = mesh->normals[i];

		if (n == Point::Up) {
			int ny = std::clamp((int)(v.z / (double)scale), 0, size);
			int nx = std::clamp((int)(v.x / (double)scale), 0, size);
			mesh->normals[i] = glm::normalize(noise[ny*(size+1)+nx] + glm::vec3(0,darkness,0));
		}
	}

	mesh->smooth();
}
