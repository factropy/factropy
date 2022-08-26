#include "common.h"
#include "config.h"
#include "popup.h"
#include "enemy.h"
#include "save.h"

#include "../imgui/setup.h"
#include "stb_image.h"

using namespace ImGui;

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
		std::sort(items.begin(), items.end(), Item::sort);
		return items.size() ? items.front()->id: 0;
	};

	auto chooseFluid = [&](auto from) {
		minivec<Fluid*> fluids;
		for (auto& [fid,_]: from) {
			fluids.push(Fluid::get(fid));
		}
		std::sort(fluids.begin(), fluids.end(), Fluid::sort);
		return fluids.size() ? fluids.front()->id: 0;
	};

	if (recipe->outputSpec) {
		return specIconChoose(recipe->outputSpec, pix);
	}

	if (recipe->mine) {
		return itemIconChoose(Item::get(recipe->mine), pix);
	}

	if (recipe->drill) {
		return fluidIconChoose(Fluid::get(recipe->drill), pix);
	}

	if (recipe->outputItems.size() > 0) {
		return itemIconChoose(Item::get(chooseItem(recipe->outputItems)), pix);
	}

	if (recipe->fluid) {
		return fluidIconChoose(Fluid::get(recipe->fluid), pix);
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
			if (i%8 != 0) {
				Print(" + ");
				SameLine();
			}
			else
			if (i) {
				NewLine();
			}
			i++;
			itemIcon(Item::get(iid));
			SameLine();
			Print(fmtc("(%u)", count));
			SameLine();
		};

		auto fluid = [&](uint fid, uint count) {
			if (i%8 != 0) {
				Print(" + ");
				SameLine();
			}
			else
			if (i) {
				NewLine();
			}
			i++;
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
		if (recipe->outputSpec) {
			i++;
			recipeIcon(recipe);
			SameLine();
			Print("(1)");
			SameLine();
		}
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

bool Popup::signalPicker(bool open, bool metas) {

	if (open) OpenPopup("##signal-picker");

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

	bool chosen = false;
	signalPicked = Signal::Key();

	if (BeginPopup("##signal-picker")) {
		if (!open && scene.keyReleased(SDLK_ESCAPE)) CloseCurrentPopup();

		PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(GetStyle().ItemSpacing.x, GetStyle().ItemSpacing.x));

		float cell = (GetContentRegionAvail().x - (GetStyle().ItemSpacing.x*19)) / 10;
		int row = 0;
		int col = 0;

		auto cr = [&]() {
			if (row++ > 0 && col > 0) {
				NewLine();
				col = 0;
			}
		};

		auto next = [&]() {
			if (col++ == 10) {
				NewLine();
				col = 1;
			}
		};

		for (auto category: Item::display) {
			for (auto group: category->display) {
				cr();
				for (auto item: group->display) {
					next();
					if (itemIconButton(item, cell)) {
						signalPicked = Signal::Key(item);
						chosen = true;
						CloseCurrentPopup();
					}
					if (IsItemHovered()) {
						tip(item->title.c_str());
					}
					SameLine();
				}
			}
		}

		minivec<Fluid*> fluids;
		for (auto& [_,fluid]: Fluid::names) fluids.push(fluid);
		std::sort(fluids.begin(), fluids.end(), Fluid::sort);

		cr();

		for (auto fluid: fluids) {
			next();
			if (fluidIconButton(fluid, cell)) {
				signalPicked = Signal::Key(fluid);
				chosen = true;
				CloseCurrentPopup();
			}
			if (IsItemHovered()) {
				tip(fluid->title.c_str());
			}
			SameLine();
		}

		auto button = ImVec2(
			cell + GetStyle().ItemSpacing.x,
			cell + GetStyle().ItemSpacing.y
		);

		if (metas) {
			cr();

			for (auto meta: std::vector<Signal::Meta>{
				Signal::Meta::Any,
				Signal::Meta::All,
				Signal::Meta::Now,
			}){
				next();

				auto mkey = Signal::Key(meta);
				if (Button(mkey.title().c_str(), button)) {
					signalPicked = mkey;
					chosen = true;
					CloseCurrentPopup();
				}
				SameLine();
			}
		}

		cr();
		Separator();

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
			next();

			auto lkey = Signal::Key(letter);
			if (Button(lkey.name().c_str(), button)) {
				signalPicked = lkey;
				chosen = true;
				CloseCurrentPopup();
			}
			SameLine();
		}

		cr();

		for (auto& label: Signal::labels) {
			if (label.drop) continue;

			next();

			auto lkey = Signal::Key(label.name);
			if (Button(lkey.title().c_str(), button)) {
				signalPicked = lkey;
				chosen = true;
				CloseCurrentPopup();
			}
			SameLine();
		}

		cr();
		Separator();

		if (Button(" Clear Signal ", ImVec2(0,button.y))) {
			signalPicked = Signal::Key();
			chosen = true;
			CloseCurrentPopup();
		}

		PopStyleVar(1);
		EndPopup();
	}

	return chosen;
}

bool Popup::tagPicker(bool open, const std::set<std::string>& tags, bool create) {

	if (open) {
		OpenPopup("##tag-picker");
		tagEdit[0] = 0;
	}

	w = (float)Config::height(0.375f);
	h = w*0.6;

	const ImVec2 size = {
		(float)w,(float)h
	};

//	const ImVec2 pos = {
//		((float)Config::window.width-size.x)/2.0f,
//		((float)Config::window.height-size.y)/2.0f
//	};

	SetNextWindowSize(size, ImGuiCond_Always);
	SetNextWindowPos(GetMousePos(), ImGuiCond_Appearing);

	bool chosen = false;
	tagPicked.clear();

	if (BeginPopup("##tag-picker")) {
		if (!open && scene.keyReleased(SDLK_ESCAPE)) CloseCurrentPopup();

		if (create && BeginTable("##tag-new-table", 2)) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);

			TableNextColumn();
				SetNextItemWidth(-1);
				if (open) SetKeyboardFocusHere();
				bool ok = InputTextWithHint("##tag-new", "(new tag)", tagEdit, sizeof(tagEdit), ImGuiInputTextFlags_EnterReturnsTrue);
				inputFocused = IsItemFocused();

			TableNextColumn();
				if (Button(fmtc(" %s ##tag-save", ICON_FA_FLOPPY_O)) || ok) {
					tagPicked = std::string(tagEdit);
					CloseCurrentPopup();
					return tagPicked.size() > 0;
				}
			EndTable();
		}

		int i = 0;
		for (auto tag: tags) {
			if (ButtonStrip(i++, fmtc(" %s ", tag))) {
				tagPicked = tag;
				CloseCurrentPopup();
				return tagPicked.size() > 0;
			}
		}

		EndPopup();
	}

	return chosen;
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
	return tipBegin(IsItemHovered());
}

bool Popup::tipBegin(bool state) {
	if (state) {
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

