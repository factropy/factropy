#include "part.h"
#include "common.h"
#include "sim.h"
#include "scene.h"
#include "config.h"

// Entities consist of parts which are rendered using OpenGL instancing, with colors and
// transformations supplied in bulk. Parts may have behaviours like spinning fans or
// particle emitters or moving foliage.

Part::Part(const Color& c) {
	color = c;
}

Part::~Part() {
}

void Part::resetAll() {
}

Part* Part::gloss(float s) {
	shine = s;
	return this;
}

Part* Part::translucent() {
	ghost = true;
	return this;
}

Part* Part::transform(const Mat4& ssrt) {
	srt = ssrt;
	return this;
}

Mat4 Part::transform() {
	return srt;
}

Part* Part::pivots(uint p, Point bump) {
	pivot = p;
	pivotBump = bump;
	return this;
}

Part* Part::lod(Mesh* mesh, Part::Detail detail, bool shadow) {
	levels.push_back({.mesh = mesh, .detail = detail, .shadow = shadow});
	return this;
}

Part* Part::multi(const std::vector<Point>& offsets) {
	auto multi = new Part(color);
	multi->shine = shine;
	multi->srt = srt;
	multi->glow = glow;
	multi->ghost = ghost;
	std::vector<glm::vec3> bumps = {offsets.begin(), offsets.end()};
	for (auto& level: levels) {
		multi->lod(new MeshMulti(level.mesh, bumps), level.detail, level.shadow);
	}
	return multi;
}

Part::LOD* Part::distanceLOD(float distance) {
	for (int i = 0, l = levels.size(); i < l; i++) {
		if (distance < (float)Config::window.levelsOfDetail[levels[i].detail]) {
			return &levels[i];
		}
	}
	return nullptr;
}

void Part::update() {
}

void Part::instance(GLuint group, float distance, const Mat4& trx) {
	instance(group, distance, trx, color);
}

void Part::instance(GLuint group, float distance, const Mat4& trx, const Color& color, bool shadow) {
	auto lod = distanceLOD(distance);
	if (lod) lod->mesh->instance(group, transform() * trx, color, shine, shadow ? lod->shadow: false, Config::mode.filters ? filter: 0);
}

bool Part::specShow(Spec* spec, uint slot, uint state) {
	if (spec->statesShow.size() > 0) {
		ensure(spec->statesShow.size() > state);
		ensure(spec->statesShow[state].size() > slot);
		return spec->statesShow[state][slot];
	}
	return true;
}

Mat4 Part::specState(Spec* spec, uint slot, uint state) {
	if (spec->states.size() == 0) {
		ensure(state == 0);
	}
	if (spec->states.size() > 0) {
		ensure(spec->states.size() > state);
		ensure(spec->states[state].size() > slot);
		return spec->states[state][slot];
	}
	return Mat4::identity;
}

Mat4 Part::aimState(Point aim) {
	if (pivot) {
		bool azimuth = pivot & Part::Azimuth;
		bool altitude = pivot & Part::Altitude;

		if (altitude && azimuth) {
			Mat4 r1 = Point(aim.x, 0, aim.z).rotation();
			Mat4 r2 = Point(0, aim.y, Point(aim.x, 0, aim.z).length()).rotation();
			Mat4 bump = pivotBump.translation();
			return r2 * bump * r1;
		}

		if (altitude) {
			Mat4 r2 = Point(0, aim.y, Point(aim.x, 0, aim.z).length()).rotation();
			Mat4 bump = pivotBump.translation();
			return r2 * bump;
		}

		if (azimuth) {
			Mat4 r = Point(aim.x, 0, aim.z).rotation();
			Mat4 bump = pivotBump.translation();
			return r * bump;
		}
	}
	return Mat4::identity;
}

void Part::instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) {
	if (!specShow(spec, slot, state)) return;

	if (spec->states.size() > 0) {
		auto lod = distanceLOD(distance);
		if (!lod) return;

		Mat4 i = specState(spec, slot, state);
		Mat4 m = i * transform();
		if (pivot) m = m*aimState(aim);
		Mat4 t = m*trx;
		lod->mesh->instance(group, t, ccolor, shine, lod->shadow, Config::mode.filters ? filter: 0);
		return;
	}
	instance(group, distance, trx, ccolor);
}

bool Part::show(Point pos, Point dir) {
	return true;
}

PartSpinner::PartSpinner(Color c, Point aaxis, float sspeed) : Part(c) {
	axis = aaxis;
	speed = sspeed;
}

void PartSpinner::update() {
	rot = Mat4::rotate(axis, -glm::radians((double)(Sim::tick%360) * speed));
}

Mat4 PartSpinner::transform() {
	return rot * srt;
}

void PartSpinner::instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) {
	if (!specShow(spec, slot, state)) return;

	auto lod = distanceLOD(distance);
	if (!lod) return;

	Mat4 i = specState(spec, slot, state);
	Mat4 m = (rot * i) * srt;
	if (pivot) m = m*aimState(aim);
	Mat4 t = m * trx;
	lod->mesh->instance(group, t, ccolor, shine, lod->shadow, Config::mode.filters ? filter: 0);
}

PartCycle::PartCycle(Color c, std::vector<Mat4> t) : Part(c) {
	transforms = t;
}

void PartCycle::update() {
	uint i = Sim::tick % transforms.size();
	shunt = transforms[i];
}

Mat4 PartCycle::transform() {
	return shunt * srt;
}

