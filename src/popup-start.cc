#include "common.h"
#include "config.h"
#include "popup.h"

#include "../imgui/setup.h"

#include <filesystem>
#include <fstream>
#include <chrono>

namespace {
	template <typename TP>
	std::time_t to_time_t(TP tp)
	{
		using namespace std::chrono;
		auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
		return system_clock::to_time_t(sctp);
	}

	uint64_t todayMS() {
		auto now = std::chrono::system_clock::now();
		time_t tnow = std::chrono::system_clock::to_time_t(now);
		tm *date = std::localtime(&tnow);
		date->tm_hour = 0;
		date->tm_min = 0;
		date->tm_sec = 0;
		auto midnight = std::chrono::system_clock::from_time_t(std::mktime(date));
		return std::chrono::duration_cast<std::chrono::milliseconds>(now-midnight).count();
	}
}

using namespace ImGui;

StartScreen::StartScreen() : Popup() {
	banner = loadTexture("assets/banner.png");

	for (int i = 1; ; i++) {
		snprintf(name, sizeof(name), "game%d", i);
		auto str = std::string(name);
		if (Config::saveNameOk(name) && Config::saveNameFree(name)) break;
	}

	snprintf(seed, sizeof(seed), "%ld", Config::mode.seeds[todayMS() % Config::mode.seeds.size()]);

	try {
		for (auto& entry: std::filesystem::directory_iterator{Config::dataPath("saves")}) {
			if (!std::filesystem::is_directory(entry)) continue;
			auto simjson = fmt("%s/sim.json", entry.path().string());
			if (!std::filesystem::exists(simjson)) continue;
			screen = Screen::Load;
			break;
		}
	}
	catch (std::filesystem::filesystem_error& e) {
		infof("%s", e.what());
	}
}

StartScreen::~StartScreen() {
	freeTexture(banner);
}

void StartScreen::draw() {
	small();
	Begin("##start-screen", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

	PushID("start-screen");

	ImageBanner(banner.id, banner.w, banner.h);

	SetCursorPos(ImVec2(GetCursorPos().x, GetCursorPos().y + GetStyle().ItemSpacing.y/2));
	Notice(Config::version.text, false);

	if (BeginTable("##layout", 2)) {
		TableSetupColumn("##layout-left", ImGuiTableColumnFlags_WidthFixed, relativeWidth(0.25));
		TableSetupColumn("##layout-right", ImGuiTableColumnFlags_WidthStretch, relativeWidth(0.25));

		TableNextColumn();
			BeginChild("##layout-left-group");
			float button = (GetContentRegionAvail().y - GetStyle().ItemSpacing.y*3) / 4;

			PushStyleColor(ImGuiCol_Button, GetColorU32(screen == Screen::Load ? ImGuiCol_ButtonActive: ImGuiCol_Button));
			if (Button(" Load Game ", ImVec2(-1,button))) screen = Screen::Load;
			PopStyleColor();

			PushStyleColor(ImGuiCol_Button, GetColorU32(screen == Screen::New ? ImGuiCol_ButtonActive: ImGuiCol_Button));
			if (Button(" New Game ", ImVec2(-1,button))) screen = Screen::New;
			PopStyleColor();

			PushStyleColor(ImGuiCol_Button, GetColorU32(screen == Screen::Video ? ImGuiCol_ButtonActive: ImGuiCol_Button));
			if (Button(" Video ", ImVec2(-1,button))) screen = Screen::Video;
			PopStyleColor();

			if (Button(" Exit ", ImVec2(-1,button))) exit(0);

			EndChild();

		TableNextColumn();
			BeginChild("##layout-right-group");

			switch (screen) {
				case Screen::New: {
					drawNew();
					break;
				}
				case Screen::Load: {
					drawLoad();
					break;
				}
				case Screen::Video: {
					drawVideo();
					break;
				}
			}

			EndChild();

		EndTable();
	}

	PopID();

	mouseOver = IsWindowHovered();
	subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
	End();
}

void StartScreen::drawNew() {
	bool ok = true;

	Separator();

	Print("Name"); SameLine();
	PushFont(Config::sidebar.font.imgui);
	PrintRight("_- a-z A-Z 0-9");
	PopFont();

	SetNextItemWidth(-1);
	InputText("##name", name, sizeof(name));
	SpacingV();

	if (!Config::saveNameOk(name)) {
		Warning("Invalid save name: _- a-z A-Z 0-9");
		ok = false;
	}

	if (std::string(name).size() && !Config::saveNameFree(std::string(name))) {
		Warning(fmtc("Save '%s' already exists.", name));
		ok = false;
	}

	Separator();

	Print("Seed"); SameLine();
	PushFont(Config::sidebar.font.imgui);
	PrintRight("seed > 0");
	PopFont();

	SetNextItemWidth(-1);
	InputText("##seed", seed, sizeof(seed));
	SpacingV();

	if (!strtoul(seed, nullptr, 10)) {
		Warning(fmtc("Seed invalid.", name));
		ok = false;
	}

	PushFont(Config::sidebar.font.imgui);
	if (Button(" Random ")) {
		snprintf(seed, sizeof(seed), "%ld", todayMS());
	}
	if (IsItemHovered()) tip(
		"Could be easy. Could be hard."
		"\n\nIf you find a great seed please jump on Discord and"
		" let everyone know! It will potentially be included in"
		" the curated seed list."
	);

	SameLine();
	if (Button(" Curated ")) {
		snprintf(seed, sizeof(seed), "%ld", Config::mode.seeds[todayMS() % Config::mode.seeds.size()]);
	}
	if (IsItemHovered()) tip(
		"Choose from a list of curated seeds with decent start locations."
	);
	PopFont();
	SpacingV();

	Separator();
	SpacingV();

	if (!ok) PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));

	if (Button(" Start Game ") && ok) {
		Config::mode.fresh = true;
		Config::mode.freshSeed = strtoul(seed, nullptr, 10);
		Config::mode.saveName = std::string(name);
	}

	if (!ok) PopStyleColor(1);
}

