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
#include "toolbar.h"

void Toolbar::add(Spec* spec) {
	specs.insert(spec);
}

void Toolbar::drop(Spec* spec) {
	specs.erase(spec);
}

bool Toolbar::has(Spec* spec) {
	return specs.count(spec) > 0;
}

void Toolbar::draw() {
	using namespace ImGui;

	std::string tip;
	SetNextWindowSize(ImVec2(-1,-1), ImGuiCond_Always);

	SetNextWindowBgAlpha(GetStyle().Colors[ImGuiCol_PopupBg].w * 0.75f);
	Begin("##toolbar", nullptr, flags);
		PushFont(Config::toolbar.font.imgui);

		std::vector<Spec*> buttons = {specs.begin(), specs.end()};

		std::sort(buttons.begin(), buttons.end(), [](auto a, auto b) {
			return a->toolbar < b->toolbar;
		});

		int iconSize = Config::toolbar.icon.size;
		float iconPix = Config::toolbar.icon.sizes[iconSize];

		int i = 0;

		if (scene.directing) {
			PushStyleColor(ImGuiCol_Button, Color(0xffa50066).gamma());
			if (ImageButton((ImTextureID)scene.iconTextures[scene.directing->spec][iconSize], ImVec2(iconPix, iconPix), ImVec2(0, 1), ImVec2(1, 0))) {
				scene.view(scene.directing->pos());
			}
			if (IsItemHovered()) {
				tip = fmt("Controlling: %s", scene.directing->spec->title);
			}
			PopStyleColor(1);
			i++;
		}

		for (auto spec: buttons) {
			if (!spec->licensed) continue;
			if (i) SameLine();
			if (ImageButton((ImTextureID)scene.iconTextures[spec][iconSize], ImVec2(iconPix, iconPix), ImVec2(0, 1), ImVec2(1, 0))) {
				scene.build(spec);
			}
			if (IsItemHovered()) {
				tip = spec->title;
			}
			i++;
		}

		PopFont();

		size = GetWindowSize();
		SetWindowPos(ImVec2(((float)Config::window.width - size.x)/2.0f, (float)Config::window.height - size.y), ImGuiCond_Always);
	End();

	if (tip.size()) {
		auto& style = GetStyle();
		PushFont(Config::sidebar.font.imgui);
		ImVec2 tipPos = ImVec2(
			(float)Config::window.width - ((float)Config::window.width/2.0f) - (CalcTextSize(tip.c_str()).x/2.0f) - style.WindowPadding.y,
			(float)Config::window.height - size.y - GetFontSize() - (style.WindowPadding.y*2.0f)
		);
		SetNextWindowPos(tipPos);
		SetNextWindowBgAlpha(style.Colors[ImGuiCol_PopupBg].w * 1.0f);

	    ImGuiWindowFlags flags = ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
		Begin("##toolbar-tip", nullptr, flags);
		PushStyleColor(ImGuiCol_Text, ImColorSRGB(0xffffccff));
		Print(tip.c_str());
		PopStyleColor(1);
		End();
		PopFont();
	}
}

