#include "common.h"
#include "scene.h"
#include "config.h"
#include "gui.h"
#include "crew.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

Scene scene;

void Scene::init() {
	glGenFramebuffers(1, &shadowMapFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);

	glGenTextures(1, &shadowMapDepthTexture);
	glBindTexture(GL_TEXTURE_2D, shadowMapDepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 4096, 4096, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMapDepthTexture, 0);
	glDrawBuffer(GL_NONE);

	ensuref(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "%s", glErr());

	shader.part = Shader("shader/part.vs", "shader/part.fs");
	shader.ghost = Shader("shader/ghost.vs", "shader/ghost.fs");
	shader.water = Shader("shader/water.vs", "shader/water.fs");
	shader.shadow = Shader("shader/shadow.vs", "shader/shadow.fs");
	shader.terrain = Shader("shader/terrain.vs", "shader/terrain.fs");
	shader.glow = Shader("shader/glow.vs", "shader/glow.fs");
	shader.flame = Shader("shader/flame.vs", "shader/flame.fs");
	shader.tree = Shader("shader/tree.vs", "shader/tree.fs");

	unit.mesh.cube = new MeshCube(1.0f);
	unit.mesh.sphere = new MeshSphere(1.0f);
	unit.mesh.line = new MeshLine(1.0f, 0.01f);
	unit.mesh.plane = new MeshPlane(1.0f);

	unit.mesh.sphere2 = new MeshSphere(1.0f,2);
	unit.mesh.sphere3 = new MeshSphere(1.0f,3);

	water = new MeshWater(Chunk::size);

	bits.wifi = new Mesh("models/wifi-hd.stl");
	bits.beaconFrame = new Mesh("models/status-beacon-frame-hd.stl");
	bits.beaconGlass = new Mesh("models/status-beacon-glass-hd.stl");
	bits.chevron = new Mesh("models/chevron-hd.stl");
	bits.flame = new Mesh("models/flame.stl");
	bits.droplet = new Mesh("models/droplet.stl");

	bits.pipeConnector = new Mesh("models/pipe-straight-hd.stl", glm::mat4(
		Point::West.rotation() * Mat4::scale(1.0, 1.0, 0.1)
	));

	icon.triangle = new Mesh("models/icon-triangle.stl");
	icon.tick = new Mesh("models/icon-tick.stl");
	icon.exclaim = new Mesh("models/icon-exclaim.stl");
	icon.electricity = new Mesh("models/icon-electricity.stl");

	for (auto& packet: packets) packet = Sim::random() > 0.5f;

	position = {100,100,100};
	direction = (position - Point::Zero).normalize();
	camera = {100,100,100};
	offset = {0,0,0};

	viewCamera = glm::lookAt(glm::vec3(camera), glm::vec3(glm::origin), glm::vec3(glm::up));
	viewWorld = glm::lookAt(glm::dvec3(position), glm::dvec3(0,0,0), glm::up);

	aspect = (float)Config::window.width / (float)Config::window.height;

	float fovx = glm::radians((float)Config::window.fov);
	fovy = glm::degrees(std::atan(std::tan(fovx/2.0)/aspect)*2.0);

	perspective = glm::perspective(glm::radians(fovy), aspect, near, far);
}

void Scene::prepare() {
	GuiEntity::prepareCaches();
}

void Scene::destroy() {
	delete unit.mesh.cube;
	delete unit.mesh.sphere;
	delete unit.mesh.line;
	delete unit.mesh.plane;

	delete unit.mesh.sphere2;
	delete unit.mesh.sphere3;
}

void Scene::view(Point pos) {
	pos.x = std::min(pos.x,  (real)world.scenario.size/2);
	pos.x = std::max(pos.x, -(real)world.scenario.size/2);
	pos.z = std::min(pos.z,  (real)world.scenario.size/2);
	pos.z = std::max(pos.z, -(real)world.scenario.size/2);

	jump.target = pos.floor(0.0);

	auto from = (Point::South + Point::Up).normalize() * 300.0f;
	jump.position = jump.target + from;

	selecting = false;
	selected.clear();
	selectingTypes = SelectAll;
}

float Scene::pen() {
	return std::max(2.0f, std::sqrt(range));
}

void Scene::cube(const Box& box, const Color& color) {
	auto scale = Mat4::scale(box.w,box.h,box.d);
	auto translate = (box.centroid() + offset).translation();
	unit.mesh.cube->instance(shader.ghost.id(), scale * translate, color);
}

void Scene::sphere(const Sphere& sphere, const Color& color) {
	auto scale = Mat4::scale(sphere.r);
	auto translate = (sphere.centroid() + offset).translation();
	auto mesh = unit.mesh.sphere;
	if (sphere.r > 1) mesh = unit.mesh.sphere2;
	if (sphere.r > 10) mesh = unit.mesh.sphere3;
	mesh->instance(shader.ghost.id(), scale * translate, color);
}

void Scene::line(const Point& a, const Point& b, const Color& color, float penW, float penH) {
	if (penW < 0.001) penW = pen();
	if (penH < 0.001) penH = pen();
	auto c = b-a;
	auto scale = Mat4::scale(penW,penH,c.length());
	auto rotate = c.rotation();
	auto translate = (a + (c * 0.5) + offset).translation();
	unit.mesh.line->instance(shader.ghost.id(), scale * rotate * translate, color);
}

void Scene::cuboid(const Cuboid& cuboid, const Color& color, float pen) {
	auto vertices = cuboid.vertices();
	line(vertices[0], vertices[1], color, pen);
	line(vertices[0], vertices[2], color, pen);
	line(vertices[0], vertices[4], color, pen);
	line(vertices[5], vertices[4], color, pen);
	line(vertices[5], vertices[7], color, pen);
	line(vertices[5], vertices[1], color, pen);
	line(vertices[7], vertices[6], color, pen);
	line(vertices[7], vertices[3], color, pen);
	line(vertices[3], vertices[1], color, pen);
	line(vertices[3], vertices[2], color, pen);
	line(vertices[6], vertices[2], color, pen);
	line(vertices[6], vertices[4], color, pen);
}

void Scene::circle(const Point& centroid, float radius, const Color& color, float pen, int step) {
	for (int i = 0; i < 360; i += step) {
		Mat4 rotA = Mat4::rotateY(glm::radians((float)i));
		Mat4 rotB = Mat4::rotateY(glm::radians((float)(i+step)));
		Point a = centroid + (Point::South * radius).transform(rotA);
		Point b = centroid + (Point::South * radius).transform(rotB);
		line(a, b, color, pen);
	}
}

void Scene::square(const Point& centroid, float half, const Color& color, float pen) {
	Point a = centroid + Point::South*half + Point::West*half;
	Point b = centroid + Point::West*half + Point::North*half;
	Point c = centroid + Point::North*half + Point::East*half;
	Point d = centroid + Point::East*half + Point::South*half;
	line(a, b, color, pen);
	line(b, c, color, pen);
	line(c, d, color, pen);
	line(d, a, color, pen);
}

void Scene::warning(Mesh* symbol, Point pos) {
	float spin = 360.0f * ((float)(Sim::tick%60)/60.0f);
	float bob = std::sin(glm::radians(spin));
	auto rot = Mat4::rotateY(glm::radians(spin));
	auto trx = (pos + Point::Up*(0.25f*bob) + scene.offset).translation();
	scene.icon.triangle->instance(scene.shader.ghost.id(), rot * trx, Color(0xffff00ff), 2.0);
	symbol->instance(scene.shader.glow.id(), rot * trx, Color(0x000000ff), 2.0);
}

void Scene::alert(Mesh* symbol, Point pos) {
	float spin = 360.0f * ((float)(Sim::tick%60)/60.0f);
	float bob = std::sin(glm::radians(spin));
	auto rot = Mat4::rotateY(glm::radians(spin));
	auto trx = (pos + Point::Up*(0.25f*bob) + scene.offset).translation();
	scene.icon.triangle->instance(scene.shader.ghost.id(), rot * trx, Color(0xff0000ff), 2.0);
	symbol->instance(scene.shader.glow.id(), rot * trx, Color(0x000000ff), 2.0);
}

void Scene::tick(Point pos) {
	risers.push_back({pos, icon.tick, 0x00ff00ff, Sim::tick});
}

void Scene::exclaim(Point pos) {
	risers.push_back({pos, icon.exclaim, 0xff0000ff, Sim::tick});
}

void Scene::updateMouse() {
	MouseState last = mouse;

	int mx, my;
	uint32_t buttons = SDL_GetMouseState(&mx, &my);

	mouse.x = mx;
	mouse.y = my;
	mouse.dx = mx - last.x;
	mouse.dy = my - last.y;

	if (mouse.dx || mouse.dy)
		mouse.moved = frame;

	mouse.wheel = wheel;
	wheel = 0;

	mouse.rH = last.rH;
	mouse.rV = last.rV;
	mouse.zW = last.zW;

	mouse.left = last.left;
	mouse.right = last.right;
	mouse.middle = last.middle;

	mouse.left.down = 0 != (buttons & SDL_BUTTON_LMASK);
	mouse.right.down = 0 != (buttons & SDL_BUTTON_RMASK);
	mouse.middle.down = 0 != (buttons & SDL_BUTTON_MMASK);

	mouse.left.changed = last.left.down != mouse.left.down;
	mouse.right.changed = last.right.down != mouse.right.down;
	mouse.middle.changed = last.middle.down != mouse.middle.down;

	for (MouseButton *button: {&mouse.left, &mouse.right, &mouse.middle}) {
		if (button->down && button->changed) {
			button->downAt = {mouse.x, mouse.y};
			button->drag = {0,0};
		}

		if (!button->down && button->changed) {
			button->upAt = {mouse.x, mouse.y};
			button->drag = {mouse.x-button->downAt.x, mouse.y-button->downAt.y};
		}

		if (button->down && !button->changed) {
			button->drag = {mouse.x-button->downAt.x, mouse.y-button->downAt.y};
		}

		if (!button->down && !button->changed) {
			button->drag = {0,0};
		}

		button->dragged = (std::abs(button->drag.x) > 5 || std::abs(button->drag.y) > 5);
		button->pressed = button->down && button->changed;
		button->released = !button->down && button->changed;
		button->clicked = button->released && !button->dragged;
	}

	if (mouse.right.down) {
		mouse.rH += Config::window.speedH * (float)mouse.dx;
		mouse.rV += Config::window.speedV * (float)mouse.dy;
	}

	if (mouse.wheel) {
		mouse.zW += Config::window.speedW * -(float)mouse.wheel;
	}
}

