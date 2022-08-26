#include "common.h"
#include "config.h"
#include "popup.h"

#include "../imgui/setup.h"

using namespace ImGui;

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
				Image(specIconChoose(en.spec, iconSize), ImVec2(iconPix, iconPix), ImVec2(0, 1), ImVec2(1, 0));

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

