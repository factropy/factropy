#include "common.h"
#include "config.h"
#include "popup.h"

#include "../imgui/setup.h"

using namespace ImGui;

PlanPopup::PlanPopup() : Popup() {
}

PlanPopup::~PlanPopup() {
}

void PlanPopup::draw() {
	bool showing = true;

	Sim::locked([&]() {
		Plan::gc();

		bool pickTagForNow = false;
		bool pickTagFilterNow = false;

		medium();
		Begin("Blueprints##blueprints", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

		float button = CalcTextSize(ICON_FA_FOLDER_OPEN).x*1.5;

		auto drawPlanIcons = [&](auto plan) {
			struct Bucket {
				Spec* spec = nullptr;
				uint count = 0;
			};

			minimap<Bucket,&Bucket::spec> buckets;
			for (auto ge: plan->entities) buckets[ge->spec].count++;

			minivec<Spec*> specs = buckets.keys();
			std::sort(specs.begin(), specs.end(), [&](auto a, auto b) {
				return buckets[a].count < buckets[b].count;
			});

			for (auto spec: specs) {
				specIcon(spec);
				SameLine();
				Print(fmtc("(%u)", buckets[spec].count));
				SameLine();
			}
			if (specs.size()) NewLine();
		};

		auto drawPlanTags = [&](auto plan) {
			int i = 0;
			if (ButtonStrip(i++, fmtc(" + Tag ##add-tag-%u", plan->id))) {
				pickTagFor = plan->id;
				pickTagForNow = true;
				pickTagFilter = false;
			}
			for (auto it = plan->tags.begin(); it != plan->tags.end(); ) {
				auto tag = *it;
				if (ButtonStrip(i++, fmtc(" %s ##tag-%u-%s", tag, plan->id, tag)))
					it = plan->tags.erase(it); else ++it;
				if (IsItemHovered()) tip(fmtc("Drop tag '%s'", tag));
			}
		};

		auto drawPlanPerm = [&](auto plan) {
			if (BeginTable(fmtc("##plan-%u", plan->id), 4)) {
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);

				TableNextColumn();
					if (Button(fmtc("%s##drop-%u", ICON_FA_TRASH_O, plan->id), ImVec2(button,0))) {
						plan->save = false;
						plan->touch();
					}

				TableNextColumn();
					if (rename.active && rename.from == plan->id) {
						auto rname = std::string(rename.edit);

						if (Button(fmtc("%s##rename-%u", ICON_FA_FLOPPY_O, plan->id), ImVec2(button,0))) {
							plan->title = rname;
							rename.active = false;
							rename.from = 0;
						}
					}
					else {
						if (Button(fmtc("%s##rename-%u", ICON_FA_PENCIL_SQUARE_O, plan->id), ImVec2(button,0))) {
							rename.active = true;
							rename.from = plan->id;
							snprintf(rename.edit, sizeof(rename.edit), "%s", plan->title.c_str());
						}
					}
					if (IsItemHovered()) tip("Rename");

				TableNextColumn();
					if (rename.active && rename.from == plan->id) {
						SetNextItemWidth(-1);
						bool ok = InputTextWithHint("##rename", plan->title.c_str(), rename.edit, sizeof(rename.edit), ImGuiInputTextFlags_EnterReturnsTrue);

						if (ok) {
							auto rname = std::string(rename.edit);
							plan->title = rname;
							rename.active = false;
							rename.from = 0;
						}
					}
					else {
						if (!plan->title.size()) {
							Print(plan->config ? "(untitled)": "(pipette)");
						}
						else {
							Print(plan->title.c_str());
						}
					}

				TableNextColumn();
					if (Button(fmtc("%s##use-%u", ICON_FA_CLIPBOARD, plan->id), ImVec2(button,0))) {
						scene.planPush(plan);
						show(false);
					}
					if (IsItemHovered()) tip("Paste");

				EndTable();
			}

			PushFont(Config::sidebar.font.imgui);
			drawPlanTags(plan);
			drawPlanIcons(plan);
			PopFont();
			SpacingV();
			SpacingV();
			Separator();
			SpacingV();
			SpacingV();
		};

		auto drawPlanTemp = [&](auto plan) {
			if (BeginTable(fmtc("##plan-%u", plan->id), 4)) {
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
				TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);

				TableNextColumn();
					if (Button(fmtc("%s##save-%u", ICON_FA_FLOPPY_O, plan->id), ImVec2(button,0))) {
						plan->save = true;
					}
					if (IsItemHovered()) tip("Save");

				TableNextColumn();
					if (rename.active && rename.from == plan->id) {
						auto rname = std::string(rename.edit);

						if (Button(fmtc("%s##rename-%u", ICON_FA_FLOPPY_O, plan->id), ImVec2(button,0))) {
							plan->title = rname;
							rename.active = false;
							rename.from = 0;
						}
					}
					else {
						if (Button(fmtc("%s##rename-%u", ICON_FA_PENCIL_SQUARE_O, plan->id), ImVec2(button,0))) {
							rename.active = true;
							rename.from = plan->id;
							snprintf(rename.edit, sizeof(rename.edit), "%s", plan->title.c_str());
						}
					}
					if (IsItemHovered()) tip("Rename");

				TableNextColumn();
					if (rename.active && rename.from == plan->id) {
						SetNextItemWidth(-1);
						bool ok = InputTextWithHint("##rename", plan->title.c_str(), rename.edit, sizeof(rename.edit), ImGuiInputTextFlags_EnterReturnsTrue);

						if (ok) {
							auto rname = std::string(rename.edit);
							plan->title = rname;
							rename.active = false;
							rename.from = 0;
						}
					}
					else {
						if (!plan->title.size()) {
							Print(plan->config ? "(untitled)": "(pipette)");
						}
						else {
							Print(plan->title.c_str());
						}
					}

				TableNextColumn();
					if (Button(fmtc("%s##paste-%u", ICON_FA_CLIPBOARD, plan->id), ImVec2(button,0))) {
						scene.planPush(plan);
						show(false);
					}
					if (IsItemHovered()) tip("Paste");

				EndTable();
			}

			PushFont(Config::sidebar.font.imgui);
			drawPlanIcons(plan);
			PopFont();
			SpacingV();
			SpacingV();
			Separator();
			SpacingV();
		};

		std::vector<Plan*> plans = {Plan::all.begin(), Plan::all.end()};
		std::sort(plans.begin(), plans.end(), [](const auto a, const auto b) {
			return a->title < b->title || (a->title == b->title && a->id < b->id);
		});

		auto latest = Plan::latest();
		if (latest && !latest->save) {
			Notice("Clipboard");
			drawPlanTemp(latest);
		}

		SpacingV();

		int i = 0;
		pickTagFilterNow = ButtonStrip(i++, fmtc(" + Filter ##add-filter"));
		if (pickTagFilterNow) { pickTagFilter = true; pickTagFor = 0; }

		for (auto tag: showTags) {
			if (ButtonStrip(i++, fmtc(" %s ##filter-tag-%s", tag, tag))) showTags.erase(tag);
		}

		SpacingV();
		SpacingV();
		Separator();
		SpacingV();
		SpacingV();

		BeginGroup();
			for (auto plan: plans) {
				if (!plan->save) continue;
				bool show = true;
				if (showTags.size()) {
					for (auto tag: showTags)
						if (!plan->tags.count(tag)) show = false;
				}
				if (show) drawPlanPerm(plan);
			}
		EndGroup();

		std::set<std::string> tags;
		for (auto plan: Plan::all) for (auto tag: plan->tags) tags.insert(tag);

		if (pickTagFor && tagPicker(pickTagForNow, tags, true)) {
			for (auto plan: Plan::all)
				if (plan->id == pickTagFor) plan->tags.insert(tagPicked);
			pickTagFor = 0;
		}

		if (pickTagFilter && tagPicker(pickTagFilterNow, tags, false)) {
			if (pickTagFilter) showTags.insert(tagPicked);
			pickTagFilter = false;
		}

		mouseOver = IsWindowHovered();
		subpopup = IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup);
		End();
		if (visible) show(showing);
	});
}