Ray Scene::mouseRay(int mx, int my) {
	// https://antongerdelan.net/opengl/raycasting.html
	float x = (2.0f * (float)mx) / (float)width - 1.0f;
	float y = 1.0f - (2.0f * (float)my) / (float)height;
	float z = 1.0f;
	glm::vec3 nds = glm::vec3(x, y, z);
	glm::vec4 clip = glm::vec4(nds.x, nds.y, -1.0, 1.0);
	glm::vec4 eye = glm::inverse(perspective) * clip;
	eye = glm::vec4(eye.x, eye.y, -1.0, 0.0);
	glm::vec4 wor = glm::inverse(viewCamera) * eye;
	glm::vec3 dir = glm::vec3(wor.x, wor.y, wor.z);
	return Ray(camera - offset, glm::normalize(dir));
}

std::pair<bool,Point> Scene::collisionRayGround(const Ray& ray, float level) {
	const double tiny = 0.00001;

	if (std::abs(ray.direction.y) > tiny) {
		double distance = (ray.position.y - level) / -ray.direction.y;

		if (distance > 0.0) {
			Point hit = ray.position + (ray.direction * distance);
			return std::pair<bool,Point>(true, {hit.x,level,hit.z});
		}
	}

	return std::pair<bool,Point>(false, Point::Zero);
}

Point Scene::groundTarget(float level) {
	auto [_,hit] = collisionRayGround(Ray(position, direction), level);
	return hit;
}

Point Scene::mouseGroundTarget(float level) {
	auto [_,hit] = collisionRayGround(mouse.ray, level);
	return hit;
}

Point Scene::mouseTerrainTarget() {
	Point pos = mouseGroundTarget();
	while (world.elevation(pos) > pos.y)
		pos += -mouse.ray.direction;
	return pos;
}

Point Scene::mouseWaterSurfaceTarget() {
	return mouseGroundTarget(-4.0f);
}

bool Scene::keyDown(int key) {
	auto it = keys.find(key);
	return it == keys.end() ? false: it->second;
}

bool Scene::keyReleased(int key) {
	auto it = keysLast.find(key);
	return (it == keysLast.end() ? false: it->second) && !keyDown(key);
}

bool Scene::buttonDown(int button) {
	switch (button) {
		case SDL_BUTTON_LEFT: return mouse.left.down;
		case SDL_BUTTON_RIGHT: return mouse.right.down;
		case SDL_BUTTON_MIDDLE: return mouse.middle.down;
	}
	return false;
}

bool Scene::buttonReleased(int button) {
	switch (button) {
		case SDL_BUTTON_LEFT: return mouse.left.released;
		case SDL_BUTTON_RIGHT: return mouse.right.released;
		case SDL_BUTTON_MIDDLE: return mouse.middle.released;
	}
	return false;
}

bool Scene::anyButtonReleased() {
	return mouse.left.released || mouse.right.released || mouse.middle.released;
}

bool Scene::buttonDragged(int button) {
	switch (button) {
		case SDL_BUTTON_LEFT: return mouse.left.dragged;
		case SDL_BUTTON_RIGHT: return mouse.right.dragged;
		case SDL_BUTTON_MIDDLE: return mouse.middle.dragged;
	}
	return false;
}

void Scene::updateCamera() {
	target = groundTarget();
	Point view = position - target;

	double pitchAngleMax = glm::radians(75.0);
	double zoomSpeedMul = std::max(0.1, view.length()/75.0);

	// compensate for variable frame rates
	zoomSpeedMul /= fps/60.0;

	if (keyDown(SDLK_w)) {
		Point ahead = {direction.x, 0, direction.z};
		position += ahead.normalize() * zoomSpeedMul;
	}

	if (keyDown(SDLK_s)) {
		Point ahead = {direction.x, 0, direction.z};
		position -= ahead.normalize() * zoomSpeedMul;
	}

	if (keyDown(SDLK_d)) {
		Point ahead = {direction.x, 0, direction.z};
		Point left = ahead.transform(Mat4::rotate(Point::Up, glm::radians(-90.0)));
		position += left.normalize() * zoomSpeedMul;
	}

	if (keyDown(SDLK_a)) {
		Point ahead = {direction.x, 0, direction.z};
		Point left = ahead.transform(Mat4::rotate(Point::Up, glm::radians(90.0)));
		position += left.normalize() * zoomSpeedMul;
	}

	if (mouse.rH > 0.0 || mouse.rH < 0.0 || mouse.rV > 0.0 || mouse.rV < 0.0) {
		double rDH = 0.0;
		double rDV = 0.0;

		if (std::abs(mouse.rH) < 0.0001) {
			rDH = mouse.rH;
			mouse.rH = 0.0;
		} else {
			double s = std::abs(mouse.rH)/4.0;
			rDH = mouse.rH > 0 ? s: -s;
			mouse.rH -= rDH;
		}

		if (std::abs(mouse.rV) < 0.0001) {
			rDV = mouse.rV;
			mouse.rV = 0.0;
		} else {
			double s = std::abs(mouse.rV)/4.0;
			rDV = mouse.rV > 0 ? s: -s;
			mouse.rV -= rDV;
		}

		target = groundTarget();
		view = position - target;
		Point right = view.cross(Point::Up).normalize();

		// as camera always orients to Up, prevent rotation past the zenith and causing a spin
		Point groundRadius = {view.x, 0.0f, view.z};
		double pitchAngle = std::acos(view.dot(groundRadius) / (view.length() * groundRadius.length()));
		rDV = (pitchAngle + rDV > pitchAngleMax) ? pitchAngleMax - pitchAngle: rDV;

		Mat4 rotateH = Mat4::rotate(Point::Up, -rDH);
		Mat4 rotateV = Mat4::rotate(right, rDV);
		Mat4 rotate = rotateH * rotateV;
		view = view.transform(rotate);

		position = target + view;
		position.y = std::max((double)Config::window.zoomLowerLimit, position.y);

		direction = (target - position).normalize();
	}

	target = groundTarget();

	if (mouse.zW > 0.0 || mouse.zW < 0.0) {
		double zDW = 0.0;

		double d = std::abs(mouse.zW);
		double s = d/4.0;

		if (std::abs(mouse.zW) < 0.01) {
			zDW = mouse.zW;
			mouse.zW = 0.0;
		} else {
			zDW = mouse.zW > 0 ? s: -s;
			mouse.zW -= zDW;
		}

		view = position - target;

		if (view.length() > (float)Config::window.zoomLowerLimit*2.0f || zDW > 0.0) {
			position += view * zDW;
			position.y = std::max((double)Config::window.zoomLowerLimit, position.y);
			direction = (target - position).normalize();
		}
	}

	if (position.distance(target) > (double)Config::window.zoomUpperLimit) {
		position = target - (direction * (double)Config::window.zoomUpperLimit);
	}

	view = position - target;
	auto vground = view.floor(0.0).normalize();
	auto snap = [&](const Point& cardinal, float left, float range) {
		auto deg = cardinal.degrees(vground);
		if (deg < range+0.01f) {
			auto rad = glm::radians(left < 0 ? deg: -deg);
			position = ((position-target).transform(Mat4::rotate(Point::Up, rad)) + target);
			direction = (target - position).normalize();
		}
	};

	if (Config::mode.cardinalSnap) {
		snap(Point::South, view.x, 3);
		snap(Point::North, -view.x, 3);
		snap(Point::East, -view.z, 3);
		snap(Point::West, view.z, 3);
	}

	if (snapNow) {
		snap(Point::South, view.x, 45);
		snap(Point::North, -view.x, 45);
		snap(Point::East, -view.z, 45);
		snap(Point::West, view.z, 45);
		snapNow = false;
	}

	camera = (position - target);
	offset = -target;
	viewCamera = glm::lookAt(glm::vec3(camera), glm::vec3(glm::origin), glm::vec3(glm::up));
	viewWorld = glm::lookAt(glm::dvec3(position), glm::dvec3(target), glm::up);
	mouse.ray = mouseRay(mouse.x, mouse.y);

	if (mouse.left.dragged) {
		if (!selecting) {
			Ray rayA = mouseRay(mouse.left.downAt.x, mouse.left.downAt.y);
			auto [_,hit] = collisionRayGround(rayA);
			selection.a = hit;
		}
		Ray rayB = mouse.ray;
		auto [_,hit] = collisionRayGround(rayB);
		selection.b = hit;
		selecting = !placing;
	}

	if (selecting && mouse.left.clicked) {
		selection = {Point::Zero, Point::Zero};
		selecting = false;
	}

	if (selecting) {
		selectionBox = Box(selection.a + (Point::Down*1000.0f), selection.b + (Point::Up*1000.0f));
	}

	if (!selecting) {
		selectingTypes = SelectAll;
	}
}

void Scene::updateVisibleCells() {
	visibleCells.clear();
	double horizonSquared = Config::window.horizon*Config::window.horizon;

	float step = Entity::RENDER;
	for (auto [cx,cy]: gridwalk(step, region)) {

		Box box = Box(
			Point(cx*step, altitudeMin, cy*step),
			Point((cx+1)*step, altitudeMax, (cy+1)*step)
		);

		// Eliminate grid cells beyond the horizon
		if (box.centroid().distanceSquared(target) > horizonSquared) continue;

		// Eliminate grid cells outside the frustum
		if (!frustum.intersects(box)) continue;

		visibleCells.push_back(box);
	}
}

