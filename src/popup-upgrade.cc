#include "common.h"
#include "config.h"
#include "popup.h"

#include "../imgui/setup.h"

using namespace ImGui;

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

