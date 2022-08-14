#include "common.h"
#include "config.h"
#include "popup.h"
#include "sim.h"
#include "entity.h"
#include "vehicle.h"
#include "energy.h"
#include "string.h"
#include "enemy.h"
#include "chunk.h"
#include "scene.h"
#include "gui.h"
#include "save.h"
#include "catenate.h"

#include "../imgui/setup.h"
#include "stb_image.h"

#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>

using namespace ImGui;

namespace {
	void openURL(const std::string& url) {
	#if defined(_WIN32)
		if (system(fmtc("explorer %s", url)) < 0)
			notef("openURL failed");
	#endif
	#if defined(__linux__) || defined(__FreeBSD__)
		// Alternatives: firefox, x-www-browser
		if (system(fmtc("xdg-open %s &", url)) < 0)
			notef("openURL failed");
	#endif
	}
}

Popup::Popup() {
	mouseOver = false;
	visible = false;
}

Popup::~Popup() {
}

void Popup::big() {

	h = (float)Config::height(0.75f);
	w = h*1.45;

	center();
}

void Popup::small() {

	h = (float)Config::height(0.375f);
	w = h*1.45;

	center();
}

void Popup::medium() {

	h = (float)Config::height(0.6f);
	w = h*0.9;

	center();
}

void Popup::narrow() {

	h = (float)Config::height(0.5f);
	w = h*0.55;

	center();
}

void Popup::center() {

	const ImVec2 size = {
		(float)w,(float)h
	};

	const ImVec2 pos = {
		((float)Config::window.width-size.x)/2.0f,
		((float)Config::window.height-size.y)/2.0f
	};

	SetNextWindowSize(size, ImGuiCond_Always);
	SetNextWindowPos(pos, ImGuiCond_Always);
}

Popup::Texture Popup::loadTexture(const char* path) {
	int width, height, components;
	unsigned char *data = stbi_load(path, &width, &height, &components, 0);
	ensuref(data, "texture invalid or missing: %s", path);

	GLuint id = 0;
	GLenum format;

	switch (components) {
		case 1: format = GL_RED; break;
		case 3: format = GL_RGB; break;
		case 4: format = GL_RGBA; break;
		default: ensuref(0, "texture invalid components: %d", components);
	}

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	stbi_image_free(data);

	return {.id = id, .w = width, .h = height};
}

void Popup::freeTexture(Texture texture) {
	glDeleteTextures(1, &texture.id);
}

int Popup::iconTier(float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;

	int iconSize = 0;
	for (int i = 0, l = sizeof(Config::toolbar.icon.sizes)/sizeof(Config::toolbar.icon.sizes[0]); i < l; i++) {
		iconSize = i;
		if ((int)std::floor(Config::toolbar.icon.sizes[i]) >= (int)std::floor(pix)) break;
	}

	return iconSize;
}

ImTextureID Popup::itemIconChoose(Item* item, float pix) {
	return (ImTextureID)scene.itemIconTextures[item->id][iconTier(pix)];
}

void Popup::itemIcon(Item* item, float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;
	Image(itemIconChoose(item, pix), ImVec2(pix, pix), ImVec2(0, 1), ImVec2(1, 0));
}

bool Popup::itemIconButton(Item* item, float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;
	return ImageButton(itemIconChoose(item, pix), ImVec2(pix, pix), ImVec2(0, 1), ImVec2(1, 0));
}

ImTextureID Popup::fluidIconChoose(Fluid* fluid, float pix) {
	return (ImTextureID)scene.fluidIconTextures[fluid->id][iconTier(pix)];
}

void Popup::fluidIcon(Fluid* fluid, float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;
	Image(fluidIconChoose(fluid, pix), ImVec2(pix, pix), ImVec2(0, 1), ImVec2(1, 0));
}

bool Popup::fluidIconButton(Fluid* fluid, float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;
	return ImageButton(fluidIconChoose(fluid, pix), ImVec2(pix, pix), ImVec2(0, 1), ImVec2(1, 0));
}

ImTextureID Popup::recipeIconChoose(Recipe* recipe, float pix) {
	auto chooseItem = [&](auto from) {
		minivec<Item*> items;
		for (auto& [iid,_]: from) {
			items.push(Item::get(iid));
		}
		std::sort(items.begin(), items.end());
		return items.size() ? items.front()->id: 0;
	};

	auto chooseFluid = [&](auto from) {
		minivec<Fluid*> fluids;
		for (auto& [fid,_]: from) {
			fluids.push(Fluid::get(fid));
		}
		std::sort(fluids.begin(), fluids.end(), [](const auto a, const auto b) {
			return a->title < b->title;
		});
		return fluids.size() ? fluids.front()->id: 0;
	};

	if (recipe->mine) {
		return itemIconChoose(Item::get(recipe->mine), pix);
	}

	if (recipe->drill) {
		return fluidIconChoose(Fluid::get(recipe->drill), pix);
	}

	if (recipe->fluid) {
		return fluidIconChoose(Fluid::get(recipe->fluid), pix);
	}

	if (recipe->outputItems.size() > 0) {
		return itemIconChoose(Item::get(chooseItem(recipe->outputItems)), pix);
	}

	if (recipe->outputFluids.size() > 0) {
		return fluidIconChoose(Fluid::get(chooseFluid(recipe->outputFluids)), pix);
	}

	if (recipe->inputItems.size() > 0) {
		return itemIconChoose(Item::get(chooseItem(recipe->inputItems)), pix);
	}

	if (recipe->inputFluids.size() > 0) {
		return fluidIconChoose(Fluid::get(chooseFluid(recipe->inputFluids)), pix);
	}

	ensuref("recipe '%s' has no icon", recipe->title);
	return 0;
}

void Popup::recipeIcon(Recipe* recipe, float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;
	Image(recipeIconChoose(recipe, pix), ImVec2(pix, pix), ImVec2(0, 1), ImVec2(1, 0));
}

bool Popup::recipeIconButton(Recipe* recipe, float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;

	auto state = ImageButton(recipeIconChoose(recipe, pix), ImVec2(pix, pix), ImVec2(0, 1), ImVec2(1, 0));

	if (!state && tipBegin()) {
		Print(recipe->title.c_str());
		int i = 0;

		auto item = [&](uint iid, uint count) {
			if (i++) {
				Print(" + ");
				SameLine();
			}
			itemIcon(Item::get(iid));
			SameLine();
			Print(fmtc("(%u)", count));
			SameLine();
		};

		auto fluid = [&](uint fid, uint count) {
			if (i++) {
				Print(" + ");
				SameLine();
			}
			fluidIcon(Fluid::get(fid));
			SameLine();
			Print(fmtc("(%u)", count));
			SameLine();
		};

		i = 0;
		for (auto& [iid,count]: recipe->inputItems) {
			item(iid, count);
		}
		for (auto& [fid,count]: recipe->inputFluids) {
			fluid(fid, count);
		}

		if (i) NewLine();
		Indent();
		Print("= ");
		SameLine();

		i = 0;
		if (recipe->mine) {
			item(recipe->mine, 1);
		}
		if (recipe->drill) {
			fluid(recipe->drill, 100);
		}
		for (auto& [iid,count]: recipe->outputItems) {
			item(iid, count);
		}
		for (auto& [fid,count]: recipe->outputFluids) {
			fluid(fid, count);
		}

		Unindent();
		tipEnd();
	}

	return state;
}

ImTextureID Popup::specIconChoose(Spec* spec, float pix) {
	return (ImTextureID)scene.specIconTextures[spec][iconTier(pix)];
}

void Popup::specIcon(Spec* spec, float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;
	Image(specIconChoose(spec, pix), ImVec2(pix, pix), ImVec2(0, 1), ImVec2(1, 0));
}

bool Popup::specIconButton(Spec* spec, float pix) {
	pix = (pix < 4.0f) ? GetFontSize(): pix;
	return ImageButton(specIconChoose(spec, pix), ImVec2(pix, pix), ImVec2(0, 1), ImVec2(1, 0));
}

uint Popup::itemPicker(bool open, std::function<bool(Item*)> show) {

	if (open) OpenPopup("##item-picker");

	if (!show) show = [](Item* item) {
		return item->manufacturable();
	};

	w = (float)Config::height(0.375f);
	h = w*0.6;

	const ImVec2 size = {
		(float)w,(float)h
	};

	const ImVec2 pos = {
		((float)Config::window.width-size.x)/2.0f,
		((float)Config::window.height-size.y)/2.0f
	};

	SetNextWindowSize(size, ImGuiCond_Always);
	SetNextWindowPos(pos, ImGuiCond_Appearing);

	itemPicked = 0;

	if (BeginPopup("##item-picker")) {
		if (!open && scene.keyReleased(SDLK_ESCAPE)) CloseCurrentPopup();

		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(GetStyle().ItemSpacing.x, GetStyle().ItemSpacing.x));

		float cell = (GetContentRegionAvail().x - (GetStyle().ItemSpacing.x*19)) / 10;
		int row = 0;
		int col = 0;

		for (auto category: Item::display) {
			for (auto group: category->display) {
				if (row++ > 0 && col > 0) {
					NewLine();
					col = 0;
				}
				for (auto item: group->display) {
					if (!show(item)) continue;
					if (col++ == 10) {
						NewLine();
						col = 1;
					}
					if (itemIconButton(item, cell)) {
						itemPicked = item->id;
						CloseCurrentPopup();
					}
					if (IsItemHovered()) {
						tip(item->title.c_str());
					}
					SameLine();
				}
			}
		}

		PopStyleVar(1);
		EndPopup();
	}

	return itemPicked;
}

Recipe* Popup::recipePicker(bool open, std::function<bool(Recipe*)> show) {

	if (open) OpenPopup("##recipe-picker");

	if (!show) show = [](Recipe* recipe) {
		return recipe->manufacturable();
	};

	w = (float)Config::height(0.375f);
	h = w*0.6;

	const ImVec2 size = {
		(float)w,(float)h
	};

	const ImVec2 pos = {
		((float)Config::window.width-size.x)/2.0f,
		((float)Config::window.height-size.y)/2.0f
	};

	SetNextWindowSize(size, ImGuiCond_Always);
	SetNextWindowPos(pos, ImGuiCond_Appearing);

	recipePicked = 0;

	if (BeginPopup("##recipe-picker")) {
		if (!open && scene.keyReleased(SDLK_ESCAPE)) CloseCurrentPopup();

		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(GetStyle().ItemSpacing.x, GetStyle().ItemSpacing.x));

		float cell = (GetContentRegionAvail().x - (GetStyle().ItemSpacing.x*19)) / 10;
		int col = 0;

		for (auto [_,recipe]: Recipe::names) {
			if (!show(recipe)) continue;
			if (col++ == 10) {
				NewLine();
				col = 1;
			}
			if (recipeIconButton(recipe, cell)) {
				recipePicked = recipe;
				CloseCurrentPopup();
			}
			SameLine();
		}

		PopStyleVar(1);
		EndPopup();
	}

	return recipePicked;
}

void Popup::topRight() {

	const ImVec2 size = {
		(float)w,(float)h
	};

	const ImVec2 pos = {
		(float)Config::window.width-size.x,
		0.0f,
	};

	SetNextWindowSize(size, ImGuiCond_Always);
	SetNextWindowPos(pos, ImGuiCond_Always);
}

void Popup::bottomLeft() {

	const ImVec2 size = {
		(float)w,(float)h
	};

	const ImVec2 pos = {
		0.0f,
		(float)Config::window.height-size.y,
	};

	SetNextWindowSize(size, ImGuiCond_Always);
	SetNextWindowPos(pos, ImGuiCond_Always);
}

void Popup::show(bool state) {
	opened = state && !visible;
	visible = state;
}

void Popup::draw() {
}

void Popup::prepare() {
}

float Popup::relativeWidth(float w) {
	return GetContentRegionAvail().x * w;
}

std::string Popup::wrap(uint line, std::string text) {
	uint gap = 0;
	uint cur = 0;
	for (uint i = 0, l = text.size(); i < l; i++) {
		if (isspace(text[i])) gap = i;
		if (cur >= line && gap > 0) {
			text[gap] = '\n';
			cur = 0;
		} else {
			cur++;
		}
	}
	return text;
}

bool Popup::tipBegin() {
	if (IsItemHovered()) {
		auto& style = GetStyle();
		ImVec2 tooltip_pos = GetMousePos();
		tooltip_pos.x += (Config::window.hdpi/96.0) * 16;
		tooltip_pos.y += (Config::window.vdpi/96.0) * 16;
		SetNextWindowPos(tooltip_pos);
		SetNextWindowBgAlpha(style.Colors[ImGuiCol_PopupBg].w * 1.0f);
	    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
		Begin("##popup-tip", nullptr, flags);
		return true;
	}
	return false;
}

void Popup::tipEnd() {
	End();
}

bool Popup::tipSmallBegin() {
	if (tipBegin()) {
		PushFont(Config::sidebar.font.imgui);
		PushStyleColor(ImGuiCol_Text, ImColorSRGB(0xffffccff));
		return true;
	}
	return false;
}

void Popup::tipSmallEnd() {
	PopStyleColor(1);
	PopFont();
	tipEnd();
}

void Popup::tip(const std::string& s) {
	tipSmallBegin();
	PushTextWrapPos((float)Config::window.width*0.1f);
	Print(s.c_str());
	PopTextWrapPos();
	tipSmallEnd();
}

void Popup::crafterNotice(Crafter& crafter) {

	auto& en = Entity::get(crafter.id);

	if (!en.isEnabled()) {
		Warning("Disabled");
	}
	else
	if (en.spec->consumeFuel && !en.burner().fueled()) {
		Alert("Fuel");
	}
	else
	if (crafter.recipe && !crafter.recipe->licensed) {
		Alert("Unlock Recipe");
	}
	else
	if (crafter.working && en.spec->consumeElectricity && crafter.efficiency < 0.1) {
		Alert("Electricity");
	}
	else
	if (crafter.working && en.spec->consumeElectricity && crafter.efficiency < 0.99) {
		Warning("Electricity");
	}
	else
	if (crafter.working || crafter.interval) {
		Notice("Working");
	}
	else
	if (!crafter.working && crafter.insufficientResources()) {
		Alert("Insufficient Resources");
	}
	else
	if (!crafter.working && crafter.excessProducts()) {
		Alert("Excess Products");
	}
	else {
		Warning("Idle");
	}
}

void Popup::launcherNotice(Launcher& launcher) {

	if (launcher.working) {
		if (launcher.progress < 0.1) {
			Notice("Launching");
		}
		else
		if (launcher.progress < 0.45) {
			Notice("Ascent");
		}
		else
		if (launcher.progress < 0.55) {
			Notice("Orbit");
		}
		else
		if (launcher.progress < 0.8) {
			Notice("Descent");
		}
		else
		if (launcher.progress < 1.0) {
			Notice("Landing");
		}
	}
	else {
		auto& en = Entity::get(launcher.id);

		if (!en.isEnabled()) {
			Warning("Disabled");
			return;
		}
		else
		if (!launcher.fueled()) {
			Alert("Fuel");
		}
		else
		if (launcher.activate) {
			Notice("Launch window scheduled");
		}
		else
		if (!launcher.cargo.size()) {
			Warning("No payload set");
		}
		else
		if (!Entity::get(launcher.id).store().isEmpty()) {
			Notice("Loading");
		}
		else {
			Warning("Idle");
		}
	}
}

void Popup::powerpoleNotice(PowerPole& pole) {

	if (!pole.network) {
		Warning("Disconnected");
	}
	else
	if (pole.network->lowPower()) {
		Alert("Low Power");
	}
	else
	if (pole.network->brownOut()) {
		Warning("Brown-out");
	}
	else
	if (pole.network->noCapacity()) {
		Warning("No Production");
	}
	else {
		Notice("Connected");
	}
}

void Popup::goalRateChart(Goal* goal, Goal::Rate& rate, float h) {

	uint minute = 60*60;
	uint hour = minute*60;
	uint tstep = minute*10;

	auto supplyCounts = rate.intervalSums(goal->period, tstep);
	auto intervalCounts = rate.intervalSums(goal->period, hour);

	auto formatCount = [&](uint count) {
		if (count >= 1000000) return fmt("%uM", count/1000000);
		if (count >= 1000) return fmt("%uk", count/1000);
		return fmt("%d", count);
	};

	auto pass = ImColorSRGB(0x004400ff);
	auto fail = ImColorSRGB(0x222222ff);
	auto head = ImColorSRGB(0x333333ff);
	auto bar = ImColorSRGB(0xddddddff);

	int columns = goal->period/hour;
	float column = GetContentRegionAvail().x/columns;

	PushFont(Config::sidebar.font.imgui);

	PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0,0));
	if (BeginTable(fmtc("#goal-sums-%s-%u", goal->name, rate.iid), goal->period/hour)) {
		for (int i = 0, l = goal->period/hour; i < l; i++) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, column);
		}
		for (int i = 0, l = goal->period/hour; i < l; i++) {
			TableNextColumn();
			TableSetBgColor(ImGuiTableBgTarget_RowBg0, head);
			TextCentered(formatCount(intervalCounts[i]).c_str());
		}
		EndTable();
	}
	PopStyleVar(1);

	uint ymax = 0;
	for (auto count: supplyCounts) ymax = std::max(count, ymax);

	uint ystep = 2000000;
	if (ymax < 1000000) ystep = 200000;
	if (ymax < 100000) ystep = 20000;
	if (ymax < 10000) ystep = 2000;
	if (ymax < 1000) ystep = 200;
	if (ymax < 100) ystep = 20;
	if (ymax < 10) ystep = 2;

	uint intervals = columns;
	uint intervalBars = hour/tstep;
	float intervalBarWidth = column / (float)intervalBars;

	uint ysteps = std::max(2u, (ymax/ystep)+1);
	float yrange = ysteps*ystep;

	auto top = GetCursorPos();

	auto origin = top;
	origin.x += GetWindowPos().x;
	origin.y += GetWindowPos().y;
	origin.y -= GetScrollY();

	float gap = std::max(1.0f, GetStyle().ItemSpacing.x/2);

	for (uint interval = 0; interval < intervals; interval++) {
		bool last = interval+1 == intervals;

		auto intervalOrigin = ImVec2(origin.x + column*interval, origin.y);

		auto p0 = ImVec2(intervalOrigin.x, intervalOrigin.y);
		auto p1 = ImVec2(intervalOrigin.x+column-(last ? 0: gap), intervalOrigin.y + h);

		GetWindowDrawList()->AddRectFilled(p0, p1, intervalCounts[interval] >= rate.count ? pass: fail);

		for (int i = 0, l = intervalBars; i < l; i++) {
			float count = supplyCounts[interval*intervalBars+i];
			float intervalBarHeight = (count/yrange)*h;

			float x0 = intervalOrigin.x + intervalBarWidth*i + gap;
			float x1 = intervalOrigin.x + intervalBarWidth*(i+1) - gap;

			float y1 = intervalOrigin.y + h;
			float y0 = y1 - std::max(1.0f, intervalBarHeight);

			auto p0 = ImVec2(x0, y0);
			auto p1 = ImVec2(x1, y1);

			GetWindowDrawList()->AddRectFilled(p0, p1, bar);
		}
	}
	SetCursorPos(ImVec2(top.x, top.y + h + gap*2));

	PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0,0));
	if (BeginTable(fmtc("#goal-hours-%s-%u", goal->name, rate.iid), goal->period/hour)) {
		for (uint64_t tick = 0; tick < goal->period; tick += hour) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, column);
		}
		for (uint64_t tick = 0; tick < goal->period; tick += hour) {
			TableNextColumn();
			TableSetBgColor(ImGuiTableBgTarget_RowBg0, head);
			TextCentered(fmtc("-%dh", (goal->period-tick)/hour));
		}
		EndTable();
	}
	PopStyleVar(1);

	SpacingV();
	PopFont();
}