void Scene::updateEntities() {
	entityPools[future].resize(Config::engine.sceneLoadingThreads);
	for (auto& pool: entityPools[future]) pool.clear();
	for (auto& [_,spec]: Spec::all) spec->count.render = 0;

	if (!visibleCells.size()) updateVisibleCells();

	selectedFuture.clear();
	hoveringFuture = 0;

	trigger doneLoading;
	trigger doneHovering;
	trigger doneInstancing;
	trigger doneInstancingItems;
	trigger doneInstancingCables;
	trigger doneGhosting;

	typedef minivec<Entity*> EBatch;
	typedef minivec<GuiEntity*> GBatch;

	channel<EBatch*,-1> forLoading;
	channel<GBatch*,-1> forHovering;
	channel<GBatch*,-1> forInstancing;
	channel<GBatch*,-1> forInstancingItems;
	channel<GBatch*,-1> forInstancingCables;
	channel<GBatch*,-1> forGhosting;

	channel<EBatch*,-1> oldEBatch;
	channel<GBatch*,-1> oldGBatch;

	std::vector<StopWatch> loadTimers(entityPools[future].size());
	std::vector<StopWatch> instancingTimers(Config::engine.sceneInstancingThreads);
	std::vector<StopWatch> instancingItemsTimers(Config::engine.sceneInstancingItemsThreads);

	// GuiEntity loaders, Sim::locked via main thread, fed by main thread
	for (int i = 0, l = entityPools[future].size(); i < l; i++) {
		crew.job([&,i]() {
			loadTimers[i].start();
			double horizonSquared = Config::window.horizon*Config::window.horizon;

			GBatch* out = new GBatch;
			out->reserve(1000);

			auto flush = [&](uint size) {
				if (out->size() > size) {
					forHovering.send(out);
					forInstancing.send(out);
					forInstancingItems.send(out);
					forInstancingCables.send(out);
					oldGBatch.send(out);
					out = new GBatch;
					out->reserve(1000);
				}
			};

			for (auto in: forLoading) {
				for (auto en: *in) {
					if (en->pos().distanceSquared(target) > horizonSquared) continue;
					if (!frustum.intersects(en->sphere())) continue;
					out->push_back(&entityPools[future][i].emplace(en));
					flush(999);
				}
			}
			flush(0);
			oldGBatch.send(out);

			loadTimers[i].stop();
			doneLoading.now();
		});
	}

	// mouse ray intersections, not Sim::locked, fed by loaders
	crew.job([&]() {
		stats.updateEntitiesHover.track(frame, [&]() {
			GBatch hovered;
			float hoverDistanceSquared = Config::window.zoomUpperLimit * Config::window.zoomUpperLimit;

			for (auto in: forHovering) {
				for (auto ge: *in) {
					if (ge->spec->select
						&& ge->pos().distanceSquared(target) < hoverDistanceSquared
						&& ge->sphere().intersects(mouse.ray)
						&& ge->selectionCuboid().intersects(mouse.ray)
					){
						hovered.push_back(ge);
					}
					ge->spec->count.render++;
				}
			}

			Point pos = position;
			auto end = mouseGroundTarget();
			Point step = (end-position).normalize()*0.25;

			for (int i = 0, l = position.distance(end)*10; !hoveringFuture && i < l; i++) {
				for (auto ge: hovered) {
					bool allow = true;
					if (ge->spec->slab) {
						allow = false;
						for (auto& se: selectedFuture) {
							if (se->id == ge->id) {
								allow = true;
								break;
							}
						}
					}
					if (allow && ge->sphere().contains(pos) && ge->cuboid().intersects(pos.box().grow(0.5))) {
						hoveringFuture = ge->id;
						break;
					}
				}
				pos += step;
			}

			doneHovering.now();
		});
	});

	// GuiEntity instanced rendering, not Sim::locked, fed by loaders
	for (int i = 0, l = Config::engine.sceneInstancingThreads; i < l; i++) {
		crew.job([&,i]() {
			instancingTimers[i].start();
			GBatch* ghost = new GBatch;

			Mesh::batchInstances();

			for (auto in: forInstancing) {
				for (auto ge: *in) {
					if (ge->ghost) {
						ghost->push_back(ge);
						continue;
					}
					ge->instance();
					ge->icon();
				}
			}

			if (ghost->size()) forGhosting.send(ghost);
			oldGBatch.send(ghost);

			Mesh::flushInstances();

			instancingTimers[i].stop();
			doneInstancing.now();
		});
	}

	// Item instanced rendering, not Sim::locked, fed by loaders
	for (int i = 0, l = Config::engine.sceneInstancingItemsThreads; i < l; i++) {
		crew.job([&,i]() {
			instancingItemsTimers[i].start();

			Mesh::batchInstances();

			for (auto in: forInstancingItems) {
				for (auto ge: *in) {
					if (ge->ghost) continue;
					ge->instanceItems();
				}
			}

			Mesh::flushInstances();

			instancingItemsTimers[i].stop();
			doneInstancingItems.now();
		});
	}

	// GuiEntity cable rendering, not Sim::locked, fed by instances
	crew.job([&]() {
		Mesh::batchInstances();

		for (auto in: forInstancingCables) {
			for (auto ge: *in) ge->instanceCables();
		}

		Mesh::flushInstances();
		doneInstancingCables.now();
	});

	// GuiEntity ghost rendering, not Sim::locked, fed by instancers
	// Runs last so that translucent ghosts can be sorted and rendered after extant entities
	crew.job([&]() {
		GBatch ghost;
		Mesh::batchInstances();

		for (auto in: forGhosting) ghost.append(*in);

		std::sort(ghost.begin(), ghost.end(), [&](GuiEntity* a, GuiEntity* b) {
			return a->pos().distanceSquared(position) < b->pos().distanceSquared(position);
		});

		for (auto ge: ghost) ge->instance();

		Mesh::flushInstances();
		doneGhosting.now();
	});

	// main thread, Sim::locked, fed by Entity::grid indexes
	Sim::locked([&]() {
		stats.updateEntitiesFind.track(frame, [&]() {
			EBatch marked;
			GBatch* geBatch = new GBatch;

			// when selection box goes off screen, ensure the out of sight entities are included
			if (selecting) {
				for (auto en: Entity::intersecting(selectionBoxFuture)) {
					if (en->isMarked1()) continue;
					en->setMarked1(true);
					marked.push_back(en);
					geBatch->push_back(&entityPools[future][0].emplace(en));
					if (en->spec->select && en->spec->plan) {
						if (selectingTypes == SelectAll)
							selectedFuture.push_back(geBatch->back());
						if (selectingTypes == SelectJunk && en->spec->junk)
							selectedFuture.push_back(geBatch->back());
						if (selectingTypes == SelectUnder && (en->spec->pile || en->spec->slab))
							selectedFuture.push_back(geBatch->back());
					}
				}
				if (selectingTypes == SelectAll) {
					uint secondaries = 0;
					for (auto ge: selectedFuture) {
						secondaries += (ge->spec->pile || ge->spec->slab) ? 1:0;
					}
					if (secondaries < selectedFuture.size()) {
						discard_if(selectedFuture, [](auto a) { return (a->spec->pile || a->spec->slab); });
					}
				}
			}

			// the directed vehicle may need to show its path from off-screen
			if (directing) {
				auto ed = Entity::find(directing->id);
				if (ed && !ed->isMarked1()) {
					ed->setMarked1(true);
					marked.push_back(ed);
					geBatch->push_back(&entityPools[future][0].emplace(ed));
				}
			}

			// tube paths may be in view when their towers are out of sight
			// this should check the longest range from specs
			// this should be a bit more selective based on the frustum
			for (auto et: Entity::gridRenderTubes.dump(region)) {
				if (et->isMarked1()) continue;
				et->setMarked1(true);
				marked.push_back(et);
				if (frustum.intersects(et->sphere().grow(100))) {
					geBatch->push_back(&entityPools[future][0].emplace(et));
				}
			}

			// monorail paths may be in view when their towers are out of sight
			// this should check the longest range from specs
			// this should be a bit more selective based on the frustum
			for (auto et: Entity::gridRenderMonorails.dump(region)) {
				if (et->isMarked1()) continue;
				et->setMarked1(true);
				marked.push_back(et);
				if (frustum.intersects(et->sphere().grow(100))) {
					geBatch->push_back(&entityPools[future][0].emplace(et));
				}
			}

			// power cables may be in view when their poles are out of sight
			// this should check the longest range from specs
			// this should be a bit more selective based on the frustum
			for (auto pole: PowerPole::gridCoverage.dump(region)) {
				auto et = pole->en;
				if (et->isMarked1()) continue;
				et->setMarked1(true);
				marked.push_back(et);
				if (frustum.intersects(et->sphere().grow(100))) {
					geBatch->push_back(&entityPools[future][0].emplace(et));
				}
			}

			// waypoint lines may be in view when their waypoints are out of sight
			for (auto et: Entity::intersecting(region, Entity::gridCartWaypoints)) {
				if (et->isMarked1()) continue;
				et->setMarked1(true);
				marked.push_back(et);
				geBatch->push_back(&entityPools[future][0].emplace(et));
			}

			forHovering.send(geBatch);
			forInstancing.send(geBatch);
			forInstancingItems.send(geBatch);
			forInstancingCables.send(geBatch);

			EBatch* enBatch = new EBatch;

			// This loop is the main rendering bottleneck. It needs to stay fast and tight to
			// minimize blocking the Sim update _and_ prevent the GuiEntity loaders stalling
			for (auto& box: visibleCells) {

				// Entity::intersecting() wraps grid.search and does extra work to reduce the coarse grid
				// results to an accurate subset that definitely intersect the box, but we have to do a
				// frustum intersection anyway so just extract the coarse hits and rely on marking to
				// avoid feeding duplicates to the GuiEntity loaders

				auto end = Entity::gridRender.cells.end();
				for (auto cell: gridwalk(Entity::RENDER, box)) {
					auto it = Entity::gridRender.cells.find(cell);
					if (it == end) continue;
					for (auto en: it->second) {
						if (en->isMarked1()) continue;
						en->setMarked1(true);
						enBatch->push_back(en);
					}
				}

				if (enBatch->size() >= 1000) {
					marked.append(*enBatch);
					forLoading.send(enBatch);
					oldEBatch.send(enBatch);
					enBatch = new EBatch;
				}
			}

			marked.append(*enBatch);
			forLoading.send(enBatch);
			forLoading.close();

			oldEBatch.send(enBatch);

			for (auto en: marked) en->clearMarks();
		});

		doneLoading.wait(entityPools[future].size());
	});

	updateVisibleCells();

	forHovering.close();
	forInstancing.close();
	forInstancingItems.close();
	forInstancingCables.close();

	doneHovering.wait();
	doneInstancing.wait(Config::engine.sceneInstancingThreads);
	doneInstancingItems.wait(Config::engine.sceneInstancingItemsThreads);
	doneInstancingCables.wait();

	forGhosting.close();
	doneGhosting.wait();

	stats.updateEntitiesLoad.track(frame, loadTimers);
	stats.updateEntitiesInstances.track(frame, instancingTimers);
	stats.updateEntitiesItems.track(frame, instancingItemsTimers);

	oldEBatch.close();
	oldGBatch.close();

	for (auto eb: oldEBatch.recv_all()) delete eb;
	for (auto gb: oldGBatch.recv_all()) delete gb;
}

