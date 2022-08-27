#include "common.h"
#include "config.h"
#include "popup.h"

#include "../imgui/setup.h"

using namespace ImGui;

VehiclePopup::VehiclePopup() : Popup() {
}

VehiclePopup::~VehiclePopup() {
}

void VehiclePopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		big();
		Begin("Vehicles", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		float spacing = GetStyle().ItemSpacing.x;

		int columns = 8;
		float column = relativeWidth(1.0/columns) - spacing + (spacing/columns);

		auto trafficLights = [&](auto& cart) {
			auto light = [&](ImU32 color) {
				PushStyleColor(ImGuiCol_Text, color);
				Print(ICON_FA_CIRCLE);
				PopStyleColor();
			};

			auto off = [&]() {
				PushStyleColor(ImGuiCol_Text, ImColorSRGB(0x999999ff));
				Print(ICON_FA_CIRCLE_O);
				PopStyleColor();
			};

			auto red = [&]() {
				light(ImColorSRGB(0xdd0000ff));
			};

			auto green = [&]() {
				light(ImColorSRGB(0x00cc00ff));
			};

			auto orange = [&]() {
				light(ImColorSRGB(0xdd8800ff));
			};

			auto stop = [&]() {
				red(); off(); off();
			};

			auto wait = [&]() {
				off(); orange(); off();
			};

			auto go = [&]() {
				off(); off(); green();
			};

			if (cart.state == Cart::State::Travel && cart.halt) wait();
			else if (cart.state == Cart::State::Travel) go();
			else stop();
		};

		int i = 0;
		for (auto& cart: Cart::all) {
			auto& en = Entity::get(cart.id);
			if (en.isGhost()) continue;

			if (i) { if (i%columns == 0) NewLine(); else SameLine(); }

			if (BeginChild(fmtc("##cart-%u", cart.id), ImVec2(column, column))) {

				if (!en.isEnabled()) Warning("Disabled");
				else if (cart.blocked) Alert("Blocked");
				else if (cart.lost) Warning("Lost");
				else if (cart.state == Cart::State::Travel && cart.fueled < 0.99) Warning("Low power");
				else if (cart.state == Cart::State::Travel && cart.halt) Section("Queuing");
				else if (cart.state == Cart::State::Travel) Section("Moving");
				else Section("Stopped");

				PushFont(Config::sidebar.font.imgui);

				if (BeginTable(fmtc("##cart-inner-%u", cart.id), 2)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

					TableNextColumn();
						trafficLights(cart);

					TableNextColumn();
						specIcon(en.spec, Config::toolbar.icon.sizes[iconTier(1024)]);

					EndTable();
				}

				Print("Signal"); SameLine();
				signalKeyIcon(cart.signal.key);

				if (en.spec->store) {
					auto& store = en.store();
					if (!store.isEmpty()) {
						Print("Contents"); SameLine();
						stacksStrip(store.stacks);
					}
				}

				PopFont();
			}
			EndChild();

			if (IsItemClicked()) {
				scene.view(en.pos(), 75);
				show(false);
			}

			i++;
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}