LoadingPopup::LoadingPopup() : Popup() {
	banner = loadTexture("assets/banner.png");
}

LoadingPopup::~LoadingPopup() {
	freeTexture(banner);
}

void LoadingPopup::draw() {
	small();
	Begin(fmtc("%s##loading", Config::mode.saveName), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	ImageBanner(banner.id, banner.w, banner.h);
	if (progress < 1.0) SmallBar(std::max(0.01f, progress));

	BeginChild("##messages");
		PushFont(Config::sidebar.font.imgui);
		for (auto msg: log) Print(msg);
		SetScrollHereY();
		PopFont();
	EndChild();

	mouseOver = IsWindowHovered();
	subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
	End();
}

void LoadingPopup::print(std::string msg) {
	log.push_back(msg);
	while (log.size() > 1000) log.erase(log.begin());
}

StatsPopup2::StatsPopup2() : Popup() {
}

StatsPopup2::~StatsPopup2() {
}

void StatsPopup2::prepare() {
	for (auto [_,item]: Item::names) itemsAlpha.push_back(item);
	std::sort(itemsAlpha.begin(), itemsAlpha.end(), [&](auto a, auto b) { return a->title < b->title; });
	filter[0] = 0;
}

void StatsPopup2::draw() {
	bool showing = true;

	uint64_t second = 60;
	uint64_t minute = second*60;
	uint64_t hour = minute*60;

	int id = 0;

	big();
	Begin("Stats", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	if (BeginTabBar("stats-tabs", ImGuiTabBarFlags_None)) {

		if (BeginTabItem("Energy")) {

			Sim::locked([&]() {
				if (Sim::tick < 60*60) return;
			});

			EndTabItem();
		}

		if (BeginTabItem("Production")) {
			SpacingV();
			Warning("Imagine an old-school under-construction gif here :)");

			SpacingV();
			if (BeginTable("##prod-form", 2)) {
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x/4.0);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x/4.0);

				TableNextColumn();
				PushItemWidth(-1);
				InputTextWithHint("##prod-filter", "filter items", filter, sizeof(filter));
				inputFocused = IsItemActive();
				PopItemWidth();

				TableNextColumn();
				PushItemWidth(-1);
				if (BeginCombo("##prod-filter-mode", any ? "Match any word": "Match all words")) {
					if (Selectable("Match any word", any)) any = true;
					if (Selectable("Match all words", !any)) any = false;
					EndCombo();
				}
				PopItemWidth();
				EndTable();
			}

			double prod[61];
			double cons[61];

			auto empty = [&]() {
				for (int i = 0; i < 61; i++) prod[i] = 0;
				for (int i = 0; i < 61; i++) cons[i] = 0;
			};

			float column = GetContentRegionAvail().x/2.0;

			auto chart60 = [&](std::function<void(void)> generate) {
				if (ImPlot::BeginPlot(fmtc("##%d", ++id), ImVec2(-1,column*0.2), ImPlotFlags_CanvasOnly | ImPlotFlags_NoChild | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs)) {
					empty();
					generate();
					ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
					ImPlot::SetupAxis(ImAxis_X2, nullptr, ImPlotAxisFlags_NoDecorations);
					ImPlot::PlotLine("prod", &prod[1], 59);
					ImPlot::PlotLine("cons", &cons[1], 59);
					ImPlot::EndPlot();
				}
			};

			auto needle = std::string(filter);

			auto match = [&](const std::string& title) {
				for (auto sub: discatenate(needle, " ")) {
					auto it = std::search(
						title.begin(), title.end(), sub.begin(), sub.end(),
						[](char a, char b) {
							return std::tolower(a) == std::tolower(b);
						});
					if (any && it != title.end()) return true;
					if (!any && it == title.end()) return false;
				}
				return !any;
			};

			Sim::locked([&]() {
				if (Sim::tick < 60*60) return;

				for (auto item: itemsAlpha) {
					ensure(item->title.size());
					if (needle.size() && !match(item->title)) continue;

					if (BeginTable(fmtc("#item-prod-%u", item->id), 2)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, column);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, column);

						TableNextColumn();
						TextCentered(fmtc("%s / min", item->title));
						chart60([&]() {
							int i = Sim::tick <= hour ? (hour-Sim::tick)/minute: 0;
							for (uint64_t tick = Sim::tick <= hour ? 0: Sim::tick-hour; tick < Sim::tick; tick += minute, i++) {
								prod[i] = item->production.minutes[item->production.minute(tick)];
								cons[i] = item->consumption.minutes[item->consumption.minute(tick)];
							}
						});

						TableNextColumn();
						TextCentered(fmtc("%s / sec", item->title));
						chart60([&]() {
							int i = Sim::tick <= minute ? (minute-Sim::tick)/second: 0;
							for (uint64_t tick = Sim::tick <= minute ? 0: Sim::tick-minute; tick < Sim::tick; tick += second, i++) {
								prod[i] = item->production.seconds[item->production.second(tick)];
								cons[i] = item->consumption.seconds[item->consumption.second(tick)];
							}
						});

						EndTable();
					}
				}
			});

			EndTabItem();
		}

		if (BeginTabItem("SIM")) {

			struct Series {
				std::string title;
				TimeSeries* ts = nullptr;
				double data[61];
			};

			std::deque<Series> plots = {
				{ .title = "Chunk", .ts = &Sim::statsChunk},
				{ .title = "EntityPre", .ts = &Sim::statsEntityPre},
				{ .title = "EntityPost", .ts = &Sim::statsEntityPost},
				{ .title = "Ghost", .ts = &Sim::statsGhost},
				{ .title = "Networker", .ts = &Sim::statsNetworker},
				{ .title = "Pile", .ts = &Sim::statsPile},
				{ .title = "Explosive", .ts = &Sim::statsExplosive},
				{ .title = "Store", .ts = &Sim::statsStore},
				{ .title = "Arm", .ts = &Sim::statsArm},
				{ .title = "Crafter", .ts = &Sim::statsCrafter},
				{ .title = "Venter", .ts = &Sim::statsVenter},
				{ .title = "Effector", .ts = &Sim::statsEffector},
				{ .title = "Launcher", .ts = &Sim::statsLauncher},
				{ .title = "Conveyor", .ts = &Sim::statsConveyor},
				{ .title = "Unveyor", .ts = &Sim::statsUnveyor},
				{ .title = "Loader", .ts = &Sim::statsLoader},
				{ .title = "Balancer", .ts = &Sim::statsBalancer},
				{ .title = "Path", .ts = &Sim::statsPath},
				{ .title = "Vehicle", .ts = &Sim::statsVehicle},
				{ .title = "Cart", .ts = &Sim::statsCart},
				{ .title = "Pipe", .ts = &Sim::statsPipe},
				{ .title = "DronePort", .ts = &Sim::statsDepot},
				{ .title = "Drone", .ts = &Sim::statsDrone},
				{ .title = "Missile", .ts = &Sim::statsMissile},
				{ .title = "Explosion", .ts = &Sim::statsExplosion},
				{ .title = "Turret", .ts = &Sim::statsTurret},
				{ .title = "Computer", .ts = &Sim::statsComputer},
				{ .title = "Router", .ts = &Sim::statsRouter},
				{ .title = "Enemy", .ts = &Sim::statsEnemy},
				{ .title = "Zeppelin", .ts = &Sim::statsZeppelin},
				{ .title = "FlightLogistic", .ts = &Sim::statsFlightLogistic},
				{ .title = "FlightPath", .ts = &Sim::statsFlightPath},
				{ .title = "Tube", .ts = &Sim::statsTube},
				{ .title = "Teleporter", .ts = &Sim::statsTeleporter},
				{ .title = "Monorail", .ts = &Sim::statsMonorail},
				{ .title = "Monocar", .ts = &Sim::statsMonocar},
				{ .title = "Source", .ts = &Sim::statsSource},
				{ .title = "PowerPole", .ts = &Sim::statsPowerPole},
				{ .title = "Charger", .ts = &Sim::statsCharger},
			};

			std::sort(plots.begin(), plots.end(), [](const auto& a, const auto& b) {
				return a.title < b.title;
			});

			Sim::locked([&]() {
				if (Sim::tick < 60) return;

				double yMax = 0.0;
				for (auto& plot: plots) {
					int i = 0;
					for (uint64_t tick = Sim::tick-60; tick < Sim::tick; tick++, i++) {
						plot.data[i] = plot.ts->ticks[plot.ts->tick(tick)];
					}
					yMax = std::max(yMax, plot.ts->secondMax);
				}

				double yLim = std::ceil(yMax);

				PushFont(Config::sidebar.font.imgui);
				if (ImPlot::BeginPlot(fmtc("##%d", ++id), ImVec2(-1,-1), ImPlotFlags_NoChild | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs)) {
					ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
					ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, yLim, ImPlotCond_Always);

					for (auto& plot: plots) {
						ImPlot::PlotLine(plot.title.c_str(), &plot.data[1], 59);
					}

					ImPlot::EndPlot();
				}
				PopFont();
			});

			EndTabItem();
		}

		if (BeginTabItem("GUI")) {

			struct Series {
				std::string title;
				TimeSeries* ts = nullptr;
				double data[61];
			};

			std::deque<Series> plots = {
				{.title = "update", .ts = &scene.stats.update},
				{.title = "updateTerrain", .ts = &scene.stats.updateTerrain},
				{.title = "updateEntities", .ts = &scene.stats.updateEntities},
				{.title = "updateEntitiesParts", .ts = &scene.stats.updateEntitiesParts},
				{.title = "updateEntitiesFind", .ts = &scene.stats.updateEntitiesFind},
				{.title = "updateEntitiesLoad", .ts = &scene.stats.updateEntitiesLoad},
				{.title = "updateEntitiesHover", .ts = &scene.stats.updateEntitiesHover},
				{.title = "updateEntitiesInstances", .ts = &scene.stats.updateEntitiesInstances},
				{.title = "updateEntitiesItems", .ts = &scene.stats.updateEntitiesItems},
				{.title = "updateCurrent", .ts = &scene.stats.updateCurrent},
				{.title = "updatePlacing", .ts = &scene.stats.updatePlacing},
				{.title = "render", .ts = &scene.stats.render},
			};

			if (scene.frame < 60) return;

			double yMax = 0.0;
			for (auto& plot: plots) {
				int i = 0;
				for (uint64_t tick = scene.frame-60; tick < scene.frame; tick++, i++) {
					plot.data[i] = plot.ts->ticks[plot.ts->tick(tick)];
				}
				yMax = std::max(yMax, plot.ts->secondMax);
			}

			double yLim = 16.0;
			while (yLim < yMax) yLim = std::floor(yLim + 16);

			if (ImPlot::BeginPlot(fmtc("##%d", ++id), ImVec2(-1,-1), ImPlotFlags_NoChild | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs)) {
				ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, yLim, ImPlotCond_Always);

				for (auto& plot: plots) {
					ImPlot::PlotLine(plot.title.c_str(), &plot.data[1], 59);
				}

				ImPlot::EndPlot();
			}

			EndTabItem();
		}

		if (BeginTabItem("Debug")) {
			auto readable = [&](std::size_t bytes) {
				std::size_t K = 1024;
				if (bytes < K) return fmt("%llu B", bytes);
				if (bytes < K*K) return fmt("%llu kB", bytes/K);
				if (bytes < K*K*K) return fmt("%llu MB", bytes/K/K);
				return fmt("%llu GB", bytes/K/K/K);
			};

			if (BeginTable("memory", 3, ImGuiTableFlags_RowBg)) {
				TableSetupColumn("component");
				TableSetupColumn("memory");
				TableSetupColumn("extant");

				TableHeadersRow();

				TableNextRow();
				TableNextColumn();
				Print("World");
				TableNextColumn();
				Print(readable(world.memory()).c_str());
				TableNextColumn();

				TableNextRow();
				TableNextColumn();
				Print("Chunk");
				TableNextColumn();
				Print(readable(Chunk::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Chunk::all.size()));

				TableNextRow();
				TableNextColumn();
				Print("Entity");
				TableNextColumn();
				Print(readable(Entity::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Entity::all.size()));

				TableNextRow();
				TableNextColumn();
				Print("Arm");
				TableNextColumn();
				Print(readable(Arm::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Arm::all.size()));

				TableNextRow();
				TableNextColumn();
				Print("Conveyor");
				TableNextColumn();
				Print(readable(Conveyor::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Conveyor::managed.size() + Conveyor::unmanaged.size()));

				TableNextRow();
				TableNextColumn();
				Print("Loader");
				TableNextColumn();
				Print(readable(Loader::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Loader::all.size()));

				TableNextRow();
				TableNextColumn();
				Print("Store");
				TableNextColumn();
				Print(readable(Store::memory()).c_str());
				TableNextColumn();
				Print(fmtc("%d", Store::all.size()));

				EndTable();
			}

			if (BeginTable("recipes", 3, ImGuiTableFlags_RowBg)) {
				TableSetupColumn("recipe");
				TableSetupColumn("total energy");
				TableSetupColumn("raw materials");

				TableHeadersRow();

				std::vector<Recipe*> recipes;
				for (auto [_,recipe]: Recipe::names) recipes.push_back(recipe);
				reorder(recipes, [&](auto a, auto b) { return a->title < b->title; });

				for (auto recipe: recipes) {
					TableNextRow();

					TableNextColumn();
					Print(recipe->title.c_str());

					TableNextColumn();
					Print(recipe->totalEnergy().format().c_str());

					TableNextColumn();
					for (auto stack: recipe->totalRawItems()) {
						Print(fmt("%s(%u)", Item::get(stack.iid)->name, stack.size));
					}
					for (auto amount: recipe->totalRawFluids()) {
						Print(fmt("%s(%u)", Fluid::get(amount.fid)->name, amount.size));
					}
				}

				EndTable();
			}
			EndTabItem();
		}

		EndTabBar();
	}

	mouseOver = IsWindowHovered();
	subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);

	End();
	if (visible) show(showing);
}

EntityPopup2::EntityPopup2() : Popup() {
}

EntityPopup2::~EntityPopup2() {
}

void EntityPopup2::useEntity(uint eeid) {
	eid = eeid;
}

void EntityPopup2::draw() {
	bool showing = true;

	Sim::locked([&]() {
		if (!eid || !Entity::exists(eid) || Entity::get(eid).isGhost()) {
			eid = 0;
			show(false);
			return;
		}

		Entity &en = Entity::get(eid);

		if (opened && en.spec->named) {
			std::snprintf(name, sizeof(name), "%s", en.name().c_str());
		}

		if (opened && en.spec->networker) {
			auto& networker = en.networker();
			for (uint i = 0, l = std::min((uint)networker.interfaces.size(), 4u); i < l; i++) {
				auto& interface = networker.interfaces[i];
				std::snprintf(interfaces[i], sizeof(interfaces[i]), "%s", interface.ssid.c_str());
			}
		}

		auto focusedTab = [&]() {
			bool focused = opened;
			opened = false;
			return (focused ? ImGuiTabItemFlags_SetSelected: ImGuiTabItemFlags_None) | ImGuiTabItemFlags_NoPushId;
		};

		auto signalKey = [&](int id, Signal::Key& key, bool metas = true) {
			enum EntryType {
				ITEM = 0,
				FLUID,
			};

			struct Entry {
				EntryType type;
				std::string name;
				union {
					Item* item;
					Fluid* fluid;
				};
			};

			std::vector<Entry> entries;

			for (auto& [_,item]: Item::names) {
				entries.push_back((Entry){.type = ITEM, .name = item->title, .item = item});
			}

			for (auto& [_,fluid]: Fluid::names) {
				entries.push_back((Entry){.type = FLUID, .name = fluid->title, .fluid = fluid});
			}

			reorder(entries, [&](auto& a, auto& b) {
				return a.name < b.name;
			});

			std::string selected = "(empty)";

			if (key.valid()) {
				selected = (key == Signal::Key(Signal::Meta::Now))
					? fmt("Now %llds", (int64_t)Sim::tick/60U): key.title();
			}

			if (BeginCombo(fmtc("##key%d", id), selected.c_str())) {
				if (Selectable("(empty)", key == Signal::Key())) {
					key = Signal::Key();
				}
				for (auto& label: Signal::labels) {
					auto lkey = Signal::Key(label.name);
					if (label.drop && key != lkey) continue;
					if (Selectable(lkey.title().c_str(), key == lkey)) {
						key = Signal::Key(label.name);
					}
				}
				if (metas) {
					for (auto meta: std::vector<Signal::Meta>{
						Signal::Meta::Any,
						Signal::Meta::All,
						Signal::Meta::Now,
					}){
						auto mkey = Signal::Key(meta);
						if (Selectable(mkey.title().c_str(), key == mkey)) {
							key = Signal::Key(meta);
						}
					}
				}
				for (auto& entry: entries) {
					if (entry.type == ITEM && Selectable(entry.name.c_str(), key == Signal::Key(entry.item))) {
						key = Signal::Key(entry.item);
					}
					if (entry.type == FLUID && Selectable(entry.name.c_str(), key == Signal::Key(entry.fluid))) {
						key = Signal::Key(entry.fluid);
					}
				}
				for (auto letter: std::vector<Signal::Letter>{
					Signal::Letter::A,
					Signal::Letter::B,
					Signal::Letter::C,
					Signal::Letter::D,
					Signal::Letter::E,
					Signal::Letter::F,
					Signal::Letter::G,
					Signal::Letter::H,
					Signal::Letter::I,
					Signal::Letter::J,
					Signal::Letter::K,
					Signal::Letter::L,
					Signal::Letter::M,
					Signal::Letter::N,
					Signal::Letter::O,
					Signal::Letter::P,
					Signal::Letter::Q,
					Signal::Letter::R,
					Signal::Letter::S,
					Signal::Letter::T,
					Signal::Letter::U,
					Signal::Letter::V,
					Signal::Letter::W,
					Signal::Letter::X,
					Signal::Letter::Y,
					Signal::Letter::Z,
				}){
					auto lkey = Signal::Key(letter);
					if (Selectable(lkey.name().c_str(), key == lkey)) {
						key = Signal::Key(letter);
					}
				}
				EndCombo();

				if (key == Signal::Key(Signal::Meta::Now) && IsItemHovered()) tip(
					"!% means divisible modulo without remainder. (SIGNAL % VALUE) == 0."
					" Similar to the pure math | (vertical pipe) operator meaning 'divides',"
					" but players with a software background see the pipe as the more"
					" familiar logical OR."
				);
			}
		};

		auto signalConstant = [&](int id, Signal& signal, bool lone = true) {
			float space = GetContentRegionAvail().x;
			if (BeginTable(fmtc("##signal-const-%d", id), lone ? 3: 2)) {
				TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.25);
				if (lone) TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.334);

				TableNextColumn();
				SetNextItemWidth(-1);
				signalKey(id, signal.key, false);

				TableNextColumn();
				SetNextItemWidth(-1);
				InputInt(fmtc("##val%d", id), &signal.value);

				if (lone) {
					TableNextColumn();
					Print("Constant Signal");
				}

				EndTable();
			}
		};

		auto signalCondition = [&](int id, Signal::Condition& condition, bool lone, std::function<bool(void)> check = nullptr) {

			if (lone) {
				float space = GetContentRegionAvail().x;
				BeginTable(fmtc("##signal-cond-%d", id), check || en.spec->networker ? 4: 3);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.1);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.25);
				if (check) TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.332);
			}

			TableNextColumn();
			SetNextItemWidth(-1);
			signalKey(id, condition.key);

			TableNextColumn();
			SetNextItemWidth(-1);

			auto ops = std::map<Signal::Operator,std::string>{
				{ Signal::Operator::Eq,  "=",  },
				{ Signal::Operator::Ne,  "!=", },
				{ Signal::Operator::Lt,  "<",  },
				{ Signal::Operator::Lte, "<=", },
				{ Signal::Operator::Gt,  ">",  },
				{ Signal::Operator::Gte, ">=", },
				{ Signal::Operator::NMod, "!%",},
			};

			if (BeginCombo(fmtc("##op%d", id), ops[condition.op].c_str())) {
				for (auto [op,opName]: ops) {
					if (Selectable(opName.c_str(), condition.op == op)) {
						condition.op = op;
					}
				}
				EndCombo();
			}

			TableNextColumn();
			SetNextItemWidth(-1);
			InputInt(fmtc("##cmp%d", id), &condition.cmp);

			if (check && !condition.valid()) {
				TableNextColumn();
				Print("invalid");
			} else
			if (check) {
				TableNextColumn();
				Print(check() ? "true": "false");
			}