void Scene::updateCurrent() {
	selected = selectedFuture;
	chunks = chunksFuture;

	Sim::locked([&]() {
		if (hovering) {
			delete hovering;
			hovering = nullptr;
		}

		if (hoveringFuture && Entity::exists(hoveringFuture)) {
			hovering = new GuiEntity(hoveringFuture);
		}

		if (directing && !Entity::exists(directing->id)) {
			delete directing;
			directing = nullptr;
		}

		if (directing) {
			uint id = directing->id;
			delete directing;
			directing = new GuiEntity(id);
		}

		if (!directing && directRevert && Entity::exists(directRevert)) {
			directing = new GuiEntity(directRevert);
		}

		if (connecting && !Entity::exists(connecting->id)) {
			delete connecting;
			connecting = nullptr;
		}

		if (connecting) {
			uint id = connecting->id;
			delete connecting;
			connecting = new GuiEntity(id);
		}

		if (routing && !Entity::exists(routing->id)) {
			delete routing;
			routing = nullptr;
		}

		placingFits = !placing || placing->fits();
	});

	for (auto ge: selected) {
		if (ge->spec->select && ge->cuboid().intersects(selectionBox)) {
			cuboid(ge->selectionCuboid(), 0x0000ffff, pen());
		}
	}
}

void Scene::updatePlacing() {
	if (!placing) return;

	if (focused) {
		bool placingMoved = false;

		if (placing->entities.size() == 1 && placing->entities[0]->spec->placeOnHill) {
			auto pos = mouseTerrainTarget();
			auto pe = placing->entities[0];
			Box box = pe->spec->placeOnHillBox(pos);
			placing->move(Point(pos.x, world.hillPlatform(box), pos.z));
			placingMoved = true;
		}
		else
		if (placing->entities.size() == 1 && placing->entities[0]->spec->placeOnWaterSurface) {
			placing->move(mouseWaterSurfaceTarget());
			placingMoved = true;
		}
		else
		if (placing->entities.size() == 1 && placing->entities[0]->spec->monocar && placing->entities[0]->spec->place == Spec::Monorail) {
			auto pos = mouseGroundTarget();
			Sim::locked([&]() {
				float dist = -1;
				Entity* near = nullptr;
				for (auto em: Entity::intersecting(pos.box().grow(100), Entity::gridRenderMonorails)) {
					if (em->spec->monorail && (dist < 0 || em->pos().distance(pos) < dist)) {
						near = em;
						dist = em->pos().distance(pos);
					}
				}
				if (near) {
					placing->move(near->monorail());
					placingMoved = true;
				}
			});
		}
		else
		if (placing->entities.size() == 1 && placing->entities[0]->spec->flightLogistic) {
			auto pos = mouseGroundTarget();
			Sim::locked([&]() {
				float dist = -1;
				Entity* near = nullptr;
				for (auto em: Entity::intersecting(pos.box().grow(10))) {
					if (em->spec->flightPad && (dist < 0 || em->pos().distance(pos) < dist)) {
						near = em;
						dist = em->pos().distance(pos);
					}
				}
				if (near) {
					placing->move(near->pos() + (Point::Up * (near->spec->collision.h/2.0)));
					placingMoved = true;
				}
			});
		}

		if (!placingMoved) {
			auto pos = mouseGroundTarget();
			placing->move(pos);
			placing->floor(0.0f);
		}
	}

	for (auto te: placing->entities) {
		te->instance();
		te->instanceCables();

		cuboid(
			te->selectionCuboid(),
			placingFits ? 0x00ff00ff: 0xff0000ff,
			pen()
		);

		te->overlayHovering(placing->entities.size() == 1);
	}

	if (placing->entities.size()) {
		placing->central()->overlayAlignment();
	}
}

void Scene::updateTerrain() {
	Mesh::batchInstances();
	chunksFuture.clear();

	double horizonSquared = Config::window.horizon*Config::window.horizon;

	for (auto [cx,cy]: gridwalk(Chunk::size, region).spiral()) {
		Chunk* chunk = Chunk::request(cx, cy);
		if (!chunk) continue;

		if (chunk->centroid().distanceSquared(target) > horizonSquared) continue;

		bool hd = chunk->centroid().distance(position) < Chunk::size*5 || chunk->centroid().distance(target) < Chunk::size*3;
		bool ld = !hd && chunk->centroid().distance(position) < Chunk::size*8;

		// mark chunk as viewed even if pruned by frustum below.
		// avoids unloading nearby chunks that the camera could pan to quickly
		if (hd) {
			chunk->tickLastViewed = Sim::tick;
		}
		else
		if (ld) {
			chunk->tickLastViewedLD = Sim::tick;
		}
		else {
			chunk->tickLastViewedVLD = Sim::tick;
		}

		if (!frustum.intersects(chunk->box())) continue;
		chunksFuture.push_back(chunk);

		if (hd) {
			chunk->heightmap->instance(shader.terrain.id(), chunk->transformation(offset), Color(0xffffffff));
		}
		else
		if (ld) {
			chunk->heightmapLD->instance(shader.terrain.id(), chunk->transformationLD(offset), Color(0xffffffff));
		}
		else {
			chunk->heightmapVLD->instance(shader.terrain.id(), chunk->transformationVLD(offset), Color(0xffffffff));
		}

		if (chunk->hasWater) {
			auto trx = chunk->transformationWater(offset);
			water->instance(shader.water.id(), trx, Color(0x010160FF), 512.0f, Part::NOSHADOW);
		}

		int ctx = chunk->x*Chunk::size;
		int ctz = chunk->y*Chunk::size;

		for (auto [tx,ty]: chunk->drawOnHill) {
			World::XY at = {ctx+tx, ctz+ty};
			float h = chunk->region.elevation(at)-0.25f;
			if (target.distanceSquared(Point(ctx+tx, h, ctz+ty)) < 100.0*100.0*3) {
				Point p = {(float)(ctx+tx)+0.5f, h, (float)(ctz+ty)+0.5f};
				Mat4 r = Mat4::rotate(p, glm::radians((double)tx));
				Mat4 m = Mat4::translate(p + offset);
				auto item = Item::get(chunk->region.resource(at));
				for (auto part: item->parts) {
					part->instance(shader.part.id(), 0.0f, r * m, part->color, Part::NOSHADOW);
				}
			}
		}
	}

	if (Config::mode.skyBlocks) {
		for (auto& block: sky.blocks) {
			if (block.clear && frustum.intersects(block.box())) {
				scene.cube(block.box().shrink(8), 0xffffffff);
			}
		}
	}

	Mesh::flushInstances();
}

void Scene::tipBegin(float ww) {
	using namespace ImGui;

	auto& style = GetStyle();
	ImVec2 tooltip_pos = GetMousePos();
	tooltip_pos.x += (Config::window.hdpi/96.0) * 16;
	tooltip_pos.y += (Config::window.vdpi/96.0) * 16;
	SetNextWindowPos(tooltip_pos);

	// popup "big"
	float h = (float)Config::height(0.75f);
	float w = (h*1.45)*0.16*ww;

	SetNextWindowSize(ImVec2(w,-1));
	SetNextWindowBgAlpha(style.Colors[ImGuiCol_PopupBg].w * 0.75f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
	Begin("##space-tip", nullptr, flags);
	PushFont(Config::sidebar.font.imgui);
}

void Scene::tipEnd() {
	using namespace ImGui;
	PopFont();
	End();
}

void Scene::tipResources(Box mbox, Box dbox) {
	std::map<uint,uint> minables;
	std::map<uint,uint> drillables;

	if (mbox.w > 0.1) for (Stack stack: world.minables(mbox)) {
		minables[stack.iid] = std::max(minables[stack.iid], stack.size);
	}

	if (dbox.w > 0.1) for (Amount amount: world.drillables(dbox)) {
		drillables[amount.fid] = std::max(drillables[amount.fid], amount.size);
	}

	auto colorStore = ImGui::GetColorU32(ImVec4(0.6f, 0.6f, 0.6f, 1.0f));

	if (minables.size() || drillables.size()) {
		ImGui::Separator();
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorStore);

		uint maxCount = 0;
		for (auto [_,count]: minables) {
			maxCount = std::max(maxCount, count);
		}

		for (auto [iid,count]: minables) {
			ImGui::Print(Item::get(iid)->ore.c_str());
			ImGui::SameLine(); ImGui::PrintRight(fmtc("%d", count));
			ImGui::SmallBar((float)count/(float)maxCount);
		}

		maxCount = 0;
		for (auto [_,count]: drillables) {
			maxCount = std::max(maxCount, count);
		}

		for (auto [fid,count]: drillables) {
			ImGui::Print(Fluid::get(fid)->title.c_str());
			ImGui::SameLine(); ImGui::PrintRight(fmtc("%d", count));
			ImGui::SmallBar((float)count/(float)maxCount);
		}

		ImGui::PopStyleColor(1);
	}
	else {
		ImGui::Print("(no resources)");
	}
}

void Scene::tipStorage(Store& store) {
	auto colorStore = ImGui::GetColorU32(ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorStore);

	for (auto level: store.levels) {
		auto count = store.count(level.iid);
		ImGui::Print(Item::get(level.iid)->title.c_str());
		ImGui::SameLine(); ImGui::PrintRight(fmtc("%u/%u", count, level.upper));
		ImGui::SmallBar((float)count/(float)level.upper);
	}

	for (auto stack: store.stacks) {
		if (store.level(stack.iid)) continue;
		auto count = store.count(stack.iid);
		uint limit = store.limit().items(stack.iid);
		ImGui::Print(Item::get(stack.iid)->title.c_str());
		ImGui::SameLine(); ImGui::PrintRight(fmtc("%u/%u", count, limit));
		ImGui::SmallBar((float)count/(float)limit);
	}

	ImGui::PopStyleColor(1);
}

void Scene::tipDebug(GuiEntity* ge) {
	using namespace ImGui;

	Print(fmtc("id %u", ge->id));
	Print(fmtc("pos %s", std::string(ge->pos())));
	Print(fmtc("dir %s", std::string(ge->dir())));
}

