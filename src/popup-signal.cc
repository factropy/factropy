#include "common.h"
#include "config.h"
#include "popup.h"

#include "../imgui/setup.h"

using namespace ImGui;

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