//			else
//			if (en.spec->networker) {
//				TableNextColumn();
//				auto network = en.networker().input().network;
//				Print(condition.evaluate(network ? network->signals: Signal::NoSignals) ? "true": "false");
//			}

			if (lone) {
				EndTable();
			}
		};

		struct Option {
			uint id;
			const char* title;
			bool operator<(const Option& other) {
				return std::string(title) < std::string(other.title);
			};
		};

		std::vector<Option> optionFluids;

		for (auto& [_,fluid]: Fluid::names) {
			if (fluid->raw || fluid->manufacturable())
				optionFluids.push_back({fluid->id, fluid->title.c_str()});
		}

		std::sort(optionFluids.begin(), optionFluids.end(), [](const auto& a, const auto& b) {
			return Fluid::get(a.id)->title < Fluid::get(b.id)->title;
		});

		bool operableStore = en.spec->store && (en.spec->storeSetLower || en.spec->storeSetUpper);

		if (operableStore || en.spec->computer || en.spec->router || en.spec->powerpole) medium(); else small();
		Begin(fmtc("%s###entity", en.title()), &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if (BeginTabBar("entity-tabs", ImGuiTabBarFlags_None)) {

			if (en.spec->arm && BeginTabItem("Arm", nullptr, focusedTab())) {
				Arm& arm = en.arm();

				PushID("arm");

				SpacingV();
				SmallBar(arm.orientation);

				SpacingV();
				Section("Item Filtering");

				uint remove = 0;
				uint insert = 0;

				int i = 0;
				for (auto iid: arm.filter) {
					if (ButtonStrip(i++, fmtc(" %s ", Item::get(iid)->title))) remove = iid;
					if (IsItemHovered()) tip("Remove item from filter set.");
				}

				auto pick = ButtonStrip(i++, arm.filter.size() ? " + Item ": " Set Filter ");
				if (IsItemHovered()) tip("Add item to filter set.");
				if (itemPicker(pick)) insert = itemPicked;

				if (remove) arm.filter.erase(remove);
				if (insert) arm.filter.insert(insert);

				SpacingV();
				Section("Belt Lane Control");

				Checkbox("Input near", &arm.inputNear);
				Checkbox("Input far", &arm.inputFar);
				Checkbox("Output near", &arm.outputNear);
				Checkbox("Output far", &arm.outputFar);

				SpacingV();
				Section("Enable Rule");

				const char* monitor = nullptr;
				switch (arm.monitor) {
					case Arm::Monitor::InputStore: {
						monitor = "Source store";
						break;
					}
					case Arm::Monitor::OutputStore: {
						monitor = "Destination store";
						break;
					}
					case Arm::Monitor::Network: {
						monitor = "Wifi Network";
						break;
					}
				}
				if (BeginCombo("Monitor##moniotor-combo", monitor)) {
					if (Selectable("Source store", arm.monitor == Arm::Monitor::InputStore)) {
						arm.monitor = Arm::Monitor::InputStore;
					}
					if (Selectable("Destination store", arm.monitor == Arm::Monitor::OutputStore)) {
						arm.monitor = Arm::Monitor::OutputStore;
					}
					if (Selectable("Wifi Network", arm.monitor == Arm::Monitor::Network)) {
						arm.monitor = Arm::Monitor::Network;
					}
					EndCombo();
				}
				signalCondition(0, arm.condition, true, [&]() { return arm.checkCondition(); });

				PopID();

				EndTabItem();
			}

			if (en.spec->loader && BeginTabItem("Loader", nullptr, focusedTab())) {
				Loader& loader = en.loader();

				PushID("loader");

				SpacingV();
				Section("Item Filtering");

				uint remove = 0;
				uint insert = 0;

				int i = 0;
				for (auto iid: loader.filter) {
					if (ButtonStrip(i++, fmtc(" %s ", Item::get(iid)->title))) remove = iid;
					if (IsItemHovered()) tip("Remove item from filter set.");
				}

				auto pick = ButtonStrip(i++, loader.filter.size() ? " + Item ": " Set Filter ");
				if (IsItemHovered()) tip("Add item to filter set.");
				if (itemPicker(pick)) insert = itemPicked;

				if (remove) loader.filter.erase(remove);
				if (insert) loader.filter.insert(insert);

				SpacingV();
				Section("Enable Rule");
				if (BeginCombo("Monitor", loader.monitor == Loader::Monitor::Store ? "Store": "Network")) {
					if (Selectable("Store", loader.monitor == Loader::Monitor::Store)) {
						loader.monitor = Loader::Monitor::Store;
					}
					if (Selectable("Network", loader.monitor == Loader::Monitor::Network)) {
						loader.monitor = Loader::Monitor::Network;
					}
					EndCombo();
				}
				signalCondition(0, loader.condition, true, [&]() { return loader.checkCondition(); });

				Checkbox("Ignore container limits", &loader.ignore);
				if (IsItemHovered()) tip("Loader will override container item limits.");

				PopID();

				EndTabItem();
			}

			if (en.spec->pipeValve && BeginTabItem("Valve", nullptr, focusedTab())) {
				Pipe& pipe = en.pipe();

				PushID("pipe");
				LevelBar(pipe.network->level());

				int level = (int)(pipe.overflow * 100.0f);
				if (SliderInt("level", &level, 0, 100)) {
					pipe.overflow = std::round((float)level) / 100.0f;
				}

				if (pipe.filter) {
					Fluid* fluid = Fluid::get(pipe.filter);
					if (Button(fluid->title.c_str())) {
						pipe.filter = 0;
					}
				}

				if (BeginCombo("filter", nullptr)) {
					for (auto& option: optionFluids) {
						if (pipe.filter != option.id && Selectable(option.title, false)) {
							pipe.filter = option.id;
						}
					}
					EndCombo();
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->pipeCapacity > Liquid::l(10000) && BeginTabItem("Fluid Tank", nullptr, focusedTab())) {
				Pipe& pipe = en.pipe();

				PushID("fluid-tank");

				Print(pipe.network->fid ? Fluid::get(pipe.network->fid)->title: "(empty)");
				SameLine();
				PrintRight(fmtc("%s / %s",
					Liquid(pipe.network->fid ? ((float)en.spec->pipeCapacity.value * pipe.network->level()): 0).format(),
					Liquid(pipe.network->fid ? pipe.network->count(pipe.network->fid): 0).format()
				));

				SmallBar(pipe.network->level());

				Pipe::transmitters.erase(en.id);
				Checkbox("Transmit fluid level", &en.pipe().transmit);
				if (en.pipe().transmit && !en.isGhost()) {
					Pipe::transmitters.insert(en.id);
				}

				PopID();

				EndTabItem();
			}

			if (operableStore && BeginTabItem("Storage", nullptr, focusedTab())) {
				Store& store = en.store();

				PushID("store-manual");

					int id = 0;

					uint clear = 0;
					uint down = 0;

					Mass usage = store.usage();
					Mass limit = store.limit();

					SpacingV();
					SmallBar(usage.portion(limit));

					float width = GetContentRegionAvail().x;
					SpacingV();

					auto pick = Button(" + Item ");
					if (IsItemHovered()) tip(
						"Accept an item with limits."
					);

					if (itemPicker(pick)) {
						store.levelSet(Item::get(itemPicked)->id, 0, 0);
					}

					SameLine();
					if (Button(" + Construction ")) {
						std::set<Item*> items;
						for (auto [_,spec]: Spec::all) {
							if (!spec->licensed) continue;
							if (spec->junk) continue;
							for (auto [iid,_]: spec->materials) {
								items.insert(Item::get(iid));
							}
						}
						for (auto item: items) {
							if (store.level(item->id)) continue;
							store.levelSet(item->id, 0, 0);
						}
					}
					if (IsItemHovered()) tip(
						"Accept all construction materials with limits."
					);

					if (en.spec->depot) {
						SameLine();
						Checkbox("Purge", &store.purge);
						if (IsItemHovered()) tip("Drones will move items without limits to overflow containers.");
					}

					PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemInnerSpacing.x/2,GetStyle().ItemInnerSpacing.y/2));

					SpacingV();
					if (BeginTable(fmtc("##store-levels-%u", id++), en.spec->crafter ? 10: 9)) {
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, GetFontSize());
						TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, width*0.1f);
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, width*0.04f);
						TableSetupColumn("Lower", ImGuiTableColumnFlags_WidthFixed, width*0.2f);
						TableSetupColumn("Upper", ImGuiTableColumnFlags_WidthFixed, width*0.2f);
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, width*0.04f);
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, width*0.04f);
						TableSetupColumn(fmtc("##%d", id++), ImGuiTableColumnFlags_WidthFixed, width*0.04f);

						TableNextRow(ImGuiTableRowFlags_Headers);
						TableNextColumn();
						TableHeader("");

						TableNextColumn();
						TableHeader("Item");
						if (IsItemClicked()) store.sortAlpha();

						TableNextColumn();
						TableHeader("#");
						TableNextColumn();
						TableHeader("");
						TableNextColumn();
						TableHeader("Lower");
						TableNextColumn();
						TableHeader("Upper");
						TableNextColumn();
						TableHeader("");
						TableNextColumn();
						TableHeader("");

						if (en.spec->crafter) {
							TableNextColumn();
							TableHeader("craft");
						}

						for (auto& level: store.levels) {
							Item *item = Item::get(level.iid);
							TableNextRow();

							int limit = (int)std::max(level.upper, (uint)store.limit().value/item->mass.value);
							int step  = (int)std::max(1U, (uint)limit/100);

							TableNextColumn();
							itemIcon(item);

							TableNextColumn();
							Print(fmtc("%s", item->title));

							TableNextColumn();
							Print(fmtc("%d", store.count(level.iid)));

							TableNextColumn();
							if (store.isRequesting(level.iid)) {
								Print("r"); if (IsItemHovered()) tip("requesting");
							} else
							if (store.isActiveProviding(level.iid)) {
								Print("a"); if (IsItemHovered()) tip("actively providing");
							} else
							if (store.isProviding(level.iid)) {
								Print("p"); if (IsItemHovered()) tip("passively providing");
							}

							int lower = level.lower;
							int upper = level.upper;

							TableNextColumn();
							if (en.spec->storeSetLower) {
								SetNextItemWidth(width*0.2f);
								if (InputIntClamp(fmtc("##%d", id++), &lower, 0, limit, step, step*10)) {
									store.levelSet(level.iid, lower, upper);
								}
							}

							TableNextColumn();
							if (en.spec->storeSetUpper) {
								SetNextItemWidth(width*0.2f);
								if (InputIntClamp(fmtc("##%d", id++), &upper, 0, limit, step, step*10)) {
									store.levelSet(level.iid, lower, upper);
								}
							}

							TableNextColumn();
							if (Button(fmtc("%s##%d", ICON_FA_THERMOMETER_FULL, id++), ImVec2(-1,0))) {
								store.levelSet(level.iid, lower, limit);
							}
							if (IsItemHovered()) tip("maximum upper limit");

							TableNextColumn();
							if (Button(fmtc("%s##%d", ICON_FA_LEVEL_DOWN, id++), ImVec2(-1,0))) {
								down = level.iid;
							}
							if (IsItemHovered()) tip("move row to bottom");

							TableNextColumn();
							if (Button(fmtc("%s##%d", ICON_FA_TIMES, id++), ImVec2(-1,0))) {
								clear = level.iid;
							}
							if (IsItemHovered()) tip("remove row limits");
						}

						for (Stack stack: store.stacks) {
							if (store.level(stack.iid) != nullptr) continue;
							Item *item = Item::get(stack.iid);

							int limit = (uint)store.limit().value/item->mass.value;
							int step  = (int)std::max(1U, (uint)limit/100);

							TableNextRow();

							TableNextColumn();
							itemIcon(item);

							TableNextColumn();
							Print(fmtc("%s", item->title));

							TableNextColumn();
							Print(fmtc("%d", store.count(stack.iid)));

							TableNextColumn();
							TableNextColumn();

							TableNextColumn();
							if (Button(fmtc("+##%d", id++), ImVec2(-1,0))) {
								uint size = stack.size%step ? stack.size+step-(stack.size%step): stack.size;
								store.levelSet(stack.iid, 0, size);
							}
							if (IsItemHovered()) tip(
								"Limit item"
							);

							TableNextColumn();
							TableNextColumn();
							TableNextColumn();

							if (en.spec->crafter) {
								TableNextColumn();
							}
						}

						EndTable();
					}
					PopStyleVar();

					if (clear) {
						store.levelClear(clear);
					}

					if (down) {
						auto level = store.level(down);
						uint lower = level->lower;
						uint upper = level->upper;
						store.levelClear(down);
						store.levelSet(down, lower, upper);
					}

				PopID();

				EndTabItem();
			}

			if (en.spec->teleporter && en.spec->teleporterSend && BeginTabItem("Teleporter", nullptr, focusedTab())) {
				Teleporter& teleporter = en.teleporter();

				PushID("teleporter-send");

				if (Button("teleport now")) {
					teleporter.trigger = true;
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->launcher && BeginTabItem("Launcher", nullptr, focusedTab())) {
				Launcher& launcher = en.launcher();
				Store& store = en.store();

				PushID("launcher");

				auto fuelRequired = launcher.fuelRequired();
				auto fuelAccessable = launcher.fuelAccessable();

				SpacingV();
				launcherNotice(launcher);

				SmallBar(launcher.progress);
				if (IsItemHovered()) tip("Launch progress");

				auto colorStore = ImColorSRGB(0x999999ff);
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);

				for (auto& amount: fuelRequired) {
					SpacingV();
					auto need = Liquid(amount.size);
					auto have = Liquid(fuelAccessable.has(amount.fid) ? fuelAccessable[amount.fid].size: (uint)0);
					Title(Fluid::get(amount.fid)->title.c_str()); SameLine();
					PrintRight(fmtc("%s / %s", have.format(), need.format()));
					SmallBar(have.portion(need));
				}

				SpacingV();

				uint insert = 0;
				uint remove = 0;

				int i = 0;
				for (auto iid: launcher.cargo) {
					if (ButtonStrip(i++, fmtc(" %s ", Item::get(iid)->title))) remove = iid;
					if (IsItemHovered()) tip("Drop item type from payload");
				}

				auto pick = ButtonStrip(i++, launcher.cargo.size() ? " + Item ": " Set Payload ");
				if (IsItemHovered()) tip("Add an item type to payload.");
				if (itemPicker(pick)) insert = itemPicked;

				if (remove) launcher.cargo.erase(remove);
				if (insert) launcher.cargo.insert(insert);

				SpacingV();

				SmallBar(store.usage().portion(store.limit()));
				if (IsItemHovered()) tip("Loading progress");

				SpacingV();

				PopStyleColor(1);

				Section("Launch Rule");
				if (BeginCombo("Monitor", "Network")) {
					if (launcher.en->spec->networker && Selectable("Network", launcher.monitor == Launcher::Monitor::Network)) {
						launcher.monitor = Launcher::Monitor::Network;
					}
					EndCombo();
				}
				signalCondition(0, launcher.condition, true, [&]() { return launcher.checkCondition(); });

				bool ready = launcher.ready();
				if (Button(" Launch ") && ready) launcher.activate = true;
				if (IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !ready) tip("Not ready");

				PopID();

				EndTabItem();
			}

			if (en.spec->crafter && en.spec->crafterShowTab && BeginTabItem("Crafting", nullptr, focusedTab())) {
				Crafter& crafter = en.crafter();

				PushID("crafter");
				SpacingV();
				crafterNotice(crafter);

				SmallBar(crafter.progress);
				SpacingV();

				auto craftable = [&](Recipe* recipe) {
					return recipe->manufacturable() && crafter.craftable(recipe);
				};

				if (crafter.recipe) {
					auto pick = Button(fmtc(" %s ", crafter.recipe->title.c_str()));
					if (IsItemHovered()) tip("Change recipe");
					if (recipePicker(pick, craftable)) crafter.craft(recipePicked);
				}
				else {
					if (recipePicker(Button(" Set Recipe "), craftable)) {
						crafter.craft(recipePicked);
					}
				}

				if (crafter.recipe) {
					float width = GetContentRegionAvail().x;

					if (crafter.recipe->inputItems.size() || crafter.recipe->inputFluids.size()) {

						SpacingV();
						if (BeginTable("inputs", 4)) {
							TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
							TableSetupColumn("Resource", ImGuiTableColumnFlags_WidthStretch);
							TableSetupColumn("Consumes", ImGuiTableColumnFlags_WidthFixed, width*0.2f);
							TableSetupColumn("Contains", ImGuiTableColumnFlags_WidthFixed, width*0.2f);

							TableHeadersRow();

							auto atLeast = [&](int limit, int have, std::string s) {
								if (have < limit) {
									TextColored(0xff0000ff, s.c_str());
								}
								else {
									Print(s.c_str());
								}
								if (IsItemHovered()) tip(
									"Resource input buffers allow for 2x recipe consumption."
									" To avoid production stalling keep input buffers full."
								);
							};

							if (crafter.recipe->inputItems.size() && en.spec->store) {
								for (auto& stack: crafter.inputItemsState()) {
									TableNextRow();

									std::string& name = Item::get(stack.iid)->title;
									int consume = crafter.recipe->inputItems[stack.iid];
									int have = stack.size;

									TableNextColumn();
									itemIcon(Item::get(stack.iid));

									TableNextColumn();
									Print(name.c_str());

									TableNextColumn();
									Print(fmtc("%d", consume));

									TableNextColumn();
									atLeast(consume, have, fmt("%d", have));
								}
							}

							if (crafter.recipe->inputFluids.size()) {
								for (auto& amount: crafter.inputFluidsState()) {
									TableNextRow();

									std::string& name = Fluid::get(amount.fid)->title;
									int consume = crafter.recipe->inputFluids[amount.fid];
									int have = amount.size;

									TableNextColumn();
									fluidIcon(Fluid::get(amount.fid));

									TableNextColumn();
									Print(name.c_str());

									TableNextColumn();
									Print(Liquid(consume).format());

									TableNextColumn();
									atLeast(consume, have, Liquid(have).format());
								}
							}

							EndTable();
						}
					}

					if (crafter.recipe->outputItems.size() || crafter.recipe->outputFluids.size() || crafter.recipe->mine || crafter.recipe->drill) {

						SpacingV();
						if (BeginTable("outputs", 4)) {
							TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
							TableSetupColumn("Product", ImGuiTableColumnFlags_WidthStretch);
							TableSetupColumn("Produces", ImGuiTableColumnFlags_WidthFixed, width*0.2f);
							TableSetupColumn("Contains", ImGuiTableColumnFlags_WidthFixed, width*0.2f);

							TableHeadersRow();

							auto atMost = [&](int limit, int have, std::string s) {
								if (have && have >= limit) {
									TextColored(0xff0000ff, s.c_str());
								}
								else {
									Print(s.c_str());
								}
							};

							if (crafter.recipe->outputItems.size() && en.spec->store) {
								for (auto& stack: crafter.outputItemsState()) {
									TableNextRow();

									std::string& name = Item::get(stack.iid)->title;
									int produce = crafter.recipe->outputItems[stack.iid];
									int have = stack.size;

									TableNextColumn();
									itemIcon(Item::get(stack.iid));

									TableNextColumn();
									Print(name.c_str());

									TableNextColumn();
									Print(fmtc("%d", produce));

									TableNextColumn();
									atMost(produce+1, have, fmt("%d", have));
									if (IsItemHovered()) {
										tip("Product output buffers are capped at 2x production, and"
											" a run will not start if there is insufficient space."
											" To avoid production stalling keep output buffers empty."
										);
									}
								}
							}

							if (crafter.recipe->mine && en.spec->store) {
								TableNextRow();

								std::string& name = Item::get(crafter.recipe->mine)->title;
								int produce = 1;
								int have = en.store().count(crafter.recipe->mine);
								int limit = en.store().limit().items(crafter.recipe->mine);

								TableNextColumn();
								itemIcon(Item::get(crafter.recipe->mine));

								TableNextColumn();
								Print(name.c_str());

								TableNextColumn();
								Print(fmtc("%d", produce));

								TableNextColumn();
								atMost(limit, have, fmtc("%d", have));
								if (IsItemHovered() && crafter.recipe->mine) {
									tip(
										"Mining product output buffers are capped by the entity storage capacity."
									);
								}
							}

							if (crafter.recipe->outputFluids.size()) {
								for (auto& amount: crafter.outputFluidsState()) {
									TableNextRow();

									std::string& name = Fluid::get(amount.fid)->title;
									int produce = crafter.recipe->outputFluids[amount.fid];
									int have = amount.size;

									TableNextColumn();
									fluidIcon(Fluid::get(amount.fid));

									TableNextColumn();
									Print(name.c_str());

									TableNextColumn();
									Print(Liquid(produce).format().c_str());

									TableNextColumn();
									atMost(produce+1, have, Liquid(have).format());
									if (IsItemHovered()) {
										tip(
											"Fluid product buffers must be flushed to a pipe network before the next run starts."
										);
									}
								}
							}

							if (crafter.recipe->drill) {
								TableNextRow();

								std::string& name = Fluid::get(crafter.recipe->drill)->title;
								int produce = 100;
								int have = 0;

								for (auto& amount: crafter.exportFluids) {
									if (amount.fid == crafter.recipe->drill) have = amount.size;
								}

								TableNextColumn();
								fluidIcon(Fluid::get(crafter.recipe->drill));

								TableNextColumn();
								Print(name.c_str());

								TableNextColumn();
								Print(Liquid(produce).format().c_str());

								TableNextColumn();
								atMost(0, have, Liquid(have).format());
								if (IsItemHovered() && crafter.recipe->drill) {
									tip(
										"Drilling product buffers must be flushed to a pipe network before the next run starts."
									);
								}
							}

							EndTable();
						}
					}

					if (crafter.en->spec->crafterTransmitResources && crafter.recipe->mine) {
						Checkbox("Transmit hill ore count", &crafter.transmit);
					}

					if (crafter.en->spec->crafterTransmitResources && crafter.recipe->drill) {
						Checkbox("Transmit lake oil count", &crafter.transmit);
					}
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->vehicle && BeginTabItem("Vehicle", nullptr, focusedTab())) {
				Vehicle& vehicle = en.vehicle();

				PushID("vehicle");

				bool patrol = en.vehicle().patrol;
				if (Checkbox("patrol", &patrol)) {
					vehicle.patrol = patrol;
				}

				bool handbrake = vehicle.handbrake;
				if (Checkbox("handbrake", &handbrake)) {
					vehicle.handbrake = handbrake;
				}

				int n = 0;
				int thisWay = 0, dropWay = -1;

				for (auto waypoint: vehicle.waypoints) {
					if (waypoint->stopId && Entity::exists(waypoint->stopId)) {
						Print(Entity::get(waypoint->stopId).name().c_str());
					} else {
						Print(fmt("%f,%f,%f", waypoint->position.x, waypoint->position.y, waypoint->position.z));
					}

					if (waypoint == vehicle.waypoint) {
						SameLine();
						Print("(current)");
					}

					int thisCon = 0, dropCon = -1;
					for (auto condition: waypoint->conditions) {

						if_is<Vehicle::DepartInactivity>(condition, [&](auto con) {
							Print(fmt("inactivity %d", con->seconds));
							InputInt(fmtc("seconds##%d", n++), &con->seconds);
						});

						if_is<Vehicle::DepartItem>(condition, [&](auto con) {

							SetNextItemWidth(200);
							if (BeginCombo(fmtc("##%d", n++), (con->iid ? Item::get(con->iid)->title.c_str(): "item"), ImGuiComboFlags_None)) {
								for (auto& item: Item::all) {
									if (Selectable(fmtc("%s##%d", item.title, n++), con->iid == item.id)) {
										con->iid = item.id;
									}
								}
								EndCombo();
							}

							std::map<uint,std::string> ops = {
								{ Vehicle::DepartItem::Eq,  "==" },
								{ Vehicle::DepartItem::Ne,  "!=" },
								{ Vehicle::DepartItem::Lt,  "<" },
								{ Vehicle::DepartItem::Lte, "<=" },
								{ Vehicle::DepartItem::Gt,  ">" },
								{ Vehicle::DepartItem::Gte, ">=" },
							};

							SameLine();
							SetNextItemWidth(100);
							if (BeginCombo(fmtc("##%d", n++), ops[con->op].c_str(), ImGuiComboFlags_None)) {
								for (auto [op,opname]: ops) {
									if (Selectable(fmtc("%s##%d-%u", opname.c_str(), n++, op), con->op == op)) {
										con->op = op;
									}
								}
								EndCombo();
							}

							SameLine();
							SetNextItemWidth(200);
							InputInt(fmtc("##%d", n++), (int*)(&con->count));
						});

						SameLine();
						if (Button(fmtc("-con##%d", n++))) {
							dropCon = thisCon;
						}

						thisCon++;
					}

					if (dropCon >= 0) {
						auto it = waypoint->conditions.begin();
						std::advance(it, dropCon);
						delete *it;
						waypoint->conditions.erase(it);
					}

					if (Button(fmtc("+activity##%d", n++))) {
						waypoint->conditions.push_back(new Vehicle::DepartInactivity());
					}

					SameLine();

					if (Button(fmtc("+item##%d", n++))) {
						waypoint->conditions.push_back(new Vehicle::DepartItem());
					}

					if (Button(fmtc("-way##%d", n++))) {
						dropWay = thisWay;
					}

					thisWay++;
				}

				if (dropWay >= 0) {
					auto it = vehicle.waypoints.begin();
					std::advance(it, dropWay);
					delete *it;
					vehicle.waypoints.erase(it);
				}

				for (auto [eid,name]: Entity::names) {
					if (Button(fmtc("+way %s##%d", name.c_str(), n++))) {
						vehicle.addWaypoint(eid);
					}
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->cart && BeginTabItem("Vehicle", nullptr, focusedTab())) {
				Cart& cart = en.cart();

				PushID("cart");

				auto route = [&]() {
					if (cart.line == CartWaypoint::Red) return "Red";
					if (cart.line == CartWaypoint::Blue) return "Blue";
					if (cart.line == CartWaypoint::Green) return "Green";
					ensure(false); return "Red";
				};

				if (BeginCombo("Route", route())) {
					if (Selectable("Red", cart.line == CartWaypoint::Red)) {
						cart.line = CartWaypoint::Red;
					}
					if (Selectable("Blue", cart.line == CartWaypoint::Blue)) {
						cart.line = CartWaypoint::Blue;
					}
					if (Selectable("Green", cart.line == CartWaypoint::Green)) {
						cart.line = CartWaypoint::Green;
					}
					EndCombo();
				}

				signalConstant(0, cart.signal);

				PopID();

				EndTabItem();
			}

			if (en.spec->cartStop && BeginTabItem("Vehicle Stop", nullptr, focusedTab())) {
				CartStop& stop = en.cartStop();

				PushID("cart-stop");

				const char* selected = "Inactivity";
				if (stop.depart == CartStop::Depart::Empty) selected = "Vehicle Empty";
				if (stop.depart == CartStop::Depart::Full) selected = "Vehicle Full";

				if (BeginCombo("Departure condition", selected)) {
					if (Selectable("Inactivity", stop.depart == CartStop::Depart::Inactivity)) {
						stop.depart = CartStop::Depart::Inactivity;
					}
					if (Selectable("Vehicle Empty", stop.depart == CartStop::Depart::Empty)) {
						stop.depart = CartStop::Depart::Empty;
					}
					if (Selectable("Vehicle Full", stop.depart == CartStop::Depart::Full)) {
						stop.depart = CartStop::Depart::Full;
					}
					EndCombo();
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->cartWaypoint && BeginTabItem("Vehicle Waypoint", nullptr, focusedTab())) {
				CartWaypoint& waypoint = en.cartWaypoint();

				PushID("cart-waypoint");
				int down = -1;
				int clear = -1;

				SpacingV();
				Section("Vehicle Redirection Rules");

				auto route = [&](int line) {
					if (line == CartWaypoint::Red) return "Red";
					if (line == CartWaypoint::Blue) return "Blue";
					if (line == CartWaypoint::Green) return "Green";
					ensure(false); return "Red";
				};

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				for (int i = 0; i < (int)waypoint.redirections.size(); i++) {
					auto& redirection = waypoint.redirections[i];

					float space = GetContentRegionAvail().x;
					if (BeginTable(fmtc("##signal-cond-%d", i), 6)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.1);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.2);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.15);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);

						signalCondition(i, redirection.condition, false);

						TableNextColumn();
						SetNextItemWidth(-1);
						if (BeginCombo(fmtc("##route%d", i), route(redirection.line))) {
							if (Selectable("Red", redirection.line == CartWaypoint::Red)) {
								redirection.line = CartWaypoint::Red;
							}
							if (Selectable("Blue", redirection.line == CartWaypoint::Blue)) {
								redirection.line = CartWaypoint::Blue;
							}
							if (Selectable("Green", redirection.line == CartWaypoint::Green)) {
								redirection.line = CartWaypoint::Green;
							}
							EndCombo();
						}

						TableNextColumn();
						SetNextItemWidth(-1);
						if (Button(fmtc("%s##down%d", ICON_FA_LEVEL_DOWN, i), ImVec2(-1,0))) {
							down = i;
						}

						TableNextColumn();
						SetNextItemWidth(-1);
						if (Button(fmtc("%s##clear%d", ICON_FA_TRASH, i), ImVec2(-1,0))) {
							clear = i;
						}

						EndTable();
					}
				}

				PopStyleVar(1);

				if (down >= 0) {
					waypoint.redirections.push_back(waypoint.redirections[down]);
					waypoint.redirections.erase(down);
				}

				if (clear >= 0) {
					waypoint.redirections.erase(clear);
				}

				if (Button("Add rule")) {
					waypoint.redirections.push_back({});
					waypoint.redirections.back().condition.op = Signal::Operator::Gt;
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->monorailStop && BeginTabItem("Monorail Stop", nullptr, focusedTab())) {
				auto& monorail = en.monorail();

				PushID("monorail-stop");

				SpacingV();
				Section("Container Wait Rules");

				Checkbox("Wait for containers to fill before loading", &monorail.filling);
				Checkbox("Wait for containers to empty after unloading", &monorail.emptying);

				BeginDisabled(!en.spec->networker);
				Checkbox("Transmit container contents", &monorail.transmit.contents);
				EndDisabled();

				PopID();

				EndTabItem();
			}

			if (en.spec->monorail && BeginTabItem("Monorail Waypoint", nullptr, focusedTab())) {
				Monorail& waypoint = en.monorail();

				PushID("monorail-waypoint");
				int down = -1;
				int clear = -1;

				SpacingV();
				Section("Monorail Car Redirection Rules");

				auto route = [&](int line) {
					if (line == Monorail::Red) return "Red";
					if (line == Monorail::Blue) return "Blue";
					if (line == Monorail::Green) return "Green";
					ensure(false); return "Red";
				};

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				for (int i = 0; i < (int)waypoint.redirections.size(); i++) {
					auto& redirection = waypoint.redirections[i];

					float space = GetContentRegionAvail().x;
					if (BeginTable(fmtc("##signal-cond-%d", i), 6)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.1);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.2);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.15);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);

						signalCondition(i, redirection.condition, false);

						TableNextColumn();
						SetNextItemWidth(-1);
						if (BeginCombo(fmtc("##route%d", i), route(redirection.line))) {
							if (Selectable("Red", redirection.line == Monorail::Red)) {
								redirection.line = Monorail::Red;
							}
							if (Selectable("Blue", redirection.line == Monorail::Blue)) {
								redirection.line = Monorail::Blue;
							}
							if (Selectable("Green", redirection.line == Monorail::Green)) {
								redirection.line = Monorail::Green;
							}
							EndCombo();
						}

						TableNextColumn();
						SetNextItemWidth(-1);
						if (Button(fmtc("%s##down%d", ICON_FA_LEVEL_DOWN, i), ImVec2(-1,0))) {
							down = i;
						}

						TableNextColumn();
						SetNextItemWidth(-1);
						if (Button(fmtc("%s##clear%d", ICON_FA_TRASH, i), ImVec2(-1,0))) {
							clear = i;
						}

						EndTable();
					}
				}

				PopStyleVar(1);

				if (down >= 0) {
					waypoint.redirections.push_back(waypoint.redirections[down]);
					waypoint.redirections.erase(down);
				}

				if (clear >= 0) {
					waypoint.redirections.erase(clear);
				}

				if (Button("Add rule")) {
					waypoint.redirections.push_back({});
					waypoint.redirections.back().condition.op = Signal::Operator::Gt;
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->monocar && BeginTabItem("Monocar", nullptr, focusedTab())) {
				Monocar& monocar = en.monocar();

				PushID("monocar");

				auto route = [&]() {
					if (monocar.line == Monorail::Red) return "Red";
					if (monocar.line == Monorail::Blue) return "Blue";
					if (monocar.line == Monorail::Green) return "Green";
					ensure(false); return "Red";
				};

				if (BeginCombo("Route", route())) {
					if (Selectable("Red", monocar.line == Monorail::Red)) {
						monocar.line = Monorail::Red;
					}
					if (Selectable("Blue", monocar.line == Monorail::Blue)) {
						monocar.line = Monorail::Blue;
					}
					if (Selectable("Green", monocar.line == Monorail::Green)) {
						monocar.line = Monorail::Green;
					}
					EndCombo();
				}

				if (IsWindowAppearing()) checks.clear();
				checks.resize(monocar.constants.size());

				if (BeginTable("##constant-signals", 2)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

					int i = 0;
					for (auto& signal: monocar.constants) {
						TableNextRow();
						TableNextColumn();
						Checkbox(fmtc("##check%d", i), &checks[i]);
						TableNextColumn();
						signalConstant(i, signal, false);
						i++;
					}

					EndTable();
				}

				if (Button("Add Signal##add-signal")) {
					monocar.constants.resize(monocar.constants.size()+1);
					monocar.constants.back().value = 1;
				}

				bool checked = false;
				for (auto& check: checks) checked = checked || check;

				if (checked && Button("Drop Selected##drop-signals")) {
					for (int i = checks.size()-1; i >= 0; --i) {
						if (checks[i]) monocar.constants.erase(i);
					}
					checks.clear();
				}

				SpacingV();

				switch (monocar.state) {
					case Monocar::State::Start:
					case Monocar::State::Acquire:
					case Monocar::State::Travel:
						PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
						Button("Depart");
						PopStyleColor(1);
						break;
					case Monocar::State::Stop:
					case Monocar::State::Unload:
					case Monocar::State::Unloading:
					case Monocar::State::Load:
					case Monocar::State::Loading:
						if (Button("Depart"))
							monocar.flags.move = true;
						break;
				}

				if (IsItemHovered()) tip(
					"Car will depart a monorail stop as soon as possible."
				);

				SpacingV();

				Checkbox("Hand Brake", &monocar.flags.stop);
				if (IsItemHovered()) tip(
					"Car will stop immediately but continue to hold a reservation"
					" for the next tower, which may block other cars."
				);

				PopID();

				EndTabItem();
			}

			if (en.spec->computer && BeginTabItem("Computer", nullptr, focusedTab())) {
				auto& computer = en.computer();

				PushID("computer");

				if (BeginTable("##toggles", 3)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.33);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.33);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.34);

					TableNextColumn();
					bool enabled = en.isEnabled();
					if (Checkbox("Power On", &enabled)) {
						en.setEnabled(enabled);
						computer.reboot();
					}

					TableNextColumn();
					if (Checkbox("Debug Mode", &computer.debug)) {
						computer.reboot();
					}

					TableNextColumn();
					switch (computer.err) {
						case Computer::Ok: {
							Print("System Ready");
							break;
						}
						case Computer::What: {
							Print("Report a Bug!");
							if (IsItemHovered()) tip(
								"The entity is in a state that should be impossible."
								" Please report a bug in the game and attach your program!"
							);
							break;
						}
						case Computer::StackUnderflow: {
							Print("Stack Underflow");
							break;
						}
						case Computer::StackOverflow: {
							Print("Stack Overflow");
							break;
						}
						case Computer::OutOfBounds: {
							Print("Out of Bounds");
							break;
						}
						case Computer::Syntax: {
							Print("Syntax Error");
							break;
						}
					}
					EndTable();
				}

				if (BeginCombo("##source", nullptr)) {
					for (const auto& file: std::filesystem::directory_iterator("programs/")) {
						if (Selectable((const char*)(file.path().c_str()), false)) {
							auto slurp = std::ifstream(file.path());
							computer.source = std::string(
								(std::istreambuf_iterator<char>(slurp)),
								(std::istreambuf_iterator<char>())
							);
						}
					}
					EndCombo();
				}
				SameLine();
				if (Button("Load Program")) {
					computer.reboot();
				}

				{
					Section("Environment Variables");
					if (IsItemHovered()) {
						tip(
							"Environment signals key/value pairs are written "
							"to RAM each tick starting at address 00."
						);
					}
					uint env = 0;
					std::vector<Signal::Key> drop;

					if (BeginTable("##environment", 2)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.6);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

						for (auto& signal: computer.env) {
							TableNextRow();
							TableNextColumn();
							signalConstant(env, signal, false);
							TableNextColumn();
							if (Button(fmtc("Remove##forget-%u", env))) {
								drop.push_back(signal.key);
							}
							if (IsItemHovered()) {
								tip("Remove this environment signal");
							}
							env++;
						}
						TableNextRow();
						TableNextColumn();
						signalConstant(env, computer.add, false);
						TableNextColumn();
						if (Button("Create##define-env")) {
							computer.env[computer.add.key].value = computer.add.value;
							computer.add.key.type = Signal::Type::Stack;
							computer.add.key.id = 0;
							computer.add.value = 0;
						}
						if (IsItemHovered()) {
							tip("Add or overwrite an environment signal");
						}

						EndTable();
					}
					for (auto& key: drop) {
						computer.env.erase(key);
					}
				}

				auto memory = [&](int id, minivec<int>& mem) {
					if (BeginTable(fmtc("##memory%d", id), 9)) {
						for (uint i = 0; i < mem.size(); ) {
							TableNextRow();
							Print(fmtc("%03d", i));

							for (uint j = 0; j < 8 && i < mem.size(); j++, i++) {
								TableNextColumn();
								Print(fmt("%d", mem[i]));
							}
						}
						EndTable();
					}
				};

				Section("RAM");
				memory(1, computer.ram);
				if (IsItemHovered()) {
					tip(
						"RAM is read/write memory for use by the program."
					);
				}

				Section("ROM");
				memory(2, computer.rom);
				if (IsItemHovered()) {
					tip(
						"ROM is read-only memory used by the compiler to store static "
						"stuff like environment variables and string literals."
					);
				}

				Section(fmtc("Program: %u cycles", (uint)computer.code.size()));
				Print(computer.log.size() ? computer.log.c_str(): "(no output)");

				NewLine();

				for (uint i = 0; i < computer.code.size(); i++) {
					auto& instruction = computer.code[i];

					switch (instruction.opcode) {
						case Computer::Nop: {
							Print(fmtc("Nop, %d", instruction.value));
							break;
						}
						case Computer::Rom: {
							Print(fmtc("Rom, %d", instruction.value));
							break;
						}
						case Computer::Ram: {
							Print(fmtc("Ram, %d", instruction.value));
							break;
						}
						case Computer::Jmp: {
							Print(fmtc("Jmp, %d", instruction.value));
							break;
						}
						case Computer::Jz: {
							Print(fmtc("Jz, %d", instruction.value));
							break;
						}
						case Computer::Call: {
							Print(fmtc("Call, %d", instruction.value));
							break;
						}
						case Computer::Ret: {
							Print(fmtc("Ret, %d", instruction.value));
							break;
						}
						case Computer::Push: {
							Print(fmtc("Push, %d", instruction.value));
							break;
						}
						case Computer::Pop: {
							Print(fmtc("Pop, %d", instruction.value));
							break;
						}
						case Computer::Dup: {
							Print(fmtc("Dup, %d", instruction.value));
							break;
						}
						case Computer::Drop: {
							Print(fmtc("Drop, %d", instruction.value));
							break;
						}
						case Computer::Over: {
							Print(fmtc("Over, %d", instruction.value));
							break;
						}
						case Computer::Swap: {
							Print(fmtc("Swap, %d", instruction.value));
							break;
						}
						case Computer::Fetch: {
							Print(fmtc("Fetch, %d", instruction.value));
							break;
						}
						case Computer::Store: {
							Print(fmtc("Store, %d", instruction.value));
							break;
						}
						case Computer::Lit: {
							Print(fmtc("Lit, %d", instruction.value));
							break;
						}
						case Computer::Add: {
							Print(fmtc("Add, %d", instruction.value));
							break;
						}
						case Computer::Sub: {
							Print(fmtc("Sub, %d", instruction.value));
							break;
						}
						case Computer::Mul: {
							Print(fmtc("Mul, %d", instruction.value));
							break;
						}
						case Computer::Div: {
							Print(fmtc("Div, %d", instruction.value));
							break;
						}
						case Computer::Mod: {
							Print(fmtc("Mod, %d", instruction.value));
							break;
						}
						case Computer::And: {
							Print(fmtc("And, %d", instruction.value));
							break;
						}
						case Computer::Or: {
							Print(fmtc("Or, %d", instruction.value));
							break;
						}
						case Computer::Xor: {
							Print(fmtc("Xor, %d", instruction.value));
							break;
						}
						case Computer::Inv: {
							Print(fmtc("Inv, %d", instruction.value));
							break;
						}
						case Computer::Eq: {
							Print(fmtc("Eq, %d", instruction.value));
							break;
						}
						case Computer::Ne: {
							Print(fmtc("Ne, %d", instruction.value));
							break;
						}
						case Computer::Lt: {
							Print(fmtc("Lt, %d", instruction.value));
							break;
						}
						case Computer::Lte: {
							Print(fmtc("Lte, %d", instruction.value));
							break;
						}
						case Computer::Gt: {
							Print(fmtc("Gt, %d", instruction.value));
							break;
						}
						case Computer::Gte: {
							Print(fmtc("Gte, %d", instruction.value));
							break;
						}
						case Computer::Min: {
							Print(fmtc("Min, %d", instruction.value));
							break;
						}
						case Computer::Max: {
							Print(fmtc("Max, %d", instruction.value));
							break;
						}
						case Computer::Print: {
							Print(fmtc("Print, %d", instruction.value));
							break;
						}
						case Computer::DumpTop: {
							Print(fmtc("DumpTop, %d", instruction.value));
							break;
						}
						case Computer::DumpStack: {
							Print(fmtc("DumpStack, %d", instruction.value));
							break;
						}
						case Computer::Send: {
							Print(fmtc("Send, %d", instruction.value));
							break;
						}
						case Computer::Recv: {
							Print(fmtc("Recv, %d", instruction.value));
							break;
						}
						case Computer::Sniff: {
							Print(fmtc("Sniff, %d", instruction.value));
							break;
						}
						case Computer::Now: {
							Print(fmtc("Now, %d", instruction.value));
							break;
						}
					}

					if (computer.ip == (int)i) {
						SameLine();
						Print("<--ip");
					}
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->router && BeginTabItem("Wifi Router", nullptr, focusedTab())) {
				auto& router = en.router();
				auto& networker = en.networker();

				PushID("router");

				int i = 0;
				int clear = -1;

				auto wifiConnection = [&](int id, int* nic) {
					std::string title = networker.interfaces[*nic].network
						? networker.interfaces[*nic].network->ssid: "Wifi Interface 1";
					if (BeginCombo(fmtc("##wifi-connection-%d", id), title.c_str())) {
						for (int i = 0, l = networker.interfaces.size(); i < l; i++) {
							auto& interface = networker.interfaces[i];
							std::string title = interface.network ? interface.network->ssid: fmt("Wifi Interface %d", i+1);
							if (Selectable(title.c_str(), *nic == i)) *nic = i;
						}
						EndCombo();
					}
				};

				float space = GetContentRegionAvail().x;
				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				int n = 0;
				for (auto& rule: router.rules) {

					auto check = [&]() {
						return rule.condition.evaluate(networker.interfaces[rule.nicSrc].network
							? networker.interfaces[rule.nicSrc].network->signals : Signal::NoSignals);
					};

					auto title = fmt("Rule %d", ++n);

					SpacingV();
					if (rule.mode == Router::Rule::Mode::Alert && rule.alert == Router::Rule::Alert::Warning && check()) {
						Warning(title);
					}
					else
					if (rule.mode == Router::Rule::Mode::Alert && rule.alert == Router::Rule::Alert::Critical && check()) {
						Alert(title);
					}
					else {
						Section(title);
					}

					if (BeginTable(fmtc("##table%d", ++i), 5)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.1);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.2);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.225);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.05);

						signalCondition(++i, rule.condition, false, check);

						TableNextColumn();
						if (Button(fmtc("%s##clear%d", ICON_FA_TRASH), ImVec2(-1,0))) {
							clear = &rule - router.rules.data();
						}
						EndTable();
					}
					SpacingV();

					if (BeginTable(fmtc("##table%d", ++i), 2)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, space*0.63);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

						const char* selectedMode = nullptr;
						switch (rule.mode) {
							case Router::Rule::Mode::Forward: selectedMode = "Forward signal"; break;
							case Router::Rule::Mode::Generate: selectedMode = "Generate signal"; break;
							case Router::Rule::Mode::Alert: selectedMode = "Raise alert"; break;
						}
						TableNextColumn();
						PushItemWidth(-1);
						if (BeginCombo(fmtc("##rule%d", ++i), selectedMode)) {
							if (Selectable("Forward signal", rule.mode == Router::Rule::Mode::Forward)) rule.mode = Router::Rule::Mode::Forward;
							if (Selectable("Generate signal", rule.mode == Router::Rule::Mode::Generate)) rule.mode = Router::Rule::Mode::Generate;
							if (Selectable("Raise alert", rule.mode == Router::Rule::Mode::Alert)) rule.mode = Router::Rule::Mode::Alert;
							EndCombo();
						}
						PopItemWidth();
						SpacingV();
						TableNextColumn();
						Print("Action when true");

						if (rule.condition.key != Signal::Key(Signal::Meta::Now)) {
							TableNextColumn();
							PushItemWidth(-1);
							wifiConnection(++i, &rule.nicSrc);
							PopItemWidth();
							SpacingV();
							TableNextColumn();
							Print("Input network");
						}

						switch (rule.mode) {
							case Router::Rule::Mode::Forward: {
								TableNextColumn();
								PushItemWidth(-1);
								wifiConnection(++i, &rule.nicDst);
								PopItemWidth();
								SpacingV();
								TableNextColumn();
								Print("Output network");
								break;
							}
							case Router::Rule::Mode::Generate: {
								TableNextColumn();
								PushItemWidth(-1);
								wifiConnection(++i, &rule.nicDst);
								PopItemWidth();
								SpacingV();
								TableNextColumn();
								Print("Output network");

								TableNextColumn();
								PushItemWidth(space*0.397);
								signalKey(++i, rule.signal.key, false);
								PopItemWidth();
								SetNextItemWidth(-1);
								SameLine();
								InputInt(fmtc("##int%d", ++i), &rule.signal.value);
								PopItemWidth();
								SpacingV();
								TableNextColumn();
								Print("Output signal");
								break;
							}
							case Router::Rule::Mode::Alert: {
								const char* selectedAlert = nullptr;
								switch (rule.alert) {
									case Router::Rule::Alert::Notice: selectedAlert = "Notice"; break;
									case Router::Rule::Alert::Warning: selectedAlert = "Warning"; break;
									case Router::Rule::Alert::Critical: selectedAlert = "Critical"; break;
								}
								TableNextColumn();
								PushItemWidth(-1);
								if (BeginCombo(fmtc("##alert-level-%d", ++i), selectedAlert)) {
									if (Selectable("Notice", rule.alert == Router::Rule::Alert::Notice)) rule.alert = Router::Rule::Alert::Notice;
									if (Selectable("Warning", rule.alert == Router::Rule::Alert::Warning)) rule.alert = Router::Rule::Alert::Warning;
									if (Selectable("Critical", rule.alert == Router::Rule::Alert::Critical)) rule.alert = Router::Rule::Alert::Critical;
									EndCombo();
								}
								PopItemWidth();
								SpacingV();
								TableNextColumn();
								Print("Alert level");

								TableNextColumn();
								int r = i;
								float s = GetFontSize() * 1.5;
								auto size = ImVec2(s, s);
								for (int i = 1, l = Sim::customIcons.size(); i < l; i++) {
									auto& icon = Sim::customIcons[i];
									bool active = i == rule.icon;
									if (GetContentRegionAvail().x < size.x) { NewLine(); SpacingV(); }
									if (active) PushStyleColor(ImGuiCol_Button, Color(0x009900ff));
									if (Button(fmtc("%s##icon-%d-%d", icon, r, i), size)) rule.icon = i;
									if (active) PopStyleColor(1);
									SameLine();
								}
								NewLine();
								TableNextColumn();
								Print("Alert icon");
								break;
							}
						}
						EndTable();
					}
				}

				if (clear >= 0) {
					router.rules.erase(clear);
				}

				Separator();
				SpacingV();

				if (Button("Add Rule##add")) {
					router.rules.resize(router.rules.size()+1);
				}

				PopStyleVar();
				PopID();

				EndTabItem();
			}

			if (en.spec->depot && BeginTabItem("Drone Port", nullptr, focusedTab())) {
				auto& depot = en.depot();

				PushID("depot");

				Checkbox("Construction", &depot.construction);
				if (IsItemHovered()) tip("Drone Port will dispatch drones to construct ghosts.");

				Checkbox("Deconstruction", &depot.deconstruction);
				if (IsItemHovered()) tip("Drone Port will dispatch drones to deconstruct ghosts.");

				if (en.spec->depotAssist && en.spec->networker) {
					Checkbox("Wifi Network", &depot.network);
					if (IsItemHovered()) tip("Drone Port will dispatch drones to networked containers.");
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->balancer && BeginTabItem("Balancer", nullptr, focusedTab())) {
				auto& balancer = en.balancer();

				PushID("balancer");

				SpacingV();
				Section("Priority");
				Checkbox("Input", &balancer.priority.input);
				Checkbox("Output", &balancer.priority.output);

				SpacingV();
				Section("Item Filtering");

				uint remove = 0;
				uint insert = 0;

				int i = 0;
				for (auto iid: balancer.filter) {
					if (ButtonStrip(i++, fmtc(" %s ", Item::get(iid)->title))) remove = iid;
					if (IsItemHovered()) tip("Remove item from filter set.");
				}

				auto pick = ButtonStrip(i++, balancer.filter.size() ? " + Item ": " Set Filter ");
				if (IsItemHovered()) tip("Add item to filter set.");
				if (itemPicker(pick)) insert = itemPicked;

				if (remove) balancer.filter.erase(remove);
				if (insert) balancer.filter.insert(insert);

				PopID();

				EndTabItem();
			}

			if (en.spec->tube && BeginTabItem("Tube", nullptr, focusedTab())) {
				auto& tube = en.tube();

				PushID("tube");

				SpacingV();
				Section("Input / Output Modes");
				auto mode = [&](Tube::Mode m) {
					switch (m) {
						case Tube::Mode::BeltOrTube: return "belt-or-tube";
						case Tube::Mode::BeltOnly: return "belt-only";
						case Tube::Mode::TubeOnly: return "tube-only";
						case Tube::Mode::BeltPriority: return "belt-priority";
						case Tube::Mode::TubePriority: return "tube-priority";
					}
					return "belt-or-tube";
				};

				if (BeginCombo("Input", mode(tube.input))) {
					for (auto m: std::vector<Tube::Mode>{
						Tube::Mode::BeltOrTube,
						Tube::Mode::BeltOnly,
						Tube::Mode::TubeOnly,
						Tube::Mode::BeltPriority,
						Tube::Mode::TubePriority,
					}) {
						if (Selectable(mode(m), tube.input == m)) {
							tube.input = m;
						}
					}
					EndCombo();
				}

				if (BeginCombo("Output", mode(tube.output))) {
					for (auto m: std::vector<Tube::Mode>{
						Tube::Mode::BeltOrTube,
						Tube::Mode::BeltOnly,
						Tube::Mode::TubeOnly,
						Tube::Mode::BeltPriority,
						Tube::Mode::TubePriority,
					}) {
						if (Selectable(mode(m), tube.output == m)) {
							tube.output = m;
						}
					}
					EndCombo();
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->networker && BeginTabItem("Wifi Network", nullptr, focusedTab())) {
				auto& networker = en.networker();

				PushID("networker");

				for (uint i = 0, l = std::min((uint)networker.interfaces.size(), 4u); i < l; i++) {
					auto& interface = networker.interfaces[i];

					SpacingV();
					Section(fmtc("Wifi Interface %u", i+1));
					PushID(fmtc("networker.interface%d", i));

					if (!interface.network && interface.ssid.size()) {
						Alert(fmt("Network %s is not in range.", interface.ssid));
					}

					if (interface.network) {
						Notice(fmt("Connected to \"%s\".", interface.ssid));
					}

					if (networker.isHub() && !interface.network) {
						bool saveSSID = InputTextWithHint(fmtc("##%u", i), "<new network name>", interfaces[i], sizeof(interfaces[i]), ImGuiInputTextFlags_EnterReturnsTrue);
						inputFocused = IsItemActive();
						SameLine();
						if (Button(fmtc("Create##%u", i)) || saveSSID) {
							interface.ssid = std::string(interfaces[i]);
							std::snprintf(interfaces[i], sizeof(interfaces[i]), "%s", interface.ssid.c_str());
							Networker::rebuild = true;
						}
						SpacingV();
					}

					if (BeginCombo("##networks", interface.ssid.c_str())) {
						for (auto ssid: networker.ssids()) {
							if (Selectable(ssid.c_str(), interface.ssid == ssid)) {
								interface.ssid = ssid;
								std::snprintf(interfaces[i], sizeof(interfaces[i]), "%s", interface.ssid.c_str());
								interfaces[i][0] = 0;
								Networker::rebuild = true;
							}
						}
						EndCombo();
					}

					if (!interface.network) {
						SameLine();
						Print("All networks");
					}

					if (interface.network) {
						SameLine();
						if (Button("Disconnect")) {
							interface.ssid = "";
							interfaces[i][0] = 0;
							Networker::rebuild = true;
						}
					}

					BeginChild("##signals", ImVec2(-1,300));

						if (operableStore) {
							Checkbox("Transmit items in storage", &en.store().transmit);
						}

						if (interface.network) {
							Print("Network signals");
							for (auto signal: interface.network->signals) {
								Print(std::string(signal).c_str());
							}
						}
						else {
							Print("Local signals");
							for (auto signal: interface.signals) {
								Print(std::string(signal).c_str());
							}
						}

					EndChild();

					PopID();
					Separator();
				}

				PopID();

				EndTabItem();
			}

			if (en.spec->powerpole && BeginTabItem("Electricity", nullptr, focusedTab())) {
				auto& powerpole = en.powerpole();

				PushID("powerpole");

				SpacingV();
				powerpoleNotice(powerpole);

				if (powerpole.network) {
					auto network = powerpole.network;

					auto capacity = network->capacityReady;
					SpacingV();
					Print(fmtc("Electricity Network #%u", network->id)); SameLine();
					PrintRight(fmtc("%s / %s", network->demand.formatRate(), capacity.formatRate()));
					OverflowBar(network->load, ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));
					OverflowBar(network->bufferedLevel.portion(network->bufferedLimit), ImColorSRGB(0xffff00ff), ImColorSRGB(0x9999ddff));

//					SpacingV();
//					OverflowBar(network->satisfaction, ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));
//					OverflowBar(network->charge, ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));
//					OverflowBar(network->discharge, ImColorSRGB(0x00aa00ff), ImColorSRGB(0xff0000ff));

					struct Stat {
						Spec* spec = nullptr;
						Energy energy = 0;
					};

					minivec<Stat> production;
					minivec<Stat> consumption;

					for (auto& [spec,series]: network->production) {
						auto energy = Energy(series.ticks[series.tick(Sim::tick-1)]);
						if (energy) production.push_back({spec,energy});
					}

					for (auto& [spec,series]: network->consumption) {
						auto energy = Energy(series.ticks[series.tick(Sim::tick-1)]);
						if (energy) consumption.push_back({spec,energy});
					}

					std::sort(production.begin(), production.end(), [&](const auto& a, const auto& b) {
						return a.energy > b.energy;
					});

					std::sort(consumption.begin(), consumption.end(), [&](const auto& a, const auto& b) {
						return a.energy > b.energy;
					});

					SpacingV();
					SpacingV();
					if (BeginTable("list", 2)) {
						float column = GetContentRegionAvail().x/2;
						TableSetupColumn("Consumers", ImGuiTableColumnFlags_WidthFixed, column);
						TableSetupColumn("Producers", ImGuiTableColumnFlags_WidthFixed, column);
						TableHeadersRow();

						TableNextColumn();
						SpacingV();
						for (auto& [spec,energy]: consumption) {
							Print(spec->title.c_str());
							SameLine(); PrintRight(fmtc("%s", energy.formatRate()));
							PushStyleColor(ImGuiCol_PlotHistogram, Color(0xaaaaaaff));
							SmallBar(energy.portion(network->demand));
							PopStyleColor(1);
							SpacingV();
						}

						TableNextColumn();
						SpacingV();
						for (auto& [spec,energy]: production) {
							Print(spec->title.c_str());
							SameLine(); PrintRight(fmtc("%s", energy.formatRate()));
							PushStyleColor(ImGuiCol_PlotHistogram, Color(0xaaaaaaff));
							SmallBar(energy.portion(network->supply));
							PopStyleColor(1);
							SpacingV();
						}

						EndTable();
					}
				}

				PopID();
				EndTabItem();
			}

			if (BeginTabItem("Settings", nullptr, focusedTab())) {

				PushID("entity");
					{
						bool permanent = en.isPermanent();
						if (Checkbox("Permanent", &permanent)) en.setPermanent(permanent);
						if (IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) tip(
							"Permanent structures cannot be accidentally deconstructed."
							"\n\nWARNING -- Removing permanent structures that have not"
							" yet been unlocked may make it difficult to continue!"
						);
					}
					if (en.spec->enable) {
						bool enabled = en.isEnabled();
						if (Checkbox("Enabled", &enabled)) en.setEnabled(enabled);
					}
					if (en.spec->generateElectricity) {
						bool generating = en.isGenerating();
						if (Checkbox("Generate Electricity", &generating)) en.setGenerating(generating);
					}
					if (en.spec->named) {
						bool saveEnter = InputText("Name", name, sizeof(name), ImGuiInputTextFlags_EnterReturnsTrue);
						inputFocused = IsItemActive();
						SameLine();
						if (Button(" Save ") || saveEnter) {
							if (std::strlen(name)) en.rename(name);
						}
					}
					if (en.spec->consumeFuel) {
						Burner& burner = en.burner();
						Print("Fuel");
						LevelBar(burner.energy.portion(burner.buffer));
						for (Stack stack: burner.store.stacks) {
							Item* item = Item::get(stack.iid);
							Print(fmtc("%s(%u)", item->title, stack.size));
						}
					}
				PopID();

				EndTabItem();
			}

			EndTabBar();
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

RecipePopup::RecipePopup() : Popup() {
}

RecipePopup::~RecipePopup() {
}

void RecipePopup::prepare() {
	sorted.items.clear();
	sorted.fluids.clear();
	sorted.recipes.clear();
	sorted.specs.clear();

	for (auto& [_,item]: Item::names)
		sorted.items.push_back(item);
	for (auto& [_,fluid]: Fluid::names)
		sorted.fluids.push_back(fluid);
	for (auto& [_,recipe]: Recipe::names)
		sorted.recipes.push_back(recipe);
	for (auto& [_,spec]: Spec::all)
		if (spec->build) sorted.specs.push_back(spec);
	for (auto& [_,goal]: Goal::all)
		sorted.goals.push_back(goal);

	std::sort(sorted.items.begin(), sorted.items.end(), Item::sort);
	std::sort(sorted.fluids.begin(), sorted.fluids.end(), Fluid::sort);
	reorder(sorted.recipes, [&](auto a, auto b) { return a->title < b->title; });
	reorder(sorted.specs, [&](auto a, auto b) { return a->title < b->title; });
	reorder(sorted.goals, [&](auto a, auto b) { return b->depends(a); });
}

void RecipePopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		big();
		Begin("Build##recipe", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

		if (IsWindowAppearing()) {
			expanded.goal.clear();
			if (Goal::current()) expanded.goal[Goal::current()] = true;
		}

		float column = GetContentRegionAvail().x/4.0;

		PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));
		if (BeginTable("list", 4, ImGuiTableFlags_BordersInnerV)) {
			TableSetupColumn("##items", ImGuiTableColumnFlags_WidthFixed, column);
			TableSetupColumn("##recipes", ImGuiTableColumnFlags_WidthFixed, column);
			TableSetupColumn("##specs", ImGuiTableColumnFlags_WidthFixed, column);
			TableSetupColumn("##goals", ImGuiTableColumnFlags_WidthFixed, column);

			TableNextColumn();

				Header("Items & Fluids", false);
				if (SmallButtonInlineRight(fmtc(" %s ##%s", ICON_FA_LOCK, "toggle-items-fluids"))) showUnavailableItemsFluids = !showUnavailableItemsFluids;
				if (IsItemHovered()) tip("Toggle locked items & fluids");

				BeginChild("list-items-and-fluids"); {
					for (auto category: Item::display) {
						bool show = false;
						for (auto group: category->display) {
							for (auto item: group->display) show = show || showItem(item);
						}
						if (show) {
							Section(category->title, false);
							for (auto group: category->display) {
								for (auto item: group->display) drawItem(item);
							}
						}
					}
					bool show = false;
					for (auto& fluid: sorted.fluids) {
						show = show || showFluid(fluid);
					}
					if (show) {
						Section("Fluids", false);
						for (auto& fluid: sorted.fluids) drawFluid(fluid);
					}
				}
				EndChild();

			TableNextColumn();

				Header("Recipes", false);
				if (SmallButtonInlineRight(fmtc(" %s ##%s", ICON_FA_LOCK, "toggle-recipes"))) showUnavailableRecipes = !showUnavailableRecipes;
				if (IsItemHovered()) tip("Toggle locked recipes");

				BeginChild("list-recipes");
				for (auto& recipe: sorted.recipes) drawRecipe(recipe);
				EndChild();

			TableNextColumn();

				Header("Structures & Vehicles", false);
				if (SmallButtonInlineRight(fmtc(" %s ##%s", ICON_FA_LOCK, "toggle-specs"))) showUnavailableSpecs = !showUnavailableSpecs;
				if (IsItemHovered()) tip("Toggle locked structures & vehicles");

				BeginChild("list-specs");
				for (auto& spec: sorted.specs) drawSpec(spec);
				EndChild();

			TableNextColumn();

				Header("Goals", false);
				BeginChild("list-goals");
				for (auto& goal: sorted.goals) drawGoal(goal);
				EndChild();

			EndTable();
		}
		PopStyleVar(1);

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