void Scene::update(uint w, uint h, float f) {
	width = w;
	height = h;
	fps = f;

	aspect = (float)width / (float)height;

	float fovx = glm::radians((float)Config::window.fov);
	fovy = glm::degrees(std::atan(std::tan(fovx/2.0)/aspect)*2.0);

	perspective = glm::perspective(glm::radians(fovy), aspect, near, far);

	if (jump.position != Point::Zero) {
		position = jump.position;
		target = jump.target;
		direction = (target-position).normalize();
		camera = position-target;
		jump.position = Point::Zero;
		jump.target = Point::Zero;
		updateCamera();
	}

	if (focused) {
		updateMouse();
		updateCamera();
	}

	frustum = Frustum(glm::dmat4(perspective) * viewWorld);
	region = target.box().grow(Config::window.horizon);

	Mesh::resetAll();

	current = Mesh::current;
	future = Mesh::future;
	ensure(current != future);
	selectionBoxFuture = selectionBox;

	stats.updateCurrent.track(frame, [&]() { updateCurrent(); });
	stats.updatePlacing.track(frame, [&]() { updatePlacing(); });

	range = camera.length();

	if (hovering) {
		cuboid(hovering->selectionCuboid(), 0x0000ffff, pen());
		hovering->overlayHovering();
	}

	if (directing) {
		directing->overlayDirecting();
	}

	if (routing) {
		routing->overlayRouting();
		routing->overlayAlignment();
	}

	for (auto it = risers.begin(); it != risers.end(); ) {
		auto& riser = *it;

		if (Sim::tick > riser.tick+60) {
			it = risers.erase(it);
			continue;
		}

		float spin = 360.0f * ((float)(Sim::tick%60)/60.0f);
		auto rot = Mat4::rotateY(glm::radians(spin));
		auto rise = Point::Up * ((float)(Sim::tick-riser.tick) * 0.02);
		auto trx = (riser.start + rise + scene.offset).translation();
		scene.icon.triangle->instance(scene.shader.ghost.id(), rot * trx, riser.color, 2.0);
		riser.icon->instance(scene.shader.glow.id(), rot * trx, Color(0x000000ff), 2.0);
		++it;
	}

	bool showTip = keyDown(SDLK_SPACE) || (Config::mode.autotip && (frame-mouse.moved) > fps);

	if (focused && showTip && !(selecting || placing || hovering)) {
		auto box = mouseGroundTarget().box().grow(0.5);
		tipBegin();
		ImGui::Header("Resources");
		tipResources(box, box);
		tipEnd();
	}

	if (focused && showTip && (selecting || placing || hovering)) {
		tipBegin();

		auto colorStore = ImGui::ImColorSRGB(0x999999ff);
		auto colorEnergy = ImGui::ImColorSRGB(0x9999ddff);

		if (selecting && selected.size()) {
			ImGui::Header("Selection");
			std::map<std::string,int> counts;
			for (auto ge: selected) {
				counts[ge->spec->title]++;
			}
			for (auto [title,count]: counts) {
				ImGui::Print(title.c_str());
				ImGui::SameLine();
				ImGui::PrintRight(fmtc("%d", count));
			}
		}

		if (placing && placing->entities.size() == 1) {
			ImGui::Header(placing->entities.front()->spec->title.c_str());
		}

		if (placing && placing->entities.size() > 1) {
			ImGui::Header("Construction Plan");
			std::map<std::string,int> counts;
			for (auto ge: placing->entities) {
				counts[ge->spec->title]++;
			}
			for (auto [title,count]: counts) {
				ImGui::Print(title.c_str());
				ImGui::SameLine();
				ImGui::PrintRight(fmtc("%d", count));
			}
		}

		if (hovering) {
			ImGui::Header(hovering->spec->title.c_str());
		}

		if (hovering && hovering->ghost) {
			Sim::locked([&]() {
				if (!Entity::exists(hovering->id)) return;
				auto& en = Entity::get(hovering->id);
				if (!en.isGhost()) return;
				auto& gstore = en.ghost().store;
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				for (auto& level: gstore.levels) {
					ImGui::Print(Item::get(level.iid)->title.c_str());
					ImGui::SameLine();
					ImGui::PrintRight(fmtc("%u/%u", gstore.count(level.iid), level.lower));
					if (en.isConstruction()) {
						ImGui::SmallBar((float)gstore.count(level.iid)/(float)level.lower);
					}
					if (en.isDeconstruction()) {
						bool done = false;
						for (auto [iid,count]: en.spec->materials) {
							if (iid == level.iid) {
								ImGui::SmallBar((float)gstore.count(level.iid)/(float)count);
								done = true;
								break;
							}
						}
						if (!done) {
							// surplus materials
							ImGui::SmallBar((float)gstore.count(level.iid)/(float)gstore.limit().items(level.iid));
						}
					}
				}
				ImGui::PopStyleColor(1);
			});
		}

		if (hovering && !hovering->ghost) {
			if (hovering->spec->health) {
				Color color = hovering->health < hovering->spec->health ? 0xff0000ff: 0x00cc00ff;
				ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color);
				ImGui::SmallBar((float)hovering->health/(float)hovering->spec->health);
				ImGui::PopStyleColor(1);
			}

			if (hovering->spec->consumeFuel && !hovering->spec->crafter) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& burner = en.burner();
					if (!burner.fueled()) {
						ImGui::Spacing();
						ImGui::Alert("Fuel");
					}
				});
			}

			if (hovering->spec->powerpole) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& pole = en.powerpole();
					ImGui::Spacing();
					Popup::powerpoleNotice(pole);
				});
			}

			if (hovering->spec->crafter) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& crafter = en.crafter();
					ImGui::Spacing();
					Popup::crafterNotice(crafter);
					ImGui::Print(crafter.recipe ? crafter.recipe->title.c_str(): "(no recipe)");
					auto energy = crafter.working ? crafter.consumption() * crafter.speed(): Energy(0);
					ImGui::SameLine(); ImGui::PrintRight(fmtc("%s", energy.formatRate()));
					ImGui::SmallBar(crafter.progress);
				});
			}

			if (hovering->spec->pipe && !hovering->spec->launcher) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& pipe = en.pipe();
					if (!pipe.network) return;
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
					ImGui::Print(pipe.network->fid ? Fluid::get(pipe.network->fid)->title: "(empty)");
					ImGui::SameLine();
					ImGui::PrintRight(fmtc("%s / %s",
						Liquid(pipe.network->fid ? ((float)en.spec->pipeCapacity.value * pipe.network->level()): 0).format(),
						Liquid(pipe.network->fid ? pipe.network->count(pipe.network->fid): 0).format()
					));
					ImGui::SmallBar(pipe.network->level());
					ImGui::PopStyleColor(1);
				});
			}

			if (hovering->spec->launcher) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& launcher = en.launcher();
					ImGui::Spacing();
					Popup::launcherNotice(launcher);
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
					auto required = launcher.fuelRequired();
					for (auto amount: launcher.fuelAccessable()) {
						ImGui::Print(Fluid::get(amount.fid)->title.c_str());
						ImGui::SameLine(); ImGui::PrintRight(Liquid(amount.size).format());
						ImGui::SmallBar(Liquid(amount.size).portion(Liquid(required[amount.fid].size)));
					}
					ImGui::PopStyleColor(1);
				});
			}

			if (hovering->spec->store) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& store = en.store();
					if (en.spec->tipStorage)
						tipStorage(store);
					else {
						ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
						ImGui::SmallBar(store.usage().portion(store.limit()));
						ImGui::PopStyleColor(1);
					}
				});
			}

			if (hovering->spec->arm) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& arm = en.arm();
					if (arm.filter.size()) {
						int i = 0;
						std::string csv;
						for (auto iid: arm.filter) {
							if (i++) csv += ", ";
							csv += Item::get(iid)->title;
						}
						ImGui::Print(fmtc("Filter: %s", csv));
					}
					else {
						ImGui::Print(arm.iid ? Item::get(arm.iid)->title.c_str(): "(arm empty)");
					}
				});
			}

			if (hovering->spec->loader && hovering->spec->conveyor) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& loader = en.loader();
					auto& conveyor = en.conveyor();
					if (loader.filter.size()) {
						int i = 0;
						std::string csv;
						for (auto iid: loader.filter) {
							if (i++) csv += ", ";
							csv += Item::get(iid)->title;
						}
						ImGui::Print(fmtc("Filter: %s", csv));
					}
					else {
						miniset<uint> items;
						for (auto iid: conveyor.items())
							items.insert(iid);
						int i = 0;
						std::string csv;
						for (auto iid: items) {
							if (i++) csv += ", ";
							csv += Item::get(iid)->title;
						}
						ImGui::Print(i ? csv.c_str(): "(belt empty)");
					}
				});
			}

			if (hovering->spec->balancer && hovering->spec->conveyor) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& balancer = en.balancer();
					if (balancer.filter.size()) {
						int i = 0;
						std::string csv;
						for (auto iid: balancer.filter) {
							if (i++) csv += ", ";
							csv += Item::get(iid)->title;
						}
						ImGui::Print(fmtc("Filter: %s", csv));
					}
					if (balancer.priority.input && balancer.priority.output) {
						ImGui::Print("Priority: Input, Output");
					}
					if (balancer.priority.input && !balancer.priority.output) {
						ImGui::Print("Priority: Input");
					}
					if (!balancer.priority.input && balancer.priority.output) {
						ImGui::Print("Priority: Output");
					}
				});
			}

			if (hovering->spec->tube && hovering->spec->conveyor) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& tube = en.tube();
					if (tube.input == Tube::Mode::BeltOrTube) { ImGui::Print("Input: Belt or Tube"); }
					if (tube.input == Tube::Mode::BeltOnly) { ImGui::Print("Input: Belt only"); }
					if (tube.input == Tube::Mode::TubeOnly) { ImGui::Print("Input: Tube only"); }
					if (tube.input == Tube::Mode::BeltPriority) { ImGui::Print("Input: Belt priority"); }
					if (tube.input == Tube::Mode::TubePriority) { ImGui::Print("Input: Tube priority"); }
					if (tube.output == Tube::Mode::BeltOrTube) { ImGui::Print("Output: Belt or Tube"); }
					if (tube.output == Tube::Mode::BeltOnly) { ImGui::Print("Output: Belt only"); }
					if (tube.output == Tube::Mode::TubeOnly) { ImGui::Print("Output: Tube only"); }
					if (tube.output == Tube::Mode::BeltPriority) { ImGui::Print("Output: Belt priority"); }
					if (tube.output == Tube::Mode::TubePriority) { ImGui::Print("Output: Tube priority"); }
				});
			}

			if (!hovering->spec->loader && !hovering->spec->tube && !hovering->spec->balancer && hovering->spec->conveyor) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& conveyor = en.conveyor();
					miniset<uint> items;
					for (auto iid: conveyor.items())
						items.insert(iid);
					int i = 0;
					std::string csv;
					for (auto iid: items) {
						if (i++) csv += ", ";
						csv += Item::get(iid)->title;
					}
					ImGui::Print(i ? csv.c_str(): "(belt empty)");
				});
			}

			if (hovering->spec->consumeCharge) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& charger = en.charger();
					ImGui::Print(charger.energy.format().c_str());
					ImGui::SameLine();
					ImGui::PrintRight(charger.chargeRate().formatRate().c_str());
					ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colorEnergy);
					ImGui::SmallBar(charger.level());
					ImGui::PopStyleColor(1);
				});
			}

			if (hovering->spec->windTurbine) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto pos = en.pos();
					float wind = Sim::windSpeed({pos.x, pos.y-(en.spec->collision.h/2.0f), pos.z});
					Energy supplied = en.spec->energyGenerate * wind;
					ImGui::Print(supplied.formatRate().c_str());
				});
			}

			if (hovering->spec->drone) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& drone = en.drone();
					if (!drone.iid) return;
					ImGui::Print(fmtc("%s (%d)", Item::get(drone.stack.iid)->title, drone.stack.size));
				});
			}

			if (hovering->spec->flightLogistic) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& flight = en.flightLogistic();
					switch (flight.stage) {
						case FlightLogistic::Home: {
							ImGui::Print("Home");
							break;
						}
						case FlightLogistic::ToSrc: {
							ImGui::Print("ToSrc");
							break;
						}
						case FlightLogistic::ToSrcDeparting: {
							ImGui::Print("ToSrcDeparting");
							break;
						}
						case FlightLogistic::ToSrcFlight: {
							ImGui::Print("ToSrcFlight");
							break;
						}
						case FlightLogistic::Loading: {
							ImGui::Print("Loading");
							break;
						}
						case FlightLogistic::ToDst: {
							ImGui::Print("ToDst");
							break;
						}
						case FlightLogistic::ToDstDeparting: {
							ImGui::Print("ToDstDeparting");
							break;
						}
						case FlightLogistic::ToDstFlight: {
							ImGui::Print("ToDstFlight");
							break;
						}
						case FlightLogistic::Unloading: {
							ImGui::Print("Unloading");
							break;
						}
						case FlightLogistic::ToDep: {
							ImGui::Print("ToDep");
							break;
						}
						case FlightLogistic::ToDepDeparting: {
							ImGui::Print("ToDepDeparting");
							break;
						}
						case FlightLogistic::ToDepFlight: {
							ImGui::Print("ToDepFlight");
							break;
						}
					}
					if (flight.dep && Entity::exists(flight.dep)) {
						scene.line(en.pos(), Entity::get(flight.dep).pos(), 0xffa500ff);
					}
					if (flight.src && Entity::exists(flight.src)) {
						scene.line(en.pos(), Entity::get(flight.src).pos(), 0xff0000ff);
					}
					if (flight.dst && Entity::exists(flight.dst)) {
						scene.line(en.pos(), Entity::get(flight.dst).pos(), 0x0000ffff);
					}
					ImGui::Print(fmtc("%d %d %lu %lu", flight.flight->request, flight.flight->moving, flight.flight->pause, flight.flight->path.waypoints.size()));
					scene.line(flight.flight->origin, flight.flight->destination, 0xffffffff);
				});
			}

			if (hovering->spec->turret) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& turret = en.turret();
					if (en.spec->turretLaser) {
						ImGui::Print("Zap!");
						ImGui::SameLine();
						ImGui::PrintRight(fmtc("%dd", turret.damage()));
					}
					else {
						uint ammoId = turret.ammo();
						if (ammoId) {
							ImGui::Print(Item::get(ammoId)->title.c_str());
							ImGui::SameLine();
							ImGui::PrintRight(fmtc("%dd", turret.damage()));
						}
						else {
							ImGui::Print("(no ammo)");
						}
					}
				});
			}

			if (hovering->spec->cartStop) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& cartStop = en.cartStop();
					ImGui::Print(
						cartStop.depart == CartStop::Depart::Full ? "Depart: Full":
						cartStop.depart == CartStop::Depart::Empty ? "Depart: Empty":
						"Depart: Inactivity"
					);
				});
			}

			if (hovering->spec->cartWaypoint) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& cartWaypoint = en.cartWaypoint();
					for (auto& redirection: cartWaypoint.redirections) {
						if (!redirection.condition.valid()) continue;
						ImGui::Print(redirection.condition.readable().c_str());
						ImGui::SameLine(); ImGui::PrintRight(
							redirection.line == Monorail::Green ? "Green":
							redirection.line == Monorail::Blue ? "Blue":
							"Red"
						);
					}
				});
			}

			if (hovering->spec->cart) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& cart = en.cart();
					float speed = cart.state == Cart::State::Travel ? cart.speed: 0;
					ImGui::Print(fmtc("%dkm/h", (int)std::round(speed*(float)(60*60*60)/1000.0f)));
				});
			}

			if (hovering->spec->monorail) {
				Sim::locked([&]() {
					if (!Entity::exists(hovering->id)) return;
					auto& en = Entity::get(hovering->id);
					auto& monorail = en.monorail();
					if (en.spec->monorailStop) {
						if (monorail.filling && monorail.emptying) ImGui::Print("Filling, Emptying");
						if (monorail.filling && !monorail.emptying) ImGui::Print("Filling");
						if (!monorail.filling && monorail.emptying) ImGui::Print("Emptying");
					}
					for (auto& redirection: monorail.redirections) {
						if (!redirection.condition.valid()) continue;
						ImGui::Print(redirection.condition.readable().c_str());
						ImGui::SameLine(); ImGui::PrintRight(
							redirection.line == Monorail::Green ? "Green":
							redirection.line == Monorail::Blue ? "Blue":
							"Red"
						);
					}
				});
			}
		}

		if (keyDown(SDLK_PERIOD) && hovering) {
			tipDebug(hovering);
		}

		if (placing) {
			for (auto ge: placing->entities) {
				if (keyDown(SDLK_PERIOD)) {
					tipDebug(ge);
				}
				if (ge->spec->recipeTags.count("mining") || ge->spec->recipeTags.count("drilling")) {
					auto box = mouseGroundTarget().box().grow(0.5);
					tipResources(box, box);
					break;
				}
			}
		}

		auto controlHints = [&](auto& controls) {
			ImGui::Spacing();
			ImGui::Separator();
			struct kv {
				std::string combo;
				std::string blurb;
			};
			std::vector<kv> pairs;
			for (auto [combo,blurb]: controls) {
				pairs.push_back({combo,blurb});
			}
			std::sort(pairs.begin(), pairs.end(), [](const auto& a, const auto& b) {
				return a.blurb < b.blurb;
			});
			for (auto kv: pairs) {
				ImGui::Print(kv.blurb.c_str());
				ImGui::SameLine();
				ImGui::PrintRight(fmtc("[%s]", kv.combo.c_str()));
			}
		};

		if (gui.controlHintsSpecific.size()) controlHints(gui.controlHintsSpecific);
		if (gui.controlHintsRelated.size()) controlHints(gui.controlHintsRelated);
		if (gui.controlHintsGeneral.size()) controlHints(gui.controlHintsGeneral);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Print("Show this window");
		ImGui::SameLine();
		ImGui::PrintRight("[SpaceBar]");

		tipEnd();
	}

	if (selecting && !gui.focused) {
		Point bump = Point::Up * 0.1;
		cube(Box(selection.a + bump, selection.b + bump), 0x0000ffff);
	}

	if (Config::mode.grid && position.distance(target) < 100.0f) {
		const int grid = 30;
		Color color = Config::window.grid;

	    Point center = target.round();
	    float y = 0.0;

		for (int x = -grid; x <= grid; x++) for (int z = -grid; z <= grid; z++) {
			auto p = Point((float)x+0.5f,0,(float)z+0.5f) + center;

			if (!world.isLand(p)) continue;
			if (x != grid) {
				line(
					Point((float)x+0.0f, y, (float)z) + center,
					Point((float)x+1.0f, y, (float)z) + center,
					color,
					4.0,
					1.0
				);
			}
			if (z != grid) {
				line(
					Point((float)x, y, (float)z+0.0f) + center,
					Point((float)x, y, (float)z+1.0f) + center,
					color,
					4.0,
					1.0
				);
			}
		}
	}

	for (auto m: output.queue.recv_all()) {
		output.visible.push_back(m);
	}

	while (output.visible.size() && output.visible.front().tick < Sim::tick) {
		output.visible.pop_front();
	}
}

