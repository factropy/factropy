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

		Plan* currentPlan = nullptr;
		bool pickTagForNow = false;
		bool pickTagFilterNow = false;

		big();
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

		if (BeginTable("##two-pane", 2)) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, relativeWidth(0.4));
			TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

			TableNextColumn(); {
				std::vector<Plan*> plans = {Plan::all.begin(), Plan::all.end()};
				std::sort(plans.begin(), plans.end(), [](const auto a, const auto b) {
					return a->title < b->title || (a->title == b->title && a->id < b->id);
				});

				auto latest = Plan::latest();
				if (latest && !latest->save) {
					BeginGroup();
					Notice("Clipboard");
					drawPlanTemp(latest);
					EndGroup();
					if (IsItemClicked()) current = latest->id;
					if (current == latest->id) currentPlan = latest;
				}

				SpacingV();

				int i = 0;
				pickTagFilterNow = ButtonStrip(i++, fmtc(" + Filter ##add-filter"));
				if (pickTagFilterNow) { pickTagFilter = true; pickTagFor = 0; }

				for (auto it = showTags.begin(); it != showTags.end(); ) {
					if (ButtonStrip(i++, fmtc(" %s ##filter-tag-%s", *it, *it)))
						it = showTags.erase(it); else ++it;
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
						if (show) {
							BeginGroup();
							drawPlanPerm(plan);
							EndGroup();
							if (IsItemClicked()) current = plan->id;
							if (current == plan->id) currentPlan = plan;
						}
					}
				EndGroup();
			}

			TableNextColumn(); {
				if (currentPlan) preview(currentPlan, GetContentRegionAvail());
			}

			EndTable();
		}

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

void PlanPopup::preview(Plan* plan, ImVec2 size) {
	auto cursor = GetCursorPos();
	SetCursorPos(ImVec2(cursor.x, cursor.y + size.y));

	auto origin = cursor;
	origin.x += GetWindowPos().x - GetScrollX();
	origin.y += GetWindowPos().y - GetScrollY();

	auto topLeft = ImVec2(origin.x, origin.y);
	auto bottomRight = ImVec2(origin.x + size.x, origin.y + size.y);

	if (!IsRectVisible(topLeft, bottomRight)) return;
	GetWindowDrawList()->PushClipRect(topLeft, bottomRight, true);

	struct {
		float x = 0;
		float y = 0;
	} range;

	for (uint i = 0, l = plan->entities.size(); i < l; i++) {
		auto ge = plan->entities[i];
		auto offset = plan->offsets[i];
		range.x = std::max(range.x, (float)(std::abs(offset.x) + ge->spec->collision.w*0.5));
		range.y = std::max(range.y, (float)(std::abs(offset.z) + ge->spec->collision.d*0.5));
	}

	float d = std::max(range.x, range.y)*2;
	float s = std::min(size.x, size.y);
	float r = (s/d)*0.8;

	auto tile = ImVec2(r,r);
	auto pad = tile.x/8;

	auto centroid = ImVec2(origin.x + size.x/2, origin.y + size.y/2);

	auto x = [&](float n) { return n*tile.x; };
	auto y = [&](float n) { return n*tile.y; };

	auto maxIcon = Config::toolbar.icon.sizes[iconTier(1024)];

	for (uint i = 0, l = plan->entities.size(); i < l; i++) {
		auto ge = plan->entities[i];
		auto offset = plan->offsets[i];
		auto box = ge->spec->box(offset, ge->dir(), ge->spec->collision);
		auto p0 = ImVec2(centroid.x + x(box.x) - x(box.w*0.5) + pad, centroid.y + y(box.z) - y(box.d*0.5) + pad);
		auto p1 = ImVec2(centroid.x + x(box.x) + x(box.w*0.5) - pad, centroid.y + y(box.z) + y(box.d*0.5) - pad);
		GetWindowDrawList()->AddRect(p0, p1, GetColorU32(Color(0xffffffff)));
		auto icon = std::min(maxIcon, std::min(x(box.w), y(box.d))) * 0.9;
		p0 = ImVec2(centroid.x + x(box.x) - icon*0.5, centroid.y + y(box.z) - icon*0.5);
		p1 = ImVec2(centroid.x + x(box.x) + icon*0.5, centroid.y + y(box.z) + icon*0.5);
		GetWindowDrawList()->AddImage(specIconChoose(ge->spec, tile.x), p0, p1, ImVec2(0, 1), ImVec2(1, 0));
	}

	auto h = GetFontSize();
	auto w = CalcTextSize("W").x;

	GetWindowDrawList()->AddText(ImVec2(centroid.x, centroid.y - size.y*0.5), GetColorU32(Color(0xffffffff)), "N");
	GetWindowDrawList()->AddText(ImVec2(centroid.x, centroid.y + size.y*0.5 - h), GetColorU32(Color(0xffffffff)), "S");
	GetWindowDrawList()->AddText(ImVec2(centroid.x - size.x*0.5 + w*0.5, centroid.y - h*0.5), GetColorU32(Color(0xffffffff)), "W");
	GetWindowDrawList()->AddText(ImVec2(centroid.x + size.x*0.5 - w, centroid.y - h*0.5), GetColorU32(Color(0xffffffff)), "E");

	GetWindowDrawList()->PopClipRect();
}
