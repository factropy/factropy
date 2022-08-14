#pragma once

// Entities consist of parts which are rendered using OpenGL instancing, with colors and
// transformations supplied in bulk. Parts may have behaviours like spinning fans or
// particle emitters or moving foliage.

struct Part;

#include "mesh.h"
#include "color.h"
#include "mat4.h"
#include "spec.h"
#include "recipe.h"
#include <map>
#include <vector>

struct Part {
	static inline std::set<Part*> all;
	static void reset();
	static void resetAll();

	enum Detail {
		HD = 0,
		MD = 1,
		LD = 2,
		VLD = 3,
	};

	struct LOD {
		Mesh* mesh = nullptr;
		enum Detail detail = HD;
		bool shadow = true;
	};

	std::vector<LOD> levels;

	GLfloat shine = 0.0f;

	Mat4 srt = Mat4::identity;
	// The components used to generate srt may be reused
	// for automatic multi-part/mesh generation for items
	// on belts, and it's simpler to store these than
	// decompose the matrix later. For anything that uses
	// transform() directly these should be avoided or
	// set manually.
	Point scale = {1,1,1};
	Point translate = {0,0,0};
	Point rotateAxis = {0,1,0};
	float rotateDegrees = 0.0f;

	Color color;
	bool glow = false;
	bool ghost = false;
	bool autoColor = false;
	bool customColor = false;
	uint filter = 0;

	// info for GuiEntity
	static inline const uint Azimuth = (1<<0);
	static inline const uint Altitude = (1<<1);

	uint pivot = 0;
	Point pivotBump = Point::Zero;
	Part* pivots(uint p, Point bump = Point::Zero);

	static inline const bool SHADOW = true;
	static inline const bool NOSHADOW = false;

	Part(const Color& c);
	virtual ~Part();
	bool shadow();
	Part* gloss(float s);
	Part* translucent();
	Part* lod(Mesh* mesh, Detail detail, bool shadow = NOSHADOW);
	virtual void update();
	Part* transform(const Mat4& ssrt);
	virtual Mat4 transform();
	virtual bool show(Point pos, Point dir);
	LOD* distanceLOD(float distance);
	void instance(GLuint group, float distance, const Mat4& trx);
	void instance(GLuint group, float distance, const Mat4& trx, const Color& color, bool shadow = true);

	bool specShow(Spec* spec, uint slot, uint state);
	Mat4 specState(Spec* spec, uint slot, uint state);
	Mat4 aimState(Point aim);
	virtual void instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe);

	Part* multi(const std::vector<Point>& offsets);
};

struct PartSpinner : Part {
	Mat4 rot = Mat4::identity;
	Point axis = Point::Up;
	float speed = 1;

	PartSpinner(Color c, Point axis = Point::Up, float speed = 1.0f);
	void update() override;
	Mat4 transform() override;
	void instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) override;
};

struct PartCycle : Part {
	std::vector<Mat4> transforms;
	Mat4 shunt = Mat4::identity;

	PartCycle(Color c, std::vector<Mat4> t);
	void update() override;
	Mat4 transform() override;
};

struct PartSmoke : Part {

	struct Particle {
		Point offset;
		Point spread;
		float tickDV;
		uint64_t life;
	};

	int particlesMax;
	int particlesPerTick;
	float particleRadius;
	float emitRadius;
	float spreadFactor;
	float tickDH;
	float tickDV;
	float tickDecay;
	uint lifeLower;
	uint lifeUpper;
	std::vector<Particle> particles;
	Point up = Point::Up;
	Point south = Point::South;

	PartSmoke(Color color, Point up, int particlesMax, int particlesPerTick, float particleRadius, float emitRadius, float spreadFactor, float tickDH, float tickDV, float tickDecay, uint lifeLower, uint lifeUpper);

	void instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) override;
	void update() override;
};

struct PartRecipeFluid : Part {
	PartRecipeFluid(Color c = 0xccccffff);
	void instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) override;
};

struct PartGridRepeat : Part {
	int interval = 5;

	PartGridRepeat(Color c, int i);
	bool show(Point pos, Point dir) override;
};

struct PartFlame : Part {
	struct Flame {
		Point scale;
		Color color;
	};

	Flame flames[3];

	PartFlame();

	void instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) override;
};

struct PartTree : Part {
	PartTree(Color c);
	void instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) override;
};

