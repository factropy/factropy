#include "common.h"
#include "config.h"
#include "popup.h"
#include "gui.h"

#include "../imgui/setup.h"

using namespace ImGui;

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

