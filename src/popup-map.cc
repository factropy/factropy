#include "common.h"
#include "config.h"
#include "popup.h"

#include "../imgui/setup.h"

using namespace ImGui;

MapPopup::MapPopup() : Popup() {
}

MapPopup::~MapPopup() {
}

void MapPopup::prepare() {
	int w = world.scenario.size;
	int h = w/2;

	std::vector<Tile> tiles(w * w);

	auto tile = [&](World::XY at) {
		ensure(at.x >= -h && at.x < h);
		ensure(at.y >= -h && at.y < h);
		return &tiles[(at.y+h)*w + (at.x+h)];
	};

	for (int16_t y = -h; y < h; y++) {
		for (int16_t x = -h; x < h; x++) {
			auto mtile = tile({x,y});
			mtile->terrain = TileLand;
		}
	}

	for (auto& wtile: world.tiles) {
		if (wtile.hill()) tile(wtile.at())->terrain = TileHill;
		if (wtile.lake()) tile(wtile.at())->terrain = TileLake;
	}

	ctiles.clear();
	for (int16_t y = -h; y < h; y++) {
		for (int16_t x = -h; x < h; x++) {
			auto mtile = tile({x,y});
			if (mtile->terrain == TileLand) continue;
			ctiles.push_back({.x = x, .y = y, .w = 1, .terrain = mtile->terrain});
			for (; x < h && tile({x,y})->terrain == mtile->terrain; x++) ctiles.back().w++;
		}
	}

	auto structure = [&](std::vector<Structure>& group, Box box, Color color) {
		int16_t x0 = box.x-(box.w*0.5);
		int16_t y0 = box.z-(box.d*0.5);
		int16_t x1 = box.x+(box.w*0.5);
		int16_t y1 = box.z+(box.d*0.5);
		group.push_back({ .x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1, .c = color});
	};

	std::vector<int8_t> beltgrid(w*w);
	for (auto& b: beltgrid) b = 0;

	std::vector<int8_t> pipegrid(w*w);
	for (auto& b: pipegrid) b = 0;

	slabs.clear();
	belts.clear();
	pipes.clear();
	buildings.clear();
	generators.clear();
	others.clear();
	tubes.clear();
	damaged.clear();

	for (auto& en: Entity::all) {
		if (en.spec->junk) continue;
		if (!en.spec->align) continue;
		if (en.isGhost()) continue;

		int64_t x = (int16_t)std::floor(en.pos().x);
		int64_t y = (int16_t)std::floor(en.pos().z);
		ensure(x >= -h && x < h);
		ensure(y >= -h && y < h);

		if (en.spec->conveyor) {
			if (en.dir() == Point::South) beltgrid[(y+h)*w+(x+h)] = 1;
			if (en.dir() == Point::North) beltgrid[(y+h)*w+(x+h)] = 1;
			if (en.dir() == Point::East) beltgrid[(y+h)*w+(x+h)] = 2;
			if (en.dir() == Point::West) beltgrid[(y+h)*w+(x+h)] = 2;
		}

		if (en.spec->pipe && en.spec->collision.w < 1.1 && en.spec->collision.d < 1.1) {
			if (en.dir() == Point::South) pipegrid[(y+h)*w+(x+h)] = 1;
			if (en.dir() == Point::North) pipegrid[(y+h)*w+(x+h)] = 1;
			if (en.dir() == Point::East) pipegrid[(y+h)*w+(x+h)] = 2;
			if (en.dir() == Point::West) pipegrid[(y+h)*w+(x+h)] = 2;
		}

		if (en.spec->tube && en.tube().length) {
			tubes.push_back({
				.x0 = (int16_t)(en.pos().x),
				.y0 = (int16_t)(en.pos().z),
				.x1 = (int16_t)(en.tube().target().x),
				.y1 = (int16_t)(en.tube().target().z),
			});
		}

		if (en.spec->monorail) {
			auto arrive = en.monorail().arrive();
			auto depart = en.monorail().depart();
			rails.push_back({
				.x0 = (int16_t)(arrive.x),
				.y0 = (int16_t)(arrive.z),
				.x1 = (int16_t)(depart.x),
				.y1 = (int16_t)(depart.z),
			});
			for (auto& rl: en.monorail().railsOut()) {
				auto steps = rl.rail.steps(5.0);
				ensure(steps.size() > 1);
				for (int i = 0, l = steps.size()-1; i < l; i++) {
					rails.push_back({
						.x0 = (int16_t)(steps[i].x),
						.y0 = (int16_t)(steps[i].z),
						.x1 = (int16_t)(steps[i+1].x),
						.y1 = (int16_t)(steps[i+1].z),
					});
				}
			}
		}

		if (en.spec->health && en.health < en.spec->health) {
			structure(damaged, en.box(), 0xff0000ff);
			continue;
		}

		if (en.spec->generateElectricity || en.spec->bufferElectricity) {
			structure(generators, en.box(), 0x004225ff);
			continue;
		}

		if (en.spec->slab || en.spec->pile) {
			structure(slabs, en.box(), 0xbbbbbbff);
			continue;
		}

		if (en.spec->crafter && en.spec->crafterMiner) {
			structure(buildings, en.box(), 0xCD853Fff);
			continue;
		}

		if (en.spec->crafter && en.spec->crafterSmelter) {
			structure(buildings, en.box(), 0xDAA520FF);
			continue;
		}

		if (en.spec->crafter && en.spec->crafterChemistry) {
			structure(buildings, en.box(), 0x008080FF);
			continue;
		}

		if (en.spec->crafter) {
			structure(buildings, en.box(), 0x3672a4FF);
			continue;
		}

		if (en.spec->pipe && en.spec->collision.w > 1.1 && en.spec->collision.d > 1.1) {
			structure(buildings, en.box(), 0xee9500ff);
			continue;
		}

		structure(others, en.box(), 0x666666ff);
	}

	auto lines = [&](std::vector<Structure>& group, std::vector<int8_t>& grid, Color color) {
		for (int16_t y = -h; y < h; y++) {
			for (int16_t x = -h; x < h; x++) {
				int8_t d = grid[(y+h)*w+(x+h)];
				if (d != 2) continue;
				int16_t x0 = x;
				int16_t y0 = y;
				for (; x < h && grid[(y+h)*w+(x+h+1)] == d; x++);
				int16_t x1 = x+1;
				int16_t y1 = y0+1;
				group.push_back({.x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1, .c = color});
			}
		}
		for (int16_t x = -h; x < h; x++) {
			for (int16_t y = -h; y < h; y++) {
				int8_t d = grid[(y+h)*w+(x+h)];
				if (d != 1) continue;
				int16_t x0 = x;
				int16_t y0 = y;
				for (; y < h && grid[(y+h+1)*w+(x+h)] == d; y++);
				int16_t x1 = x0+1;
				int16_t y1 = y+1;
				group.push_back({.x0 = x0, .y0 = y0, .x1 = x1, .y1 = y1, .c = color});
			}
		}
	};

	lines(belts, beltgrid, 0x555555ff);
	lines(pipes, pipegrid, 0xee9500ff);
}

void MapPopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		big();

		SetNextWindowContentSize(ImVec2(8192, 8192));
		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));

		Begin("Map##map", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus);

		float titleBar = GetFontSize() + GetStyle().FramePadding.y*2;

		auto windowPos = ImVec2(
			GetWindowPos().x, // + (GetWindowSize().x - GetContentRegionMax().x - GetContentRegionMin().x)/2.0f,
			GetWindowPos().y + titleBar // + (GetWindowSize().y - GetContentRegionMax().y - GetContentRegionMin().y)/2.0f
		);

		auto windowSize = ImVec2(
			GetWindowSize().x,
			GetWindowSize().y - titleBar
		);

		float w = world.scenario.size;
		float h = w/2;

		if (IsWindowAppearing()) {
			prepare();
			target = scene.target;
			if (retarget != Point::Zero) {
				target = retarget.floor(0.0);
				retarget = Point::Zero;
				scale = 1;
			}
		}
		else {
			if (GetIO().MouseWheel < 0) scale = std::min(16, scale+1);
			if (GetIO().MouseWheel > 0) scale = std::max( 1, scale-1);
		}

		float t = 4.0f/scale;

		if (IsWindowHovered() && IsMouseDragging(ImGuiMouseButton_Right)) {
			target.x -= GetMouseDragDelta(ImGuiMouseButton_Right).x/t;
			target.z -= GetMouseDragDelta(ImGuiMouseButton_Right).y/t;
			ResetMouseDragDelta(ImGuiMouseButton_Right);
		}

		SetScrollX(4096);
		SetScrollY(4096);

		auto origin = ImVec2(
			windowPos.x + windowSize.x/2 - (target.x*t),
			windowPos.y + windowSize.y/2 - (target.z*t)
		);

		auto pointer = ImVec2(
			(GetMousePos().x - origin.x)/t,
			(GetMousePos().y - origin.y)/t
		);

		auto l0 = ImVec2(origin.x - (h*t), origin.y - (h*t));
		auto l1 = ImVec2(origin.x + (h*t), origin.y + (h*t));
		GetWindowDrawList()->AddRectFilled(l0, l1, GetColorU32(Color(0x999999ff).gamma()));

		for (auto& ctile: ctiles) {
			int x = ctile.x;
			int y = ctile.y;

			auto p0 = ImVec2(origin.x + (float)x*t, origin.y + (float)y*t);
			auto p1 = ImVec2(origin.x + (float)x*t + t*ctile.w, origin.y + (float)y*t + t);

			Color color = ctile.terrain == TileHill ? Color(0.45,0.4,0.4,1.0): Color(0x010160FF).gamma();
			GetWindowDrawList()->AddRectFilled(p0, p1, GetColorU32(color));
		}

		auto structures = [&](std::vector<Structure>& group) {
			for (auto s: group) {
				auto p0 = ImVec2(origin.x + (float)s.x0*t, origin.y + (float)s.y0*t);
				auto p1 = ImVec2(origin.x + (float)s.x1*t, origin.y + (float)s.y1*t);
				GetWindowDrawList()->AddRectFilled(p0, p1, GetColorU32(s.c.gamma()));
			}
		};

		structures(slabs);
		structures(others);
		structures(belts);
		structures(pipes);
		structures(buildings);
		structures(generators);
		structures(damaged);

		for (auto& s: tubes) {
			auto p0 = ImVec2(origin.x + (float)s.x0*t, origin.y + (float)s.y0*t);
			auto p1 = ImVec2(origin.x + (float)s.x1*t, origin.y + (float)s.y1*t);
			GetWindowDrawList()->AddLine(p0, p1, GetColorU32(Color(0xccccccff).gamma()), t);
		}

		for (auto& s: rails) {
			auto p0 = ImVec2(origin.x + (float)s.x0*t, origin.y + (float)s.y0*t);
			auto p1 = ImVec2(origin.x + (float)s.x1*t, origin.y + (float)s.y1*t);
			GetWindowDrawList()->AddLine(p0, p1, GetColorU32(Color(0x444444ff).gamma()), t);
		}

		auto vehicle = [&](Entity* en, Color color) {
			auto pos = en->pos();
			auto dir = en->dir();
			auto ahead = (Point::South*(en->spec->collision.d*0.5)).transform(dir.rotation());
			auto front = pos + ahead;
			auto back = pos - ahead;
			auto p0 = ImVec2(origin.x + front.x*t, origin.y + front.z*t);
			auto p1 = ImVec2(origin.x + back.x*t, origin.y + back.z*t);
			GetWindowDrawList()->AddLine(p0, p1, GetColorU32(Color(color).gamma()), t*en->spec->collision.w);
		};

		for (auto& router: Router::all) {
			if (router.en->isGhost()) continue;
			if (router.alert.notice) vehicle(router.en, (scene.frame/2)%2 ? 0x000000ff: 0xeeeeeeff);
			if (router.alert.warning) vehicle(router.en, (scene.frame/2)%2 ? 0x000000ff: 0xeeeeeeff);
			if (router.alert.critical) vehicle(router.en, (scene.frame/2)%2 ? 0x000000ff: 0xeeeeeeff);
		}

		for (auto& car: Monocar::all) {
			if (car.en->isGhost()) continue;
			vehicle(car.en, car.blocked && (scene.frame/2)%2 ? 0x000000ff: 0xeeeeeeff);
		}

		for (auto& cart: Cart::all) {
			if (cart.en->isGhost()) continue;
			vehicle(cart.en, cart.blocked && (scene.frame/2)%2 ? 0x000000ff: 0xeeeeeeff);
		}

		for (auto& flight: FlightPath::all) {
			if (flight.en->isGhost()) continue;
			vehicle(flight.en, 0xffffffff);
		}

		for (auto& zeppelin: Zeppelin::all) {
			auto& en = Entity::get(zeppelin.id);
			if (en.isGhost()) continue;
			vehicle(&en, en.spec->consumeFuel ? 0xffa500ff: 0xffffffff);
		}

		{
			auto half = GetFontSize()/2.0f;
			float offset = GetFontSize();
			float hcenter = windowPos.x + windowSize.x/2.0f;
			float vcenter = windowPos.y + windowSize.y/2.0f;

			GetWindowDrawList()->AddText(GetFont(), GetFontSize(), ImVec2(hcenter - CalcTextSize("N").x/2.0f, windowPos.y+offset-half), GetColorU32(Color(0xffffffff).gamma()), "N");
			GetWindowDrawList()->AddText(GetFont(), GetFontSize(), ImVec2(hcenter - CalcTextSize("S").x/2.0f, windowPos.y+windowSize.y-offset-half), GetColorU32(Color(0xffffffff).gamma()), "S");

			GetWindowDrawList()->AddText(GetFont(), GetFontSize(), ImVec2(windowPos.x+offset - CalcTextSize("W").x/2.0f, vcenter-half), GetColorU32(Color(0xffffffff).gamma()), "W");
			GetWindowDrawList()->AddText(GetFont(), GetFontSize(), ImVec2(windowPos.x+windowSize.x-offset - CalcTextSize("E").x/2.0f, vcenter-half), GetColorU32(Color(0xffffffff).gamma()), "E");
		}

		if (IsWindowHovered() && IsMouseClicked(ImGuiMouseButton_Left)) {
			scene.view(Point(pointer.x, 0, pointer.y));
			show(false);
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);

		PopStyleVar(1);

		auto tile = world.get(World::XY(
			(int)std::floor(pointer.x),
			(int)std::floor(pointer.y)
		));

		if (tile && scene.keyDown(SDLK_SPACE)) {
			auto box = Point(tile->x, 0, tile->y).box().grow(0.5);
			scene.tipBegin(0.75f);
			Header("Resources");
			scene.tipResources(box, box);
			scene.tipEnd();
		}
	});
}