bool RecipePopup::showItem(Item* item) {
	return showUnavailableItemsFluids || item->manufacturable() || item->show || expanded.item[item];
}

void RecipePopup::drawItem(Item* item) {
	if (!showItem(item)) return;

	if (locate.item == item) {
		SetScrollHereY();
		locate.item = nullptr;
	}

	int pop = 0;

	if (highlited.item[item]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if (!item->manufacturable() && !highlited.item[item]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.item[item]);
	PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.item[item] = CollapsingHeader(fmtc("##item-%d", item->id));
	bool faint = !highlited.item[item] && !IsItemHovered();
	PopStyleColor(1);

	SameLine();
	itemIcon(item);

	SameLine();
	Print(item->title.c_str());

	SameLine();
	if (faint) PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	PushFont(Config::sidebar.font.imgui);
	if (item->fuel.energy) {
		PrintRight(fmtc("%s, %s", item->fuel.energy.format(), item->mass.format()), true);
	}
	else {
		PrintRight(item->mass.format().c_str(), true);
	}
	PopFont();
	if (faint) PopStyleColor(1);

	if (expanded.item[item]) {

		miniset<Recipe*> producers;
		for (auto& [_,recipe]: Recipe::names) {
			if (recipe->mine == item->id) {
				producers.insert(recipe);
			}
			if (recipe->outputItems.count(item->id)) {
				producers.insert(recipe);
			}
		}
		if (producers.size()) {
			Section("Produced by");
			for (auto& recipe: producers) drawRecipeButton(recipe);
			SpacingV();
		}

		miniset<Recipe*> recipeConsumers;
		for (auto& [_,recipe]: Recipe::names) {
			if (recipe->inputItems.count(item->id)) {
				recipeConsumers.insert(recipe);
			}
		}

		miniset<Spec*> specConsumers;
		for (auto& [_,spec]: Spec::all) {
			for (auto [iid,_]: spec->materials) {
				if (iid == item->id) {
					specConsumers.insert(spec);
				}
			}
		}

		if (recipeConsumers.size() || specConsumers.size()) {
			Section("Consumed by");
			for (auto& recipe: recipeConsumers) drawRecipeButton(recipe);
			for (auto& spec: specConsumers) drawSpecButton(spec);
			SpacingV();
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.item[item] = false;
}

void RecipePopup::drawItemButton(Item* item) {
	Bullet();
	if (SmallButtonInline(item->title.c_str())) {
		locate.item = item;
		expanded.item[item] = !expanded.item[item];
	}
	highlited.item[item] = highlited.item[item] || IsItemHovered();
}

void RecipePopup::drawItemButtonNoBullet(Item* item) {
	if (SmallButton(item->title.c_str())) {
		locate.item = item;
		expanded.item[item] = !expanded.item[item];
	}
	highlited.item[item] = highlited.item[item] || IsItemHovered();
}

void RecipePopup::drawItemButton(Item* item, int count) {
	Bullet();
	if (SmallButtonInline(fmtc("%s(%d)", item->title, count))) {
		locate.item = item;
		expanded.item[item] = !expanded.item[item];
	}
	highlited.item[item] = highlited.item[item] || IsItemHovered();
}

void RecipePopup::drawItemButton(Item* item, int count, int limit) {
	Bullet();
	if (SmallButtonInline(fmtc("%s(%d/%d)", item->title, count, limit))) {
		locate.item = item;
		expanded.item[item] = !expanded.item[item];
	}
	highlited.item[item] = highlited.item[item] || IsItemHovered();
}

bool RecipePopup::showFluid(Fluid* fluid) {
	return showUnavailableItemsFluids || fluid->manufacturable() || expanded.fluid[fluid];
}

void RecipePopup::drawFluid(Fluid* fluid) {
	if (!showFluid(fluid)) return;

	if (locate.fluid == fluid) {
		SetScrollHereY();
		locate.fluid = nullptr;
	}

	int pop = 0;

	if (highlited.fluid[fluid]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if (!fluid->manufacturable() && !highlited.fluid[fluid]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.fluid[fluid]);
	PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.fluid[fluid] = CollapsingHeader(fmtc("##fluid-%d", fluid->id));
	PopStyleColor(1);

	SameLine();
	fluidIcon(fluid);

	SameLine();
	Print(fluid->title.c_str());

	if (expanded.fluid[fluid]) {

		miniset<Recipe*> producers;
		for (auto& [_,recipe]: Recipe::names) {
			if (recipe->outputFluids.count(fluid->id)) producers.insert(recipe);
		}

		if (producers.size()) {
			Section("Produced by");
			for (auto& recipe: producers) drawRecipeButton(recipe);
			SpacingV();
		}

		miniset<Recipe*> consumers;
		for (auto& [_,recipe]: Recipe::names) {
			if (recipe->inputFluids.count(fluid->id)) consumers.insert(recipe);
		}

		if (consumers.size()) {
			Section("Consumed by");
			for (auto& recipe: consumers) drawRecipeButton(recipe);
			SpacingV();
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.fluid[fluid] = false;
}

void RecipePopup::drawFluidButton(Fluid* fluid) {
	Bullet();
	if (SmallButtonInline(fluid->title.c_str())) {
		locate.fluid = fluid;
		expanded.fluid[fluid] = !expanded.fluid[fluid];
	}
	highlited.fluid[fluid] = highlited.fluid[fluid] || IsItemHovered();
}

void RecipePopup::drawFluidButton(Fluid* fluid, int count) {
	Bullet();
	if (SmallButtonInline(fmtc("%s(%d)", fluid->title, count))) {
		locate.fluid = fluid;
		expanded.fluid[fluid] = !expanded.fluid[fluid];
	}
	highlited.fluid[fluid] = highlited.fluid[fluid] || IsItemHovered();
}

void RecipePopup::drawRecipe(Recipe* recipe) {

	if (!showUnavailableRecipes && !recipe->licensed && !expanded.recipe[recipe]) return;

	if (locate.recipe == recipe) {
		SetScrollHereY();
		locate.recipe = nullptr;
	}

	int pop = 0;

	if (highlited.recipe[recipe]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if ((!recipe->licensed || !recipe->manufacturable()) && !highlited.recipe[recipe]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.recipe[recipe]);
	PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.recipe[recipe] = CollapsingHeader(fmtc("##recipe-%s", recipe->name));
	bool faint = !highlited.recipe[recipe] && !IsItemHovered();
	PopStyleColor(1);

	SameLine();
	recipeIcon(recipe);

	SameLine();
	Print(recipe->title.c_str());

	SameLine();
	if (faint) PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	PushFont(Config::sidebar.font.imgui);
	PrintRight(recipe->energyUsage.format(), true);
	PopFont();
	if (faint) PopStyleColor(1);

	if (expanded.recipe[recipe]) {

		if (recipe->inputItems.size() || recipe->inputFluids.size()) {
			Section("Inputs");
			for (auto [iid,count]: recipe->inputItems) drawItemButton(Item::get(iid), count);
			SpacingV();
			for (auto [fid,count]: recipe->inputFluids) drawFluidButton(Fluid::get(fid), count);
			SpacingV();
		}

		if (recipe->outputItems.size() || recipe->outputFluids.size()) {
			Section("Outputs");
			for (auto [iid,count]: recipe->outputItems) drawItemButton(Item::get(iid), count);
			SpacingV();
			for (auto [fid,count]: recipe->outputFluids) drawFluidButton(Fluid::get(fid), count);
			SpacingV();
		}

		miniset<Spec*> specs;
		for (auto& [_,spec]: Spec::all) {
			for (auto& tag: spec->recipeTags) {
				if (recipe->tags.count(tag)) specs.insert(spec);
			}
		}
		if (specs.size()) {
			Section("Made in");
			for (auto& spec: specs) drawSpecButton(spec);
			SpacingV();
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.recipe[recipe] = false;
}

void RecipePopup::drawRecipeButton(Recipe* recipe) {
	Bullet();
	if (SmallButtonInline(recipe->title.c_str())) {
		locate.recipe = recipe;
		expanded.recipe[recipe] = !expanded.recipe[recipe];
	}
	highlited.recipe[recipe] = highlited.recipe[recipe] || IsItemHovered();
}

void RecipePopup::drawSpec(Spec* spec) {

	if (!showUnavailableSpecs && !spec->licensed && !expanded.spec[spec]) return;

	if (locate.spec == spec) {
		SetScrollHereY();
		locate.spec = nullptr;
	}

	int pop = 0;

	if (highlited.spec[spec]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if (!spec->licensed && !highlited.spec[spec]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.spec[spec]);
	PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.spec[spec] = CollapsingHeader(fmtc("##build-spec-%s", spec->name));
	bool faint = !highlited.spec[spec] && !IsItemHovered();
	PopStyleColor(1);

	SameLine();
	specIcon(spec);

	SameLine();
	Print(spec->title.c_str());

	if (faint) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	}

	PushFont(Config::sidebar.font.imgui);

	if (spec->consumeElectricity && spec->energyConsume) {
		SameLine();
		PrintRight(spec->energyConsume.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity consumer", (spec->energyConsume + spec->conveyorEnergyDrain).formatRate()));
	} else
	if (spec->consumeFuel && spec->energyConsume) {
		SameLine();
		PrintRight(spec->energyConsume.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s fuel consumer", spec->energyConsume.formatRate()));
	} else
	if (spec->conveyorEnergyDrain) {
		SameLine();
		PrintRight(spec->conveyorEnergyDrain.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity consumer", spec->conveyorEnergyDrain.formatRate()));
	} else
	if (spec->energyGenerate) {
		SameLine();
		PrintRight(spec->energyGenerate.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity generator", spec->energyGenerate.formatRate()));
	} else
	if (spec->depot) {
		SameLine();
		PrintRight(fmtc("x%d", spec->depotDrones), true);
		if (IsItemHovered()) tip(fmt("%d drones", spec->depotDrones));
	} else
	if (spec->tipStorage) {
		SameLine();
		PrintRight(spec->capacity.format().c_str(), true);
	} else
	if (spec->source && spec->sourceFluid) {
		SameLine();
		PrintRight(fmtc("%s/s", Liquid(spec->sourceFluidRate*60).format()), true);
	} else
	if (spec->pipe) {
		SameLine();
		PrintRight(spec->pipeCapacity.format().c_str(), true);
	} else
	if (spec->effector && spec->effectorElectricity > 0.0) {
		SameLine();
		PrintRight(fmtc("+%d%%", (int)std::floor(spec->effectorElectricity*100)), true);
		if (IsItemHovered()) tip(fmt(
			"+%d%% boost for electrical structures."
			" For crafters, higher speed."
			" For chargers, faster recharge.",
			(int)std::floor(spec->effectorElectricity*100)
		));
	} else
	if (spec->effector && spec->effectorFuel > 0.0) {
		SameLine();
		PrintRight(fmtc("+%d%%", (int)std::floor(spec->effectorFuel*100)), true);
		if (IsItemHovered()) tip(fmt(
			"+%d%% speed boost for fuel-burning structures.",
			(int)std::floor(spec->effectorFuel*100)
		));
	} else
	if (spec->bufferElectricity) {
		SameLine();
		PrintRight(spec->consumeChargeBuffer.format().c_str(), true);
	} else
	if (spec->turret && spec->turretLaser) {
		SameLine();
		PrintRight(fmtc("%dd", spec->damage()), true);
		if (IsItemHovered()) tip(
			"Damage per tick until stored energy is depleted."
			" Recharge rate can be boosted with battery packs."
		);
	} else
	if (spec->turret) {
		uint ammoId = Item::bestAmmo();
		// may not be unlocked
		if (ammoId) {
			SameLine();
			PrintRight(fmtc("%dd", spec->damage(ammoId)), true);
			if (IsItemHovered()) tip(fmt(
				"Damage per shot with %u tick cooldown using: %s",
				spec->turretCooldown,
				Item::get(ammoId)->title
			));
		}
	} else
	if (spec->consumeCharge && spec->energyConsume) {
		SameLine();
		PrintRight(spec->energyConsume.formatRate(), true);
		if (IsItemHovered()) tip(fmt("%s electricity consumer", spec->energyConsume.formatRate()));
	}

	PopFont();

	if (faint) {
		PopStyleColor(1);
	}

	if (expanded.spec[spec]) {

		if (spec->licensed) {
			if (BeginTable(fmtc("##buttons-%s", spec->name), spec->wiki.size() ? 3: 2)) {
				TableNextColumn();
				if (Button(fmtc("build##%s", spec->name), ImVec2(-1,0))) {
					scene.build(spec);
					show(false);
				}

				TableNextColumn();
				if (gui.toolbar->has(spec) && Button(fmtc("-toolbar##d%s", spec->name), ImVec2(-1,0))) {
					gui.toolbar->drop(spec);
				}
				else
				if (!gui.toolbar->has(spec) && Button(fmtc("+toolbar##a%s", spec->name), ImVec2(-1,0))) {
					gui.toolbar->add(spec);
				}

				TableNextColumn();
				if (spec->wiki.size() && Button("wiki", ImVec2(-1,0))) {
					openURL(spec->wiki);
				}

				EndTable();
			}
		}

		minivec<Spec*> iconSpecs;
		iconSpecs.push(spec);
		for (Spec* s = spec->cycle; s && s != spec; s = s->cycle) {
			if (!s->build) iconSpecs.push(s);
		}
		if (iconSpecs.size() > 1) {
			for (auto s: iconSpecs) {
				specIcon(s);
				if (IsItemHovered() && iconSpecs.size() > 1) tip("Use [C] to cycle ghost variants");
				SameLine();
			}
			NewLine();
		}

		if (spec->materials.size()) {
			Section("Construction Materials");
			for (auto& [iid,count]: spec->materials) drawItemButton(Item::get(iid), count);
			SpacingV();
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.spec[spec] = false;
}

void RecipePopup::drawSpecButton(Spec* spec) {
	if (!spec->build) return;
	Bullet();
	if (SmallButtonInline(spec->title.c_str())) {
		locate.spec = spec;
		expanded.spec[spec] = !expanded.spec[spec];
	}
	highlited.spec[spec] = highlited.spec[spec] || IsItemHovered();
}

void RecipePopup::drawGoal(Goal* goal) {

//	if (!showUnavailable && !goal->licensed && !expanded.goal[goal]) return;

	if (locate.goal == goal) {
		SetScrollHereY();
		locate.goal = nullptr;
	}

	int pop = 0;

	if (highlited.goal[goal]) {
		PushStyleColor(ImGuiCol_Header, GetStyleColorVec4(ImGuiCol_HeaderHovered));
		pop++;
	}

	if (!goal->active() && !highlited.goal[goal] && !expanded.goal[goal]) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
		pop++;
	}

	SetNextItemOpen(expanded.goal[goal]);
	if (!goal->active()) PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	expanded.goal[goal] = CollapsingHeader(fmtc("%s##build-goal-%s", goal->title, goal->name));
	if (!goal->active()) PopStyleColor(1);

	if (goal->met) {
		SameLine();
		PushFont(Config::sidebar.font.imgui);
		PrintRight("met", true);
		PopFont();
	}

	if (expanded.goal[goal]) {

		auto colorStore = ImColorSRGB(0x999999ff);

		if (goal->construction.size()) {
			Section("Construction");
			for (auto [spec,count]: goal->construction) {
				drawSpecButton(spec);
				SameLine(); PrintRight(fmtc("%u/%u", spec->count.extant, count));
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				SmallBar((float)spec->count.extant/(float)count);
				PopStyleColor(1);
				SpacingV();
			}
		}
		if (goal->productionItem.size() || goal->productionFluid.size()) {
			Section("Production");
			for (auto [iid,count]: goal->productionItem) {
				auto item = Item::get(iid);
				drawItemButton(item);
				SameLine(); PrintRight(fmtc("%u/%u", item->produced, count));
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				SmallBar((float)item->produced/(float)count);
				PopStyleColor(1);
				SpacingV();
			}
			for (auto [fid,count]: goal->productionFluid) {
				auto fluid = Fluid::get(fid);
				drawFluidButton(fluid);
				SameLine(); PrintRight(fmtc("%u/%u", fluid->produced, count));
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				SmallBar((float)fluid->produced/(float)count);
				PopStyleColor(1);
				SpacingV();
			}
		}
		if (goal->supplies.size()) {
			Section("Supply");
			for (auto stack: goal->supplies) {
				drawItemButton(Item::get(stack.iid));
				SameLine(); PrintRight(fmtc("%u/%u", Item::supplied[stack.iid], stack.size));
				PushStyleColor(ImGuiCol_PlotHistogram, colorStore);
				SmallBar((float)Item::supplied[stack.iid]/(float)stack.size);
				PopStyleColor(1);
				SpacingV();
			}
		}
		if (goal->rates.size()) {
			uint minute = 60*60;
			uint hour = minute*60;
			Section(fmtc("Supply over any %dh period", goal->period/hour));
			if (IsItemHovered()) tip(fmt(
				"Throughput goals are moving windows, and will auto-complete when"
				" supply rates for the listed items are sustained at or above the"
				" target level(s) for the last N hours.\n\n"
				"Basically, make everything turn green on the charts.\n\n"
				"Balancing and scaling up the factory to meet the target rate is"
				" the intention, but if you prefer to stockpile and manually"
				" release items for launch then go for it!\n\n"
				"Suggestion: consider stockpiling a moderate buffer as a fallback"
				" but also attempt to hit the sustained throughput target."
				" Whatever the outcome, later goals will consume any excess products."
			));
			for (auto& rate: goal->rates) {
				drawItemButtonNoBullet(Item::get(rate.iid));
				SameLine(); PrintRight(fmtc("%d/h", rate.count), true);
				SpacingV();
				goalRateChart(goal, rate, (float)Config::height(0.05f));
			}
		}

		if (goal->license.specs.size() || goal->license.recipes.size()) {
			Section("Unlock");
			for (auto spec: goal->license.specs) if (spec->build) { drawSpecButton(spec); }
			for (auto recipe: goal->license.recipes) { drawRecipeButton(recipe); }
			SpacingV();
		}

		if (goal->reward) {
			Section("Perks");
			Print("Upgrade points");
			SameLine(); PrintRight(fmtc("%d", goal->reward));
		}
	}

	if (pop) {
		PopStyleColor(pop);
	}

	highlited.goal[goal] = false;
}

void RecipePopup::drawGoalButton(Goal* goal) {

	if (SmallButtonInline(goal->title.c_str())) {
		locate.goal = goal;
		expanded.goal[goal] = !expanded.goal[goal];
	}
	highlited.goal[goal] = highlited.goal[goal] || IsItemHovered();
}

UpgradePopup::UpgradePopup() : Popup() {
}

UpgradePopup::~UpgradePopup() {
}

void UpgradePopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		small();
		Begin(fmtc("Upgrade (%d)##upgrade", Goal::chits), &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if (BeginTable("##upgrades", 2)) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

			struct Rate {
				const char* title;
				float* rate;
				float maxRate;
				float bump;
				std::vector<float> splits;
				std::vector<ImU32> colors;
			};

			std::vector<float> splits3 = {
				0.33f, 0.66f, 1.0f,
			};

			std::vector<ImU32> colors3 = {
				ImColorSRGB(0x00aa00ff),
				ImColorSRGB(0x0000ffff),
				ImColorSRGB(0xaa0000ff),
			};

			std::vector<float> splits2 = {
				0.5f, 1.0f,
			};

			std::vector<ImU32> colors2 = {
				ImColorSRGB(0x00aa00ff),
				ImColorSRGB(0xaa0000ff),
			};

			std::vector<Rate> rates = {
				{"Mining", &Recipe::miningRate, 3.0f, 0.1f, splits3, colors3},
				{"Drilling", &Recipe::drillingRate, 3.0f, 0.1f, splits3, colors3},
				{"Crushing", &Recipe::crushingRate, 3.0f, 0.1f, splits3, colors3},
				{"Smelting", &Recipe::smeltingRate, 3.0f, 0.1f, splits3, colors3},
				{"Crafting", &Recipe::craftingRate, 3.0f, 0.1f, splits3, colors3},
				{"Refining", &Recipe::refiningRate, 3.0f, 0.1f, splits3, colors3},
				{"Centrifuging", &Recipe::centrifugingRate, 3.0f, 0.1f, splits3, colors3},
				{"Drones", &Drone::speedFactor, 2.0f, 0.1f, splits2, colors2},
				{"Arm", &Arm::speedFactor, 2.0f, 0.1f, splits2, colors2},
			};

			for (auto& area: rates) {

				TableNextColumn();
					BeginGroup();
						Print(area.title);
						MultiBar(*area.rate / area.maxRate, area.splits, area.colors);
					EndGroup();

				auto size = GetItemRectSize();

				TableNextColumn();
					BeginDisabled(!Goal::chits);
					if (Button(fmtc(" + ##rate-%s", area.title), ImVec2(0,size.y)) && Goal::chits > 0 && *area.rate < area.maxRate-0.01f) {
						Goal::chits--;
						*area.rate = std::floor((*area.rate + area.bump) * 100.0f) / 100.0f;
					}
					if (IsItemHovered()) tip(
						fmt("%d%% / %d%%",
							(int)std::floor(*area.rate * 100.0f),
							(int)std::floor(area.maxRate * 100.0f)
						));
					EndDisabled();
			}

			EndTable();
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

SignalsPopup::SignalsPopup() : Popup() {
}

SignalsPopup::~SignalsPopup() {
}

void SignalsPopup::draw() {
	bool showing = true;

	auto focusedTab = [&]() {
		bool focused = opened;
		opened = false;
		return (focused ? ImGuiTabItemFlags_SetSelected: ImGuiTabItemFlags_None) | ImGuiTabItemFlags_NoPushId;
	};

	Sim::locked([&]() {

		narrow();
		Begin("Custom Signals##signals-popup", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if (IsWindowAppearing()) {
			inputs.clear();
			create[0] = 0;
		}

		bool created = false;

		if (BeginTabBar("main-tabs", ImGuiTabBarFlags_None)) {

			if (BeginTabItem("Active", nullptr, focusedTab())) {

				if (BeginTable("##signals-table-active", 2)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

					TableNextColumn();
					SetNextItemWidth(-1);
					created = InputTextWithHint("##signal-create", "<new signal>", create, sizeof(create), ImGuiInputTextFlags_EnterReturnsTrue);
					inputFocused = IsItemActive();

					TableNextColumn();
					created = Button("Create") || created;

					std::vector<Signal::Label> labels;
					for (auto& label: Signal::labels) {
						if (label.drop) continue;
						if (!inputs.count(label.id)) {
							auto& buf = inputs[label.id];
							snprintf(buf, sizeof(buf), "%s", label.name.c_str());
						}
						labels.push_back(label);
					}
					if (IsWindowAppearing()) {
						std::sort(labels.begin(), labels.end(), [](auto a, auto b) {
							return a.name < b.name;
						});
					}

					for (auto& label: labels) {
						if (label.drop) continue;
						TableNextColumn();
						SetNextItemWidth(-1);
						InputText(fmtc("##signal-modify-%u", label.id), inputs[label.id], sizeof(inputs[label.id]));
						inputFocused = IsItemActive();

						if (inputs[label.id][0]) {
							auto name = std::string(inputs[label.id]);
							Signal::modLabel(label.id, name);
						}

						TableNextColumn();
						if (Button("Retire")) {
							Signal::dropLabel(label.id);
						}
					}

					minivec<uint> dropA;
					for (auto& [id,_]: inputs) if (!Signal::findLabel(id)) dropA.push(id);
					for (auto& id: dropA) inputs.erase(id);

					EndTable();
				}

				if (created && create[0]) {
					auto label = std::string(create);
					Signal::addLabel(label);
					create[0] = 0;
				}

				EndTabItem();
			}

			if (BeginTabItem("Retired", nullptr, focusedTab())) {

				if (BeginTable("##signals-table-dropped", 2)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

					std::vector<Signal::Label> labels;
					for (auto& label: Signal::labels) {
						if (!label.drop) continue;
						labels.push_back(label);
					}
					std::sort(labels.begin(), labels.end(), [](auto a, auto b) {
						return a.name < b.name;
					});

					for (auto& label: labels) {
						TableNextColumn();
						Print(fmtc("%s", label.name));

						TableNextColumn();
						if (Button("Restore")) {
							Signal::addLabel(label.name);
						}
					}

					EndTable();
				}
				EndTabItem();
			}
			EndTabBar();
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

PlanPopup::PlanPopup() : Popup() {
}

PlanPopup::~PlanPopup() {
}

void PlanPopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		small();
		Begin("Blueprint##blueprint", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);


		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

PaintPopup::PaintPopup() : Popup() {
}

PaintPopup::~PaintPopup() {
}

void PaintPopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		h = (float)Config::height(0.11f);
		w = (float)Config::height(0.25f);

		const ImVec2 size = {
			(float)w,(float)h
		};

		const ImVec2 pos = {
			((float)Config::window.width-size.x)/2.0f,
			((float)Config::window.height-size.y-gui.toolbar->size.y),
		};

		SetNextWindowSize(size, ImGuiCond_Always);
		SetNextWindowPos(pos, ImGuiCond_Always);

		Begin("##paint", &showing, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		minivec<Entity*> selectedPaint;

		for (auto& ge: scene.selected) {
			if (ge->spec->coloredCustom && Entity::exists(ge->id)) {
				selectedPaint.push(&Entity::get(ge->id));
			}
		}

		if (!selectedPaint.size()) {
			show(false);
			return;
		}

		if (IsWindowAppearing()) {
			Color srgb = selectedPaint.front()->color().gamma();
			rgb.r = srgb.r;
			rgb.g = srgb.g;
			rgb.b = srgb.b;
			colors.clear();
			for (auto& [_,c]: Entity::colors) colors.insert(c);
			for (auto& [_,spec]: Spec::all) if (spec->coloredCustom) colors.insert(spec->color);
		}

		if (BeginTable("##paint-layout", 2)) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, w*0.5);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

			TableNextRow();

			TableNextColumn();
			SetNextItemWidth(w*0.5);
			if (ColorPicker3("##paint-picker", &rgb.r, ImGuiColorEditFlags_NoLabel|ImGuiColorEditFlags_NoAlpha|ImGuiColorEditFlags_NoTooltip|ImGuiColorEditFlags_NoOptions|ImGuiColorEditFlags_NoSmallPreview|ImGuiColorEditFlags_NoSidePreview)) {
				for (auto en: selectedPaint) {
					en->color(Color(rgb.r, rgb.g, rgb.b, 1.0).degamma());
				}
			}

			TableNextColumn();
			for (int i = 0, l = colors.size(); i < l; i++) {
				Color c = colors[i];
				Color srgb = colors[i].gamma();
				if (ColorButton(fmtc("paint##paint-prev-%d", i), (ImVec4){srgb.r,srgb.g,srgb.b,1.0}, ImGuiColorEditFlags_None|ImGuiColorEditFlags_NoTooltip)) {
					rgb.r = srgb.r;
					rgb.g = srgb.g;
					rgb.b = srgb.b;
					for (auto en: selectedPaint) {
						en->color(Color(c.r, c.g, c.b, 1.0));
					}
				}
				if ((i+1)%4 != 0) SameLine();
			}
			EndTable();
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

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

		auto tile = world.get({
			(int)std::floor(pointer.x),
			(int)std::floor(pointer.y)
		});

		if (tile && scene.keyDown(SDLK_SPACE)) {
			auto box = Point(tile->x, 0, tile->y).box().grow(0.5);
			scene.tipBegin(0.75f);
			Header("Resources");
			scene.tipResources(box, box);
			scene.tipEnd();
		}
	});
}

ZeppelinPopup::ZeppelinPopup() : Popup() {
}

ZeppelinPopup::~ZeppelinPopup() {
}

void ZeppelinPopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		narrow();
		Begin("Zeppelins", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if (BeginTable("##zeppelins", 3)) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

			auto dist = [](real d) {
				if (d > 1000) return fmt("%0.1fkm", d/1000.0);
				return fmt("%dm", (int)d);
			};

			for (auto& zeppelin: Zeppelin::all) {
				auto& en = Entity::get(zeppelin.id);
				TableNextRow();

				TableNextColumn();
				int iconSize = Config::toolbar.icon.size;
				float iconPix = Config::toolbar.icon.sizes[iconSize];
				Image((ImTextureID)scene.specIconTextures[en.spec][iconSize], ImVec2(iconPix, iconPix), ImVec2(0, 1), ImVec2(1, 0));

				TableNextColumn();
				Print(en.name().c_str());
				PushFont(Config::sidebar.font.imgui);
				Print(dist(en.pos().distance(scene.position)).c_str());
				PopFont();

				TableNextColumn();
				if (Button(fmtc(" Goto ##zeppelin-%d", zeppelin.id))) {
					scene.view(en.pos());
					delete scene.directing;
					scene.directing = new GuiEntity(&en);
					show(false);
				}
				SameLine();
				if (Button(fmtc(" Call ##zeppelin-%d", zeppelin.id))) {
					delete scene.directing;
					scene.directing = new GuiEntity(&en);
					en.zeppelin().flyOver(scene.target);
					show(false);
				}
			}

			EndTable();
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

MainMenu::MainMenu() : Popup() {
	capsule = loadTexture("assets/capsule.png");
}

MainMenu::~MainMenu() {
}

void MainMenu::draw() {
	bool showing = true;

	auto reset = [&]() {
		saveas[0] = 0;
		create[0] = 0;
		games.clear();
	};

	narrow();
	Begin(fmtc("%s : %ld##mainmenu", Config::mode.saveName, Sim::seed), &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if (IsWindowAppearing()) {
			reset();
			saveStatus = SaveStatus::Current;
		}

		auto focusedTab = [&]() {
			bool focused = opened;
			opened = false;
			return (focused ? ImGuiTabItemFlags_SetSelected: ImGuiTabItemFlags_None) | ImGuiTabItemFlags_NoPushId;
		};

		if (BeginTabBar("main-tabs", ImGuiTabBarFlags_None)) {
			if (BeginTabItem("Game", nullptr, focusedTab())) {

				SpacingV();
				ImageBanner(capsule.id, capsule.w, capsule.h);
				SpacingV();

				Notice(Config::version.text);

				if (IsWindowAppearing()) {
					quitting.period = 0;
					quitting.ticker = 0;
				}

				if (quitting.period > 0 && quitting.ticker < quitting.period) {
					quitting.ticker++;
				}

				if (quitting.period > 0 && quitting.ticker >= quitting.period) {
					gui.doQuit = true;
				}

				if (quitting.period == 0 && Button("Quit", ImVec2(-1,0))) {
					quitting.period = gui.fps*3;
					quitting.ticker = 0;
				}

				if (quitting.period > 0 && Button("Continue", ImVec2(-1,0))) {
					quitting.period = 0;
					quitting.ticker = 0;
				}

				if (quitting.period > 0) {
					SmallBar((float)quitting.ticker/(float)quitting.period);
				}

				SpacingV();

				PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GetStyle().ItemSpacing.x/2,0));

				if (BeginTable("##menu-game", 2)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.5);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GetContentRegionAvail().x*0.5);

					TableNextColumn();

					if (Button(Config::mode.pause ? "Resume": "Pause", ImVec2(-1,0))) {
						Config::mode.pause = !Config::mode.pause;
					}
					if (IsItemHovered()) tip("[Pause]");
					SpacingV();

					TableNextColumn();

					if (Button("Signals", ImVec2(-1,0))) {
						gui.doSignals = true;
					}
					if (IsItemHovered()) tip(
						"Custom signals for use in rules and on wifi."
					);
					SpacingV();

					TableNextColumn();

					if (Button("Zeppelins [Z]", ImVec2(-1,0))) {
						gui.doZeppelins = true;
					}

					TableNextColumn();

					if (Button("Map [M]", ImVec2(-1,0))) {
						gui.doMap = true;
					}

					EndTable();
				}
				PopStyleVar();

				SpacingV();

				Section("Settings");

				PushItemWidth(GetContentRegionAvail().x*0.5);
				InputIntClamp("Autosave##autosave", &Config::mode.autosave, 5, 60, 5, 5);
				if (IsItemHovered()) tip(
					"Autosave interval in minutes."
				);
				InputIntClamp("UPS", &Config::engine.ups, 1, 1000, 1, 1);
				if (IsItemHovered()) tip(
					"Simulation updates per second. Increase to make time speed up."
				);
				PopItemWidth();

				Checkbox("Lock UPS to FPS", &Config::engine.pulse);
				if (IsItemHovered()) tip(
					"UPS and FPS are loosely coupled by default, allowing FPS to"
					" fluctuate without impacting the apparent speed of the game."
				);

				Checkbox("Build Grid [G]", &Config::mode.grid);
				if (IsItemHovered()) tip(
					"Show the build grid when zoomed in."
				);

				Checkbox("Build Alignment [H]", &Config::mode.alignment);
				if (IsItemHovered()) tip(
					"Show alignment lines when placing a ghost or blueprint."
				);

				Checkbox("Camera Snap [TAB]", &Config::mode.cardinalSnap);
				if (IsItemHovered()) tip(
					"Align the camera when close to the cardinal directions: North, South, East, West."
				);

				Checkbox("Tip Window", &Config::mode.autotip);
				if (IsItemHovered()) tip(
					"Show the tip window after 1s of mouse inactivity. [SPACEBAR] will always manually show it."
				);

				Checkbox("Enemy Attacks", &Enemy::enable);
				if (IsItemHovered()) tip(
					"The enemy, livid that your logistics skills are so awesome, will"
					" periodically send missiles to disrupt your supply chain."
				);

				EndTabItem();
			}

			if (BeginTabItem("Graphics", nullptr, focusedTab())) {

				SpacingV();
				Section("Quality");
				if (BeginTable("##renderer-quick-buttons", 3)) {
					TableNextRow();
					TableNextColumn();
					if (Button("High", ImVec2(-1,0))) {
						Config::window.horizon = 1500;
						Config::window.fog = 1000;
						Config::window.zoomUpperLimit = 500;
						Config::window.levelsOfDetail[0] = 150;
						Config::window.levelsOfDetail[1] = 600;
						Config::window.levelsOfDetail[2] = 1000;
						Config::window.levelsOfDetail[3] = 1500;
						Config::mode.shadowmap = true;
						Config::mode.itemShadows = true;
						Config::mode.meshMerging = true;
						Config::mode.waterwaves = true;
						Config::mode.treebreeze = true;
						Config::mode.filters = true;
					}

					TableNextColumn();
					if (Button("Medium", ImVec2(-1,0))) {
						Config::window.horizon = 1200;
						Config::window.fog = 800;
						Config::window.zoomUpperLimit = 400;
						Config::window.levelsOfDetail[0] = 100;
						Config::window.levelsOfDetail[1] = 500;
						Config::window.levelsOfDetail[2] = 800;
						Config::window.levelsOfDetail[3] = 1200;
						Config::mode.shadowmap = true;
						Config::mode.itemShadows = true;
						Config::mode.meshMerging = true;
						Config::mode.waterwaves = true;
						Config::mode.treebreeze = true;
						Config::mode.filters = true;
					}

					TableNextColumn();
					if (Button("Low", ImVec2(-1,0))) {
						Config::window.horizon = 900;
						Config::window.fog = 500;
						Config::window.zoomUpperLimit = 300;
						Config::window.levelsOfDetail[0] = 50;
						Config::window.levelsOfDetail[1] = 300;
						Config::window.levelsOfDetail[2] = 500;
						Config::window.levelsOfDetail[3] = 800;
						Config::mode.shadowmap = true;
						Config::mode.itemShadows = false;
						Config::mode.meshMerging = true;
						Config::mode.waterwaves = false;
						Config::mode.treebreeze = false;
						Config::mode.filters = false;
					}

					EndTable();
				}
				SpacingV();

				Section("Custom");
				PushItemWidth(GetContentRegionAvail().x*0.5);

				InputIntClamp("FPS", &Config::window.fps, 10, 1000, 10, 10);
				if (IsItemHovered()) tip(
					"Frames per second. Only works if the game was started with VSYNC off."
					" FPS and UPS are only loosely coupled however there is a stage on every frame"
					" where the renderer must lock the simulation to extract data for the entities"
					" in view. Therefore setting FPS too high will eventually impact UPS."
				);

				InputIntClamp("FOV", &Config::window.fov, 45, 135, 5, 5);
				if (IsItemHovered()) tip(
					"Horizontal field of view."
				);

				InputIntClamp("Horizon (m)", &Config::window.horizon, 750, 1500, 10, 100);
				if (IsItemHovered()) tip(
					"Distance from ground zero (the point on the ground the camera is pointing at)"
					" to render entities. Terrain is always rendered at least this far too."
				);

				InputIntClamp("Fog (m)", &Config::window.fog, 100, 1000, 10, 100);
				if (IsItemHovered()) tip(
					"Distance from ground zero (the point on the ground the camera is pointing at)"
					" where fog blur begins."
				);

				InputIntClamp("Zoom (m)", &Config::window.zoomUpperLimit, 100, 1000, 10, 100);
				if (IsItemHovered()) tip(
					"Maximum camera distance from ground zero (the point on the ground the camera is pointing at)."
				);

				InputIntClamp("LOD close (m)", &Config::window.levelsOfDetail[0], 10, 1500, 10, 100);
				if (IsItemHovered()) tip(
					"Distance from the camera to render high-detail entity and item models."
				);

				InputIntClamp("LOD near (m)", &Config::window.levelsOfDetail[1], Config::window.levelsOfDetail[0], 1500, 10, 100);
				if (IsItemHovered()) tip(
					"Distance from the camera to render medium-detail entity and low-detail item models."
				);

				InputIntClamp("LOD far (m)", &Config::window.levelsOfDetail[2], Config::window.levelsOfDetail[1], 1500, 10, 100);
				if (IsItemHovered()) tip(
					"Distance from the camera to render low-detail entity models (items will be invisible)."
				);

				InputIntClamp("LOD distant (m)", &Config::window.levelsOfDetail[3], Config::window.levelsOfDetail[2], 1500, 10, 100);
				if (IsItemHovered()) tip(
					"Distance from the camera to render very low-detail entity models (items and small"
					" entities will be invisible)."
				);

				if (BeginCombo("##ground", Config::window.ground == Config::window.groundSand ? "sand": "grass")) {
					if (Selectable("sand", Config::window.ground == Config::window.groundSand)) {
						Config::window.ground = Config::window.groundSand;
						Config::window.grid = Config::window.gridSand;
					}
					if (Selectable("grass", Config::window.ground == Config::window.groundGrass)) {
						Config::window.ground = Config::window.groundGrass;
						Config::window.grid = Config::window.gridGrass;
					}
					EndCombo();
				}
				PopItemWidth();

				Checkbox("Shadows", &Config::mode.shadowmap);
				if (IsItemHovered()) tip(
					"Render shadows. Increases GPU load."
				);

				if (Config::mode.shadowmap) {
					Checkbox("Shadows: Items", &Config::mode.itemShadows);
					if (IsItemHovered()) tip(
						"Render item shadows. Increases GPU load."
					);
				}

				Checkbox("Mesh Merging", &Config::mode.meshMerging);
				if (IsItemHovered()) tip(
					"Generate merged meshes for groups of identical items on backed-up belts."
					" Reduces CPU load. Slightly increases GPU load."
				);

				Checkbox("Procedural Waves", &Config::mode.waterwaves);
				if (IsItemHovered()) tip(
					"Water motion. Slightly increases GPU load."
				);

				Checkbox("Procedural Breeze", &Config::mode.treebreeze);
				if (IsItemHovered()) tip(
					"Subtle tree motion. Slightly increases GPU load."
				);

				Checkbox("Procedural Textures", &Config::mode.filters);
				if (IsItemHovered()) tip(
					"Close-up surface effects on concrete and metal. Slightly increases GPU load."
				);

				Checkbox("MSAA", &Config::window.antialias);
				if (IsItemHovered()) tip(
					"Normal GL_MULTISAMPLE. Disable if better anti-aliasing is done via GPU driver settings."
				);

				if (Checkbox("VSYNC", &Config::window.vsync)) {
					SDL_GL_SetSwapInterval(Config::window.vsync ? 1:0);
				}

				Checkbox("Show UPS", &Config::mode.overlayUPS);
				Checkbox("Show FPS", &Config::mode.overlayFPS);

				EndTabItem();
			}

			if (BeginTabItem("Controls", nullptr, focusedTab())) {
				SpacingV();
				Warning("Not yet configurable.");

				int id = 0;
				float width = GetContentRegionAvail().x/2.0;

				auto display = [&](const char* name, Config::KeyMouseCombo combo, const char* text) {
					PushFont(Config::sidebar.font.imgui);
					if (BeginTable(fmtc("#controls-%d", ++id), 2, ImGuiTableFlags_BordersInnerH)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);
						TableNextRow();
						TableNextColumn();
						Print(name);
						std::string keys;
						for (const auto& mod: combo.mods) {
							if (keys.size()) keys += " + ";
							keys += std::string(SDL_GetKeyName(mod));
						}
						for (const auto& button: combo.buttonsDown) {
							if (keys.size()) keys += " + ";
							switch (button) {
								case SDL_BUTTON_LEFT:
									keys += "Left Mouse";
									break;
								case SDL_BUTTON_MIDDLE:
									keys += "Middle Mouse";
									break;
								case SDL_BUTTON_RIGHT:
									keys += "Right Mouse";
									break;
							}
						}
						for (const auto& button: combo.buttonsReleased) {
							if (keys.size()) keys += " + ";
							switch (button) {
								case SDL_BUTTON_LEFT:
									keys += "Left Mouse";
									break;
								case SDL_BUTTON_MIDDLE:
									keys += "Middle Mouse";
									break;
								case SDL_BUTTON_RIGHT:
									keys += "Right Mouse";
									break;
							}
						}
						for (const auto& mod: combo.keysReleased) {
							if (keys.size()) keys += " + ";
							keys += std::string(SDL_GetKeyName(mod));
						}
						TableNextColumn();
						Print(keys);
						EndTable();
					}
					if (IsItemHovered()) tip(text);
					PopFont();
				};

				Section("Game Interface");

				PushFont(Config::sidebar.font.imgui);
				if (BeginTable(fmtc("#controls-0", ++id), 2, ImGuiTableFlags_BordersInnerH)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);

					TableNextRow();
					TableNextColumn();
					Print("Information");
					TableNextColumn();
					Print("Hover + Space Bar");
					EndTable();
				}
				if (IsItemHovered()) tip(
					"Hover the mouse over almost anything in the game"
					" -- hills, lakes, structures, vehicles --"
					" and hold [SPACEBAR] to see useful information."
				);
				PopFont();

				display("Escape", Config::controls[Config::Action::Escape],
					"Stop the current action. Close the current window."
				);
				display("Save", Config::controls[Config::Action::Save],
					"Background save the current game."
				);
				display("Build", Config::controls[Config::Action::Build],
					"Open the build window."
				);
				display("Pause", Config::controls[Config::Action::Pause],
					"Pause the game."
				);
				display("Open", Config::controls[Config::Action::Open],
					"Open an entity window."
				);

				SpacingV();
				Section("Ghosts and Blueprints");

				display("Cycle", Config::controls[Config::Action::Cycle],
					"Cycle through ghost variations: straight belt or corner belts. Underground entry or exit. Pipe configurations."
				);
				display("SpecUp", Config::controls[Config::Action::SpecUp],
					"Switch to a related ghost variation. Belt to balancer. Balancer to loader."
				);
				display("SpecDown", Config::controls[Config::Action::SpecDown],
					"Switch to a related ghost variation. Belt to Underground belt. Pipe to underground pipe."
				);
				display("Construct", Config::controls[Config::Action::Construct],
					"Place a ghost or blueprint for construction."
				);
				display("ConstructForce", Config::controls[Config::Action::ConstructForce],
					"Place a ghost or blueprint for construction ignoring any conflicts."
				);
				display("Copy", Config::controls[Config::Action::Copy],
					"Create a blueprint from selected entities."
				);
				display("Cut", Config::controls[Config::Action::Cut],
					"Create a blueprint from selected entities, and schedule them for deconstruction."
				);
				display("Paste", Config::controls[Config::Action::Paste],
					"Load the last blueprint."
				);
				display("CopyConfig", Config::controls[Config::Action::CopyConfig],
					"Copy settings from hovered entity."
				);
				display("PasteConfig", Config::controls[Config::Action::PasteConfig],
					"Apply the most recent copied settings to hovered or selected entities."
				);
				display("Pipette", Config::controls[Config::Action::Pipette],
					"Copy an entity as a ghost."
				);
				display("Rotate", Config::controls[Config::Action::Rotate],
					"Rotate a ghost or entity (not supported by all entity types)."
				);
				display("Deconstruct", Config::controls[Config::Action::Deconstruct],
					"Schedule the selected entities for deconstruction."
				);
				display("DeconstructForce", Config::controls[Config::Action::DeconstructForce],
					"Schedule the selected entities for deconstruction."
				);
				display("ToggleConstruct", Config::controls[Config::Action::ToggleConstruct],
					"Switch a ghost between construction and deconstruction."
				);
				display("Upgrade", Config::controls[Config::Action::Upgrade],
					"Schedule an entity for upgrade. If no upgrade path exists or has been unlocked, nothing will happen."
				);
				display("UpgradeCascade", Config::controls[Config::Action::UpgradeCascade],
					"Upgrade a set of connected belts or tubes."
				);
				display("SelectJunk", Config::controls[Config::Action::SelectJunk],
					"Toggle selection between normal entities and trees/rocks."
				);
				display("SelectUnder", Config::controls[Config::Action::SelectUnder],
					"Toggle selection between normal entities and piles/slabs."
				);

				SpacingV();
				Section("Vehicle Control");

				display("Direct", Config::controls[Config::Action::Direct],
					"Set the currently controlled entity: Zeppelin, Blimp, Cart, Truck."
				);
				display("Move", Config::controls[Config::Action::Move],
					"Move the currently controlled entity: Zeppelin, Blimp, Cart, Truck."
				);

				SpacingV();
				Section("Links and Routes");

				display("Link", Config::controls[Config::Action::Link],
					"Create a link from an entity to another: waypoint to waypoint, tube to tube."
				);
				display("Connect", Config::controls[Config::Action::Connect],
					"Connect the last linked entity to another."
				);
				display("Disconnect", Config::controls[Config::Action::Disconnect],
					"Disconnect the last linked entity from another."
				);
				display("RouteRed", Config::controls[Config::Action::RouteRed],
					"Set the linked waypoint to create a red route."
				);
				display("RouteBlue", Config::controls[Config::Action::RouteBlue],
					"Set the linked waypoint to create a blue route."
				);
				display("RouteGreen", Config::controls[Config::Action::RouteGreen],
					"Set the linked waypoint to create a green route."
				);
//				display("RouteSetNext", Config::controls[Config::Action::RouteSetNext],
//					"Make a coloured route between waypoints."
//				);
//				display("RouteClrNext", Config::controls[Config::Action::RouteClrNext],
//					"Remove a coloured route between waypoints."
//				);

				SpacingV();
				Section("QoL");

				display("Flush", Config::controls[Config::Action::Flush],
					"Flush a pipe of fluid. Clear a belt or tube of items."
				);
				display("ToggleGrid", Config::controls[Config::Action::ToggleGrid],
					"Toggle the build grid."
				);
				display("ToggleAlignment", Config::controls[Config::Action::ToggleAlignment],
					"Toggle blueprint alignment lines."
				);
				display("ToggleEnable", Config::controls[Config::Action::ToggleEnable],
					"Toggle an entity on/off (enable, disable)."
				);
				display("ToggleCardinalSnap", Config::controls[Config::Action::ToggleCardinalSnap],
					"Toggle camera snapping near the cardinal directions: North, South, East, West."
				);

				SpacingV();
				Section("Other");

				display("Stats", Config::controls[Config::Action::Stats],
					"Open the stats window."
				);
				display("Map", Config::controls[Config::Action::Map],
					"Open the map window."
				);
				display("Paint", Config::controls[Config::Action::Paint],
					"Open the paint window."
				);
				display("Log", Config::controls[Config::Action::Log],
					"Open the log window."
				);
				display("Zeppelins", Config::controls[Config::Action::Zeppelins],
					"Open the Zeppelins window."
				);

				EndTabItem();
			}
			EndTabBar();
		}
	End();
	if (visible) show(showing);
}

DebugMenu::DebugMenu() : Popup() {
}

DebugMenu::~DebugMenu() {
}

void DebugMenu::draw() {

	narrow();
	Begin("Debug##debugmenu", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		if (Button("magic container", ImVec2(-1,0))) {
			scene.build(Spec::byName("magic-container"));
			show(false);
		}

		if (Button("magic generator", ImVec2(-1,0))) {
			scene.build(Spec::byName("magic-generator"));
			show(false);
		}

		if (Button("complete current goal", ImVec2(-1,0))) {
			if (Goal::current()) Goal::current()->complete();
		}

		if (Button("complete all goals", ImVec2(-1,0))) {
			for (auto [_,goal]: Goal::all) goal->complete();
		}

		if (Button("license all specs", ImVec2(-1,0))) {
			for (auto [_,spec]: Spec::all) spec->licensed = true;
		}

		if (Button("trigger attack now", ImVec2(-1,0))) {
			Enemy::trigger = true;
		}

		if (Button("dump recipes", ImVec2(-1,0))) {
			Save::dumpRecipes();
		}

		Checkbox("debug overlay", &Config::mode.overlayDebug);
		Checkbox("sky blocks", &Config::mode.skyBlocks);

	End();
}

