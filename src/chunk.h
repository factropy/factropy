#pragma once

// Chunks are large terrain meshes with several levels of detail

struct Chunk;

#include "world.h"
#include "miniset.h"
#include "scene.h"
#include <mutex>

struct Chunk {
	typedef gridwalk::xy XY;

	static const int size = 128;

	struct Terrain {
		static inline int layers = 4;
		// decrease persistence to make edges smoother
		// increase frequency to make blobs smaller
		static inline float persistenceA = 0.5; // small scale ground pattern
		static inline float frequencyA = 0.5; // small scale ground pattern
		static inline float persistenceB = 0.3; // large scale ground pattern
		static inline float frequencyB = 0.04; // large scale ground pattern

		// ground noise is done by deflecting normals with noise.
		// smoothing on lower LOD chunks reduces the effect making terrain
		// appear lighter, so we compensate by deflecting distant chunk
		// normals by vec3(0,darknessLD,0)
		//float darkness = 0.0;
		static inline float darkness = 0.1;
		static inline float darknessLD = -0.15;
		static inline float darknessVLD = -0.25;

		Chunk* chunk = nullptr;
		Terrain(Chunk* chunk);
		Mesh* hd();
		Mesh* ld();
		Mesh* vld();

		// Noise layers used for deflecting normals on flat terrain to
		// create grass patches and shading. If mesh simplification is
		// introduced for flat terrain this will need a rethink.
		static std::vector<glm::vec3> noiseData(int x, int y);
		static inline std::vector<glm::vec3> normals;
		static inline float noiseScale = 10.0f;
		static void noiseGen();
		static void noiseLoad();
	};

	static inline std::mutex looking;
	static inline std::map<XY,Chunk*> all;
	static inline std::map<Chunk*,uint64_t> deleted;

	static inline uint sequence = 0;

	struct Generation {
		XY at;
		uint id;
		Chunk* chunk;
	};

	static inline minimap<Generation,&Generation::id> generating;

	static void reset();
	static std::size_t memory();
	static Chunk* request(int x, int y);

	int x = 0, y = 0;
	uint64_t version = 0;
	Mesh* heightmap;
	Mesh* heightmapLD;
	Mesh* heightmapVLD;
	bool generated = false;
	bool loadedToVRAM = false;
	bool loadedToVRAMLD = false;
	bool loadedToVRAMVLD = false;
	uint64_t tickLastViewed = 0;
	uint64_t tickLastViewedLD = 0;
	uint64_t tickLastViewedVLD = 0;
	miniset<XY> drawOnHill;
	bool hasWater = false;
	World::Region region;

	Chunk(int x, int y);
	~Chunk();
	void generate();
	void autoload();
	Point centroid();
	Box box();
	bool discardable();
	Mat4 transformation(Point offset);
	Mat4 transformationLD(Point offset);
	Mat4 transformationVLD(Point offset);
	Mat4 transformationWater(Point offset);

	static uint prepare();

	static void tickPurge();
	static void tickChange();
	static void terrainSmooth(Mesh* mesh, float scale, int size, int edge, const std::vector<float>& elevation, const std::vector<glm::vec3>& noise, float darkness);
};
