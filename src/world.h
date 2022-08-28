#pragma once

/*

The World map is:

* pregenerated from noise as a square of World::size
* tiles with elevation > 0 are hills containing ore
* tiles with elevation < 0 are lakes containing oil
* everything else is flat land without resources

Only hill/lake tiles actually exist and consume memory. Flat tiles
are not stored and implicitly use zero/nothing field values. Tiles
may only contain a single resource each.

Tiles are generated using OpenSimplex noise layers to create a
height map which is then compressed in the middle of the range to
create large areas of flat land with discrete hills and lakes.
Ore and Oil are generated using additional noise layers with
each tile receiving the resource with the highest local value.

Extant tiles are stored in World::tiles in XY sorted order and
retrieved via a binary search in World::get(). Tiles flattened or
exhausted remain until the game is reloaded.

Tiles are grouped into contiguous features, either hills or lakes,
using a flood-fill process. Features track aggregated resources for
their tile group. Crafter entities that can mine or drill consume
resources via the feature rather than directly from tiles.
Conceptually, miners can tunnel anywhere in a hill and oil rigs
drill down to a shared deposit under a lake.

Tile updates -- flatten, mine, drill -- occur while the Crafter and
Pile systems are running which is protected by the Sim::lock. Outside
that tiles and features are immutable and accessible by multiple threads.

Tile flattening occurs when piles are built on lakes and explosives
detonated on hills. Any remaining resources are lost. Tile changes
are logged in World::changes so that the renderer knows when to
regenerate map chunk terrain meshes. There is no concept of a chunk
at the world or simulation level.

*/

#include "item.h"
#include "fluid.h"
#include "gridwalk.h"
#include "box.h"
#include "sphere.h"
#include "miniset.h"
#include "minimap.h"
#include "slabmap.h"
#include "sim.h"
#include <initializer_list>

struct World {
	typedef gridwalk::xy XY;
	gridwalk walk(const Box& box);

	static inline int sizeMin = 1024;
	static inline int sizeMax = 8192;

	struct {
		int size = 0;
	} scenario;

	int size();
	Box box();

	uint64_t memory();

	struct Tile {
		float elevation = 0;
		int16_t x = 0;
		int16_t y = 0;
		int16_t feature = 0;
		uint16_t resource = 0;
		uint16_t count = 0;

		World::XY at() const {
			return {x,y};
		}

		bool hill() const {
			return elevation > 0.001;
		}

		bool lake() const {
			return elevation < -0.001;
		}

		bool land() const {
			return !hill() && !lake();
		}
	};

	struct Feature {
		int id = 0;
		minivec<uint> tiles;
		struct {
			minimap<Stack,&Stack::iid> stacks;
			minimap<Stack,&Stack::iid> offsets;
		} mining;
		struct {
			minimap<Amount,&Amount::fid> amounts;
			minimap<Amount,&Amount::fid> offsets;
		} drilling;
	};

	struct Change {
		XY at = {0,0};
		uint64_t tick = 0;
	};

	struct {
		std::vector<uint8_t> tiles;
	} flags;

	std::vector<Tile> tiles;
	std::map<int,Feature> features;
	std::deque<Change> changes;
	bool ready = false;

	// used by thread_local caches to reset
	uint game = 0;

	void save(const char* path, channel<bool,3>* tickets);
	void load(const char* path);

	bool flag(const XY& at, const uint8_t* buf = nullptr);
	bool flag(const XY& at, bool state, uint8_t* buf = nullptr);

	Tile* get(const XY& at);
	Tile* get(const Point& p);

	int nextHill = 1;
	int nextLake = -1;

	void init();
	void reset();
	void index();
	void detect();

	bool within(const XY& at);

	void flood(const XY& at, int id);
	void flatten(const Box& b);
	float blast(const Sphere& s);

	float elevation(const XY& at);
	uint resource(const XY& at);

	bool isLand(const XY& at);
	bool isLand(const Box& b);
	bool isLake(const XY& at);
	bool isLake(const Box& b);
	bool isHill(const XY& at);
	bool isHill(const Box& b);

	float hillPlatform(const Box& b);

	Stack mine(const Box& b, uint iid);
	bool canMine(const Box& b, uint iid);
	uint countMine(const Box& b, uint iid);
	Amount drill(const Box& b, uint fid);
	bool canDrill(const Box& b, uint fid);
	uint countDrill(const Box& b, uint fid);

	std::vector<Stack> minables(Box b);
	std::vector<Amount> drillables(Box b);

	struct Region {
		std::vector<Tile> tiles;
		Box box;
		World::Tile* get(const XY& at);
		float elevation(const XY& at);
		uint resource(const XY& at);
		bool isLand(const XY& at);
		bool isLake(const XY& at);
		bool isHill(const XY& at);
		struct {
			World::XY at = {0,0};
			uint offset = 0;
			bool ok = false;
		} cache;
	};

	Region region(const Box& b);
};

extern World world;