void Scene::updateLoading(uint w, uint h) {
	width = w;
	height = h;
}

void Scene::advance() {
	crew.job([&]() {
		stats.updateTerrain.track(frame, [&]() {
			updateTerrain();
		});
		stats.updateEntities.track(frame, [&]() {
			stats.updateEntitiesParts.track(frame, [&]() {
				Part::resetAll();
				for (auto [_,spec]: Spec::all) for (auto part: spec->parts) part->update();
			});
			updateEntities();
		});
		advanceDone.now();
	});
}

void Scene::render() {
	for (auto chunk: chunks) chunk->autoload();

	glm::vec3 sunDir = glm::normalize(glm::point(3,4,0));
	glm::vec3 sun = sunDir * range;

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);
	glViewport(0, 0, 4096, 4096);
	glDrawBuffer(GL_NONE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_FRONT);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	if (Config::window.antialias) glEnable(GL_MULTISAMPLE); else glDisable(GL_MULTISAMPLE);

	float top = range;
	float right = top*aspect;

	glm::mat4 sunView = glm::lookAt(sun, glm::vec3(glm::origin), glm::vec3(glm::up));
	glm::mat4 orthographic = glm::ortho<float>(-right,right,-top,top,0.0f,range*2);

	Mesh::prepareAll();

	glUseProgram(shader.shadow.id());

	glUniformMatrix4fv(shader.shadow.uniform("projection"), 1, GL_FALSE, &orthographic[0][0]);
	glUniformMatrix4fv(shader.shadow.uniform("view"), 1, GL_FALSE, &sunView[0][0]);

	if (Config::mode.shadowmap) {
		Mesh::shadowAll(shader.part.id());
		Mesh::shadowAll(shader.tree.id());
	}

	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glClearColor(0.4,0.74,1.0,1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);

	glm::mat4 p = perspective;
	glm::mat4 v = viewCamera;
	glm::mat4 l = orthographic * sunView;
	glm::vec3 c = camera;
	Color sunColor = 0xffffffff;

	glUseProgram(shader.terrain.id());

	float fogOffset = (float)Config::window.fog;
	GLfloat tick = frame % 0xffffff;

	glUniformMatrix4fv(shader.terrain.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.terrain.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniformMatrix4fv(shader.terrain.uniform("lightSpace"), 1, GL_FALSE, &l[0][0]);
	glUniform3fv(shader.terrain.uniform("camera"), 1, &c[0]);
	glUniform4fv(shader.terrain.uniform("ambient"), 1, &Config::window.ambient[0]);
	glUniform4fv(shader.terrain.uniform("sunColor"), 1, &sunColor[0]);
	glUniform3fv(shader.terrain.uniform("sunPosition"), 1, &sun[0]);
	glUniform4fv(shader.terrain.uniform("groundColor"), 1, &Config::window.ground[0]);

	glUniform1f(shader.terrain.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.terrain.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.terrain.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	Mesh::renderAll(shader.terrain.id(), shadowMapDepthTexture);

	glUseProgram(shader.tree.id());

	glUniformMatrix4fv(shader.tree.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.tree.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniformMatrix4fv(shader.tree.uniform("lightSpace"), 1, GL_FALSE, &l[0][0]);
	glUniform3fv(shader.tree.uniform("camera"), 1, &c[0]);
	glUniform4fv(shader.tree.uniform("ambient"), 1, &Config::window.ambient[0]);
	glUniform4fv(shader.tree.uniform("sunColor"), 1, &sunColor[0]);
	glUniform3fv(shader.tree.uniform("sunPosition"), 1, &sun[0]);

	glUniform1f(shader.tree.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.tree.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.tree.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	glUniform1i(shader.tree.uniform("treeBreeze"), Config::mode.treebreeze ? 1:0);
	glUniform1f(shader.tree.uniform("tick"), tick);

	Mesh::renderAll(shader.tree.id(), shadowMapDepthTexture);

	glUseProgram(shader.part.id());

	glUniformMatrix4fv(shader.part.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.part.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniformMatrix4fv(shader.part.uniform("lightSpace"), 1, GL_FALSE, &l[0][0]);
	glUniform3fv(shader.part.uniform("camera"), 1, &c[0]);
	glUniform4fv(shader.part.uniform("ambient"), 1, &Config::window.ambient[0]);
	glUniform4fv(shader.part.uniform("sunColor"), 1, &sunColor[0]);
	glUniform3fv(shader.part.uniform("sunPosition"), 1, &sun[0]);

	glUniform1f(shader.part.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.part.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.part.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	Mesh::renderAll(shader.part.id(), shadowMapDepthTexture);

	glUseProgram(shader.glow.id());

	glUniformMatrix4fv(shader.glow.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.glow.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniform3fv(shader.glow.uniform("camera"), 1, &c[0]);

	glUniform1f(shader.glow.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.glow.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.glow.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	Mesh::renderAll(shader.glow.id(), shadowMapDepthTexture);

	glUseProgram(shader.flame.id());

	glUniformMatrix4fv(shader.flame.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.flame.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniform3fv(shader.flame.uniform("camera"), 1, &c[0]);

	glUniform1f(shader.flame.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.flame.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.flame.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	glUniform1f(shader.flame.uniform("tick"), tick);

	Mesh::renderAll(shader.flame.id(), shadowMapDepthTexture);

	glUseProgram(shader.ghost.id());

	glUniformMatrix4fv(shader.ghost.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.ghost.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniform3fv(shader.ghost.uniform("camera"), 1, &c[0]);

	glUniform1f(shader.ghost.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.ghost.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.ghost.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	Mesh::renderAll(shader.ghost.id(), shadowMapDepthTexture);

	glUseProgram(0);

	glUseProgram(shader.water.id());

	glUniformMatrix4fv(shader.water.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.water.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniformMatrix4fv(shader.water.uniform("lightSpace"), 1, GL_FALSE, &l[0][0]);
	glUniform3fv(shader.water.uniform("camera"), 1, &c[0]);
	glUniform4fv(shader.water.uniform("ambient"), 1, &Config::window.ambient[0]);
	glUniform4fv(shader.water.uniform("sunColor"), 1, &sunColor[0]);
	glUniform3fv(shader.water.uniform("sunPosition"), 1, &sun[0]);

	glUniform1f(shader.water.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.water.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.water.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	glUniform1i(shader.water.uniform("waterWaves"), Config::mode.waterwaves ? 1:0);

	glUniform1f(shader.water.uniform("tick"), tick);

	Mesh::renderAll(shader.water.id(), shadowMapDepthTexture);

	frame++;
}

void Scene::renderLoading() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
}

bool Scene::renderSpecIcon() {
	Spec* spec = nullptr;
	for (auto [_,s]: Spec::all) {
		if (!specIconTextures.count(s)) {
			spec = s;
			break;
		}
	}

	if (!spec) return false;

	Mesh::resetAll();
	Part::resetAll();

	Point savePosition = position;
	Point saveDirection = direction;

	position = Point(0.5,1,1).normalize() * (spec->iconD > 0 ? spec->iconD
		: std::max(std::max(spec->collision.w, spec->collision.h), spec->collision.d));

	direction = (-position).normalize();

	offset = Point::Zero;

	aspect = 1.0f;
	fovy = 45.0f;

	camera = (position - groundTarget());
	viewCamera = glm::lookAt(glm::vec3(camera), glm::vec3(glm::origin), glm::vec3(glm::up));
	viewWorld = glm::lookAt(glm::dvec3(position), glm::dvec3(groundTarget()), glm::up);
	range = camera.length();

	float top = range;
	float right = top*aspect;

	perspective = glm::ortho<float>(-right,right,-top,top,0.0f,range*100);

	glm::vec3 sunDir = glm::normalize(glm::point(3,4,0));
	glm::vec3 sun = sunDir * range;

	glm::mat4 sunView = glm::lookAt(sun, glm::vec3(glm::origin), glm::vec3(glm::up));
	glm::mat4 orthographic = glm::ortho<float>(-right,right,-top,top,0.0f,range*2);

	float distance = Point::Zero.distance(position);
	auto trx = Point::South.rotation() * (Point(0,spec->iconV,0) + offset).translation();

	for (uint i = 0, l = spec->parts.size(); i < l; i++) {
		auto part = spec->parts[i];
		if (!part->show(Point::Zero, Point::South)) continue;
		GLuint group = scene.shader.part.id();
		part->instanceSpec(group, distance, trx, spec, i, 0, Point::Zero, part->color, nullptr);
	}

	Mesh::prepareAll();

	int pixels = 128;

	GLuint iconFrameBufferMS = 0;
	GLuint iconDepthBufferMS = 0;
	GLuint iconTextureMS = 0;
	GLuint iconFrameBuffer = 0;
	GLuint iconTexture = 0;

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);
	glViewport(0, 0, 4096, 4096);
	glDrawBuffer(GL_NONE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glGenTextures(1, &iconTextureMS);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, iconTextureMS);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, pixels, pixels, GL_TRUE);
	glGenRenderbuffers(1, &iconDepthBufferMS);
	glBindRenderbuffer(GL_RENDERBUFFER, iconDepthBufferMS);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, pixels, pixels);

	glGenFramebuffers(1, &iconFrameBufferMS);
	glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBufferMS);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, iconTextureMS, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, iconDepthBufferMS);

	ensuref(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "B %s", glErr());

	glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBufferMS);
	glViewport(0, 0, pixels, pixels);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);

	glm::mat4 p = perspective;
	glm::mat4 v = viewCamera;
	glm::mat4 l = orthographic * sunView;
	glm::vec3 c = camera;
	Color sunColor = 0xffffffff;
	float fogOffset = (float)Config::window.fog;

	glUseProgram(shader.part.id());

	glUniformMatrix4fv(shader.part.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.part.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniformMatrix4fv(shader.part.uniform("lightSpace"), 1, GL_FALSE, &l[0][0]);
	glUniform3fv(shader.part.uniform("camera"), 1, &c[0]);
	glUniform4fv(shader.part.uniform("ambient"), 1, &Config::window.ambient[0]);
	glUniform4fv(shader.part.uniform("sunColor"), 1, &sunColor[0]);
	glUniform3fv(shader.part.uniform("sunPosition"), 1, &sun[0]);

	glUniform1f(shader.part.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.part.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.part.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	Mesh::renderAll(shader.part.id(), shadowMapDepthTexture);

	for (int i = 0, l = sizeof(Config::toolbar.icon.sizes)/sizeof(Config::toolbar.icon.sizes[0]); i < l; i++) {
		glGenTextures(1, &iconTexture);
		glBindTexture(GL_TEXTURE_2D, iconTexture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixels, pixels, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glGenFramebuffers(1, &iconFrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iconTexture, 0);

		ensuref(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "B %s", glErr());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, iconFrameBufferMS);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, iconFrameBuffer);

		glDrawBuffer(GL_BACK);
		glBlitFramebuffer(0, 0, pixels, pixels, 0, 0, pixels, pixels, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBuffer);
		glBindTexture(GL_TEXTURE_2D, 0);

		specIconTextures[spec].push_back(iconTexture);

		glDeleteFramebuffers(1, &iconFrameBuffer);
	}

	glDeleteRenderbuffers(1, &iconDepthBufferMS);
	glDeleteFramebuffers(1, &iconFrameBufferMS);
	glDeleteTextures(1, &iconTextureMS);

	position = savePosition;
	direction = saveDirection;
	return true;
}

bool Scene::renderItemIcon() {
	Item* item = nullptr;
	for (auto [_,i]: Item::names) {
		if (!itemIconTextures.count(i->id)) {
			item = i;
			break;
		}
	}

	if (!item) return false;

	Mesh::resetAll();
	Part::resetAll();

	Point savePosition = position;
	Point saveDirection = direction;

	position = Point(0.5,1,1).normalize() * 0.6;

	direction = (-position).normalize();

	offset = Point::Zero;

	aspect = 1.0f;
	fovy = 45.0f;

	camera = (position - groundTarget());
	viewCamera = glm::lookAt(glm::vec3(camera), glm::vec3(glm::origin), glm::vec3(glm::up));
	viewWorld = glm::lookAt(glm::dvec3(position), glm::dvec3(groundTarget()), glm::up);
	range = camera.length();

	float top = range;
	float right = top*aspect;

	perspective = glm::ortho<float>(-right,right,-top,top,0.0f,range*100);

	glm::vec3 sunDir = glm::normalize(glm::point(3,4,0));
	glm::vec3 sun = sunDir * range;

	glm::mat4 sunView = glm::lookAt(sun, glm::vec3(glm::origin), glm::vec3(glm::up));
	glm::mat4 orthographic = glm::ortho<float>(-right,right,-top,top,0.0f,range*2);

	float distance = Point::Zero.distance(position);
	auto trx = Point::South.rotation() * (Point(0,item->armV-0.4,0) + offset).translation();

	for (uint i = 0, l = item->parts.size(); i < l; i++) {
		auto part = item->parts[i];
		if (!part->show(Point::Zero, Point::South)) continue;
		GLuint group = scene.shader.part.id();
		part->instance(group, distance, trx);
	}

	Mesh::prepareAll();

	int pixels = 128;

	GLuint iconFrameBufferMS = 0;
	GLuint iconDepthBufferMS = 0;
	GLuint iconTextureMS = 0;
	GLuint iconFrameBuffer = 0;
	GLuint iconTexture = 0;

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);
	glViewport(0, 0, 4096, 4096);
	glDrawBuffer(GL_NONE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glGenTextures(1, &iconTextureMS);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, iconTextureMS);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, pixels, pixels, GL_TRUE);
	glGenRenderbuffers(1, &iconDepthBufferMS);
	glBindRenderbuffer(GL_RENDERBUFFER, iconDepthBufferMS);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, pixels, pixels);

	glGenFramebuffers(1, &iconFrameBufferMS);
	glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBufferMS);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, iconTextureMS, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, iconDepthBufferMS);

	ensuref(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "B %s", glErr());

	glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBufferMS);
	glViewport(0, 0, pixels, pixels);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);

	glm::mat4 p = perspective;
	glm::mat4 v = viewCamera;
	glm::mat4 l = orthographic * sunView;
	glm::vec3 c = camera;
	Color sunColor = 0xffffffff;
	float fogOffset = (float)Config::window.fog;

	glUseProgram(shader.part.id());

	glUniformMatrix4fv(shader.part.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.part.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniformMatrix4fv(shader.part.uniform("lightSpace"), 1, GL_FALSE, &l[0][0]);
	glUniform3fv(shader.part.uniform("camera"), 1, &c[0]);
	glUniform4fv(shader.part.uniform("ambient"), 1, &Config::window.ambient[0]);
	glUniform4fv(shader.part.uniform("sunColor"), 1, &sunColor[0]);
	glUniform3fv(shader.part.uniform("sunPosition"), 1, &sun[0]);

	glUniform1f(shader.part.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.part.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.part.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	Mesh::renderAll(shader.part.id(), shadowMapDepthTexture);

	for (int i = 0, l = sizeof(Config::toolbar.icon.sizes)/sizeof(Config::toolbar.icon.sizes[0]); i < l; i++) {
		glGenTextures(1, &iconTexture);
		glBindTexture(GL_TEXTURE_2D, iconTexture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixels, pixels, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glGenFramebuffers(1, &iconFrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iconTexture, 0);

		ensuref(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "B %s", glErr());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, iconFrameBufferMS);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, iconFrameBuffer);

		glDrawBuffer(GL_BACK);
		glBlitFramebuffer(0, 0, pixels, pixels, 0, 0, pixels, pixels, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBuffer);
		glBindTexture(GL_TEXTURE_2D, 0);

		itemIconTextures[item->id].push_back(iconTexture);

		glDeleteFramebuffers(1, &iconFrameBuffer);
	}

	glDeleteRenderbuffers(1, &iconDepthBufferMS);
	glDeleteFramebuffers(1, &iconFrameBufferMS);
	glDeleteTextures(1, &iconTextureMS);

	position = savePosition;
	direction = saveDirection;
	return true;
}