PartSmoke::PartSmoke(Color color, Point uup, int pm, int ppt, float pr, float er, float sf, float th, float tv, float td, uint ll, uint lu) : Part(color) {
	particlesMax = pm;
	particlesPerTick = ppt;
	particleRadius = pr;
	emitRadius = er;
	spreadFactor = sf;
	tickDH = th;
	tickDV = tv;
	tickDecay = td;
	lifeLower = ll;
	lifeUpper = lu;
	up = uup;

	for (int i = 0; i < particlesMax; i++) {
		particles.push_back({
			.offset = Point::Zero,
			.spread = Point::Zero,
			.tickDV = 0,
			.life = 0,
		});
	}
}

void PartSmoke::update() {
	int create = particlesPerTick;
	uint lifeRange = lifeUpper - lifeLower;

	for (Particle& p: particles) {
		if (p.life < Sim::tick && create > 0) {
			p.life = Sim::tick + lifeLower + std::round(Sim::random() * (float)lifeRange);
			p.offset = (south * emitRadius * std::sqrt(Sim::random())).randomHorizontal();
			p.spread = p.offset * spreadFactor;
			p.tickDV = tickDV;
			create--;
		}
		if (p.life > Sim::tick) {
			p.offset += p.spread;
			p.offset += (south * (Sim::random()*tickDH)).randomHorizontal();
			p.offset += (up * ((Sim::random()*0.5+0.5)*p.tickDV));
			p.tickDV *= tickDecay;
		}
	}
}

void PartSmoke::instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) {
	if (!specShow(spec, slot, state)) return;
	if (distance > Config::window.levelsOfDetail[Part::HD]) return;

	Mat4 i = specState(spec, slot, state);
	Mat4 m = i * transform();
	if (pivot) m = m*aimState(aim);
	Mat4 t = m*trx;

	Point pos = Point::Zero.transform(t);

	float sca = Point(t.m[0][0], t.m[0][1], t.m[0][2]).length() * particleRadius;
	Mat4 scale = Mat4::scale(sca, sca, sca);

	std::vector<glm::mat4> batch;

	for (Particle& p: particles) {
		if (p.life > Sim::tick) {
			Point v = pos + p.offset;
			Mat4 translate = v.translation();
			batch.push_back(scale * translate);
		}
	}

	scene.unit.mesh.cube->instances(scene.shader.ghost.id(), batch, ccolor);
}

PartRecipeFluid::PartRecipeFluid(Color c) : Part(c) {
}

void PartRecipeFluid::instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) {
	if (!specShow(spec, slot, state)) return;

	auto lod = distanceLOD(distance);
	if (!lod) return;

	Mat4 i = specState(spec, slot, state);
	Mat4 m = i * srt;
	if (pivot) m = m*aimState(aim);
	Mat4 t = m * trx;

	Color c = recipe && recipe->fluid ? Fluid::get(recipe->fluid)->color: ccolor;

	lod->mesh->instance(group, t, c, shine, lod->shadow);
}

PartGridRepeat::PartGridRepeat(Color c, int i) : Part(c) {
	interval = i;
}

bool PartGridRepeat::show(Point pos, Point dir) {
	bool x = dir == Point::East || dir == Point::West;
	return (x && (int)std::floor(pos.x)%interval == 0) || (!x && (int)std::floor(pos.z)%interval == 0);
}

PartFlame::PartFlame() : Part(0xffffffff) {
	flames[0].scale = Point(1,1,1);
	flames[0].color = 0xffaa66ff;

	flames[1].scale = Point(1,1,1);
	flames[1].color = 0xffff00ff;

	flames[2].scale = Point(1,1,1);
	flames[2].color = 0xffaa00ff;
}

void PartFlame::instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) {
	if (!specShow(spec, slot, state)) return;
	if (group == scene.shader.ghost.id()) return;
	if (distance > Config::window.levelsOfDetail[Part::MD]) return;

	const float r1 = 0.4;
	const float r2 = 0.3;
	const float r3 = 0.2;

	const float h1 = 0.5;
	const float h2 = 0.75;
	const float h3 = 0.9;

	Mat4 d = Mat4::translate(0,-0.15,0);

	uint bump = distance/10;

	Mat4 i = specState(spec, slot, state);
	Mat4 m = i * transform();
	if (pivot) m = m*aimState(aim);
	Mat4 t = m*d*trx;

	for (int i = 0; i < 3; i++) {
		Mat4 s = Mat4::scale(flames[i].scale.x*r1, flames[i].scale.y*h1, flames[i].scale.z*r1);
		if (i == 1) s = Mat4::scale(flames[i].scale.x*r2, flames[i].scale.y*h2, flames[i].scale.z*r1);
		if (i == 2) s = Mat4::scale(flames[i].scale.x*r1, flames[i].scale.y*h3, flames[i].scale.z*r3);
		Color c = flames[i].color;
		Mat4 r = Mat4::rotate(Point::Up, glm::radians((float)i*120));
		scene.bits.flame->instance(scene.shader.flame.id(), s*r*t, c, 0, false, bump);
	}
}

PartTree::PartTree(Color c) : Part(c) {
}

void PartTree::instanceSpec(GLuint group, float distance, const Mat4& trx, Spec* spec, uint slot, uint state, Point aim, Color ccolor, Recipe* recipe) {
	if (!specShow(spec, slot, state)) return;
	if (group == scene.shader.ghost.id()) return Part::instanceSpec(group, distance, trx, spec, slot, state, aim, ccolor, recipe);

	group = distance < Config::window.levelsOfDetail[Part::MD] ? scene.shader.tree.id(): scene.shader.part.id();
	uint bump = 0; //distance/10;

	auto lod = distanceLOD(distance);
	if (lod) lod->mesh->instance(group, transform() * trx, ccolor, shine, lod->shadow, bump);
}