void StartScreen::drawLoad() {
	struct Save {
		std::string name;
		char date[32];
		char time[32];
		uint64_t stamp = 0;
	};

	std::vector<Save> saves;

	try {
		for (auto& entry: std::filesystem::directory_iterator{Config::dataPath("saves")}) {
			if (!std::filesystem::is_directory(entry)) continue;
			auto simjson = fmt("%s/sim.json", entry.path().string());
			if (!std::filesystem::exists(simjson)) continue;
			std::string name = entry.path().filename().string();
			std::time_t cftime = to_time_t(std::filesystem::last_write_time(simjson));
			auto& save = saves.emplace_back();
			save.name = name;
			save.stamp = cftime;
			std::strftime(save.date, sizeof(save.date), "%b %d", std::localtime(&cftime));
			std::strftime(save.time, sizeof(save.time), "%H:%M", std::localtime(&cftime));
		}
	}
	catch (std::filesystem::filesystem_error& e) {
		Alert("Error listing saves");
		Print(fmtc("%s", e.what()));
		saves.clear();
	}

	std::sort(saves.begin(), saves.end(), [](const auto& a, const auto& b) { return a.stamp > b.stamp; });
	games.resize(saves.size());

	Separator();

	if (!saves.size()) {
		Print("(no saved games)");
	}

	float button = CalcTextSize(ICON_FA_FOLDER_OPEN).x*1.5;
	float width = GetContentRegionAvail().x*0.2;

	for (int i = 0, l = saves.size(); i < l; i++) {
		auto name = saves[i].name;
		if (BeginTable(fmtc("##load-list-%d", i), 5)) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, width);
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);
			TableNextRow();

			TableNextColumn();
				Checkbox(fmtc("##check-%s", name), &games[i]);

			TableNextColumn();
				if (Button(fmtc("%s##rename-%d", ICON_FA_PENCIL_SQUARE_O, i), ImVec2(button,0))) {
					if (rename.active && rename.from == name) {
						rename.active = false;
					}
					else {
						rename.active = true;
						rename.from = name;
						snprintf(rename.edit, sizeof(rename.edit), "%s", name.c_str());
					}
				}
				if (IsItemHovered()) tip("Rename");

			TableNextColumn();
				if (rename.active && rename.from == name) {
					auto rname = std::string(rename.edit);

					bool ok = rname.size() && Config::saveNameOk(rname);
					bool dup = rname.size() && !Config::saveNameFree(rname);

					dup = dup || rname.find("autosave") == 0;

					PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0,0));
					if (BeginTable("##rename-inline", 2)) {
						TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
						TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);

						TableNextColumn();
							SetNextItemWidth(-1);
							InputTextWithHint("##rename", rename.from.c_str(), rename.edit, sizeof(rename.edit));

						TableNextColumn();
							if (Button(fmtc("%s##rename-save", ICON_FA_FLOPPY_O), ImVec2(button,0)) && ok && !dup) {
								Config::saveRename(name, rname);
							}

						EndTable();
					}
					PopStyleVar(1);

					if (!ok && rname.size()) Warning("Invalid name: _- a-z A-Z 0-9");
					if (dup && rname != name) Warning("Duplicate name");
				}
				else {
					Print(name.c_str());
				}

			TableNextColumn();
				PushFont(Config::sidebar.font.imgui);
				PrintRight(fmtc("%s %s", saves[i].date, saves[i].time));
				PopFont();

			TableNextColumn();
				if (Button(fmtc("%s##open-%d", ICON_FA_FOLDER_OPEN, i), ImVec2(button,0))) {
					Config::mode.load = true;
					Config::mode.saveName = name;
				}
				if (IsItemHovered()) tip("Open");

			EndTable();
		}
		Separator();
	}

	int selected = 0; for (auto flag: games) if (flag) selected++;

	if (!selected) {
		PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
	}

	SpacingV();
	if (Button(" Delete Selected ") && selected) {
		for (int i = 0, l = saves.size(); i < l; i++) {
			if (games[i]) Config::saveDelete(saves[i].name);
		}
		games.clear();
	}

	if (!selected) {
		PopStyleColor(1);
	}
}

void StartScreen::drawVideo() {
	if (Checkbox("VSYNC", &Config::window.vsync)) {
		// controlled by main.cc menu() and game()
		Config::save();
	}

	if (Checkbox("MSAA", &Config::window.antialias)) {
		// controlled by main.cc game()
		Config::save();
	}
	if (IsItemHovered()) tip(
		"Normal GL_MULTISAMPLE. Disable if better anti-aliasing is done via GPU driver settings."
	);

	if (Checkbox("Full screen", &Config::window.fullscreen)) {
		SDL_SetWindowFullscreen(sdlWindow(), Config::window.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP: 0);
		Config::save();
	}
	if (IsItemHovered()) tip(
		"Borderless window."
	);

	if (!Config::window.fullscreen) {
		if (Checkbox("Resizable", &Config::window.resizable)) {
			SDL_SetWindowResizable(sdlWindow(), Config::window.resizable ? SDL_TRUE: SDL_FALSE);
			Config::save();
		}
		if (IsItemHovered()) tip(
			"The game is designed for 16:9 but any wide ratio should work."
			"\n\nTall ratios may result in HUD and toolbar clipping in-game."
			"\n\nAdjust horizontal field-of-view (FOV) via the in-game graphics tab."
		);
	}
}