bool Scene::renderFluidIcon() {
	Fluid* fluid = nullptr;
	for (auto [_,i]: Fluid::names) {
		if (!fluidIconTextures.count(i->id)) {
			fluid = i;
			break;
		}
	}

	if (!fluid) return false;

	Mesh::resetAll();
	Part::resetAll();

	Point savePosition = position;
	Point saveDirection = direction;

	position = Point(0.5,1,1).normalize() * 0.6;

	direction = (-position).normalize();

	offset = Point::Zero;

	aspect = 1.0f;
	fovy = 45.0f;

	camera = (position - groundTarget());
	viewCamera = glm::lookAt(glm::vec3(camera), glm::vec3(glm::origin), glm::vec3(glm::up));
	viewWorld = glm::lookAt(glm::dvec3(position), glm::dvec3(groundTarget()), glm::up);
	range = camera.length();

	float top = range;
	float right = top*aspect;

	perspective = glm::ortho<float>(-right,right,-top,top,0.0f,range*100);

	glm::vec3 sunDir = glm::normalize(glm::point(3,4,0));
	glm::vec3 sun = sunDir * range;

	glm::mat4 sunView = glm::lookAt(sun, glm::vec3(glm::origin), glm::vec3(glm::up));
	glm::mat4 orthographic = glm::ortho<float>(-right,right,-top,top,0.0f,range*2);

//	float distance = Point::Zero.distance(position);
	auto trx = Point::South.rotation() * (Point(0,-0.25,0) + offset).translation();

	scene.bits.droplet->instance(scene.shader.part.id(), trx, fluid->color, 0.0f, Part::NOSHADOW);

	Mesh::prepareAll();

	int pixels = 128;

	GLuint iconFrameBufferMS = 0;
	GLuint iconDepthBufferMS = 0;
	GLuint iconTextureMS = 0;
	GLuint iconFrameBuffer = 0;
	GLuint iconTexture = 0;

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFrameBuffer);
	glViewport(0, 0, 4096, 4096);
	glDrawBuffer(GL_NONE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glGenTextures(1, &iconTextureMS);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, iconTextureMS);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, pixels, pixels, GL_TRUE);
	glGenRenderbuffers(1, &iconDepthBufferMS);
	glBindRenderbuffer(GL_RENDERBUFFER, iconDepthBufferMS);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, pixels, pixels);

	glGenFramebuffers(1, &iconFrameBufferMS);
	glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBufferMS);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, iconTextureMS, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, iconDepthBufferMS);

	ensuref(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "B %s", glErr());

	glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBufferMS);
	glViewport(0, 0, pixels, pixels);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);

	glm::mat4 p = perspective;
	glm::mat4 v = viewCamera;
	glm::mat4 l = orthographic * sunView;
	glm::vec3 c = camera;
	Color sunColor = 0xffffffff;
	float fogOffset = (float)Config::window.fog;

	glUseProgram(shader.part.id());

	glUniformMatrix4fv(shader.part.uniform("projection"), 1, GL_FALSE, &p[0][0]);
	glUniformMatrix4fv(shader.part.uniform("view"), 1, GL_FALSE, &v[0][0]);
	glUniformMatrix4fv(shader.part.uniform("lightSpace"), 1, GL_FALSE, &l[0][0]);
	glUniform3fv(shader.part.uniform("camera"), 1, &c[0]);
	glUniform4fv(shader.part.uniform("ambient"), 1, &Config::window.ambient[0]);
	glUniform4fv(shader.part.uniform("sunColor"), 1, &sunColor[0]);
	glUniform3fv(shader.part.uniform("sunPosition"), 1, &sun[0]);

	glUniform1f(shader.part.uniform("fogDensity"), Config::window.fogDensity);
	glUniform1f(shader.part.uniform("fogOffset"), fogOffset);
	glUniform4fv(shader.part.uniform("fogColor"), 1, &Config::window.fogColor[0]);

	Mesh::renderAll(shader.part.id(), shadowMapDepthTexture);

	for (int i = 0, l = sizeof(Config::toolbar.icon.sizes)/sizeof(Config::toolbar.icon.sizes[0]); i < l; i++) {
		glGenTextures(1, &iconTexture);
		glBindTexture(GL_TEXTURE_2D, iconTexture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixels, pixels, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		glGenFramebuffers(1, &iconFrameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iconTexture, 0);

		ensuref(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "B %s", glErr());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, iconFrameBufferMS);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, iconFrameBuffer);

		glDrawBuffer(GL_BACK);
		glBlitFramebuffer(0, 0, pixels, pixels, 0, 0, pixels, pixels, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_FRAMEBUFFER, iconFrameBuffer);
		glBindTexture(GL_TEXTURE_2D, 0);

		fluidIconTextures[fluid->id].push_back(iconTexture);

		glDeleteFramebuffers(1, &iconFrameBuffer);
	}

	glDeleteRenderbuffers(1, &iconDepthBufferMS);
	glDeleteFramebuffers(1, &iconFrameBufferMS);
	glDeleteTextures(1, &iconTextureMS);

	position = savePosition;
	direction = saveDirection;
	return true;
}

void Scene::build(Spec* spec, Point dir) {
	planDrop();
	if (spec) {
		auto plan = new Plan(Point::Zero);
		auto ge = new GuiFakeEntity(spec);
		ge->dir(dir);
		ge->floor(0.0f); // build level applied by plan
		plan->add(ge);
		planPush(plan);
	}
}

void Scene::saveFramebuffer() {
	int width = 4096;
	int height = 4096;
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	stbi_write_png("out.png", width, height, nrChannels, buffer.data(), stride);
}

void Scene::print(std::string m) {
	output.queue.send({m,Sim::tick+(60*30)});
}

Scene::Texture Scene::loadTexture(const char* path) {
	return Popup::loadTexture(path);
}

void Scene::planPush(Plan* plan) {
	if (!plans.size() || plans.front() != plan) {
		plans.shove(plan);
	}

	while (plans.size() > 2) {
		delete plans.pop();
	}

	placing = plans.front();
}

void Scene::planDrop() {
	placing = nullptr;
}

void Scene::planPaste() {
	planDrop();
	if (plans.size()) {
		placing = plans.front();
	}
}

