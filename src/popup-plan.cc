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
		uint scroll = 0;

		big();
		Begin("Blueprints##blueprints", &showing, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

		if (IsWindowAppearing()) scroll = current;
		float button = CalcTextSize(ICON_FA_FOLDER_OPEN).x*1.5;

		auto drawPlanEntities = [&](auto plan) {
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

			int i = 0;
			for (auto spec: specs) {
				specStrip(i++, spec, buckets[spec].count);
			}
		};

		auto drawPlanMaterials = [&](auto plan) {
			stacksStrip(plan->materials());
		};

		auto drawPlanTags = [&](auto plan) {
			int i = 0;
			if (ButtonStrip(i++, fmtc(" + Tag ##add-tag-%u", plan->id))) {
				pickTagFor = plan->id;
				pickTagForNow = true;
				pickTagFilter = false;
			}
			if (IsItemHovered()) tip("Add tag to plan");
			for (auto it = plan->tags.begin(); it != plan->tags.end(); ) {
				auto tag = *it;
				if (ButtonStrip(i++, fmtc(" %s ##tag-%u-%s", tag, plan->id, tag)))
					it = plan->tags.erase(it); else ++it;
				if (IsItemHovered()) tip(fmtc("Drop tag '%s'", tag));
			}
		};

		auto drawPlan = [&](auto plan) {
			auto start = GetCursorAbs();

			bool renameNow = false;

			float margin = GetStyle().ItemSpacing.x*2;

			GetWindowDrawList()->ChannelsSplit(2);
			GetWindowDrawList()->ChannelsSetCurrent(1);

			SetCursorPos(ImVec2(GetCursorPos().x + margin, GetCursorPos().y + margin - GetStyle().CellPadding.y));

			if (BeginTable(fmtc("##plan-%u", plan->id), 1, 0, ImVec2(GetContentRegionAvail().x - margin, 0))) {

				TableNextColumn();
				if (BeginTable(fmtc("##plan-inner-%u", plan->id), 4)) {
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
					TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, button);

					TableNextColumn();
						if (Button(fmtc("%s##drop-%u", plan->save ? ICON_FA_TRASH_O: ICON_FA_FLOPPY_O, plan->id), ImVec2(button,0))) {
							scroll = plan->id;
							plan->save = !plan->save;
							Plan::saveAll();
						}
						if (IsItemHovered() && plan->save) tip("Remove from library");
						if (IsItemHovered() && !plan->save) tip("Add to library");

					TableNextColumn();
						if (rename.active && rename.from == plan->id) {
							auto rname = std::string(rename.edit);

							if (Button(fmtc("%s##rename-%u", ICON_FA_CHECK, plan->id), ImVec2(button,0))) {
								plan->title = rname;
								rename.active = false;
								rename.from = 0;
								scroll = !plan->save ? plan->id: 0;
								current = plan->id;
							}
							if (IsItemHovered()) tip("Save");
						}
						else {
							if (Button(fmtc("%s##rename-%u", ICON_FA_PENCIL_SQUARE_O, plan->id), ImVec2(button,0))) {
								rename.active = true;
								rename.from = plan->id;
								snprintf(rename.edit, sizeof(rename.edit), "%s", plan->title.c_str());
								renameNow = true;
							}
							if (IsItemHovered()) tip("Rename");
						}

					TableNextColumn();
						if (rename.active && rename.from == plan->id) {
							SetNextItemWidth(-1);
							if (renameNow) SetKeyboardFocusHere();
							bool ok = InputTextWithHint("##rename", plan->title.c_str(), rename.edit, sizeof(rename.edit), ImGuiInputTextFlags_EnterReturnsTrue);
							inputFocused = IsItemFocused();

							if (ok) {
								auto rname = std::string(rename.edit);
								plan->title = rname;
								rename.active = false;
								rename.from = 0;
								scroll = !plan->save ? plan->id: 0;
								current = plan->id;
							}
						}
						else {
							Print(!plan->title.size() ? "(untitled)": plan->title.c_str());
						}

					TableNextColumn();
						if (Button(fmtc("%s##use-%u", ICON_FA_CLIPBOARD, plan->id), ImVec2(button,0))) {
							scene.planPush(plan);
							show(false);
						}
						if (IsItemHovered()) tip("Paste");

					EndTable();
				}

				TableNextColumn(); {
					PushFont(Config::sidebar.font.imgui);
					drawPlanTags(plan);
					PushStyleColor(ImGuiCol_Text, GetColorU32(Color(0xeeeeeeff)));
					drawPlanEntities(plan);
					drawPlanMaterials(plan);
					PopStyleColor();
					PopFont();
				}

				EndTable();
			}

			SetCursorPos(ImVec2(GetCursorPos().x, GetCursorPos().y + margin*0.5));

			auto end = ImVec2(start.x + GetContentRegionAvail().x, GetCursorAbs().y);
			if (IsWindowHovered() && IsRectVisible(start, end) && IsMouseHoveringRect(start, end) && IsMouseClicked(ImGuiMouseButton_Left)) current = plan->id;

			GetWindowDrawList()->ChannelsSetCurrent(0);
			if (current == plan->id) {
				GetWindowDrawList()->AddRectFilled(start, end, GetColorU32(ImGuiCol_FrameBgActive), GetStyle().FrameRounding);

				GetWindowDrawList()->AddRectFilled(
					ImVec2(start.x + GetStyle().FrameRounding, start.y + GetStyle().FrameRounding),
					ImVec2(end.x - GetStyle().FrameRounding, end.y - GetStyle().FrameRounding),
					GetColorU32(Color(0x000000ff)),
					GetStyle().FrameRounding
				);
			}
			else {
				GetWindowDrawList()->AddRect(start, end, GetColorU32(ImGuiCol_FrameBg), GetStyle().FrameRounding);
			}
			GetWindowDrawList()->ChannelsSetCurrent(1);

			GetWindowDrawList()->ChannelsMerge();

			SpacingV();
			SpacingV();
			SpacingV();
		};

		std::vector<Plan*> listed;
		std::vector<Plan*> sorted = {Plan::all.begin(), Plan::all.end()};
		std::sort(sorted.begin(), sorted.end(), [](const auto a, const auto b) {
			return a->title < b->title || (a->title == b->title && a->id < b->id);
		});

		if (BeginTable("##two-pane", 2)) {
			TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, relativeWidth(0.4));
			TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);

			TableNextColumn(); {
				auto latest = Plan::clipboard;
				if (latest && !latest->save) {
					listed.push_back(latest);
					Notice("Clipboard");
					drawPlan(latest);
				}

				Header("Library");
				SpacingV();

				int i = 0;
				pickTagFilterNow = ButtonStrip(i++, fmtc(" + Filter ##add-filter"));
				if (IsItemHovered()) tip("Filter by tag");
				if (pickTagFilterNow) { pickTagFilter = true; pickTagFor = 0; }

				for (auto it = showTags.begin(); it != showTags.end(); ) {
					bool click = ButtonStrip(i++, fmtc(" %s ##filter-tag-%s", *it, *it));
					if (IsItemHovered()) tip(fmtc("Drop filter tag '%s'", *it));
					if (click) it = showTags.erase(it); else ++it;
				}

				SpacingV();
				SpacingV();

				BeginChild("##plans-saved");
					for (auto plan: sorted) {
						if (!plan->save) continue;
						bool show = true;
						if (showTags.size()) {
							for (auto tag: showTags)
								if (!plan->tags.count(tag)) show = false;
						}
						if (show) {
							listed.push_back(plan);
							drawPlan(plan);
						}
						if (scroll && plan->id == scroll) {
							SetScrollHereY();
						}
					}
				EndChild();
			}

			TableNextColumn(); {
				Plan* active = nullptr;

				for (auto plan: listed) {
					if (plan->id == current) active = plan;
				}

				if (!active && Plan::clipboard && !Plan::clipboard->save) {
					active = Plan::clipboard;
				}

				if (!active && listed.size()) {
					active = listed.front();
				}

				if (active) {
					current = active->id;
					preview(active, GetContentRegionAvail());
				}
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
	GetWindowDrawList()->AddRectFilled(topLeft, bottomRight, GetColorU32(Color(0x000000ff)));

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

	auto white = GetColorU32(Color(0xffffffff));
	auto offwhite = GetColorU32(Color(0xeeeeeeff));

	struct triangle {
		Point v0, v1, v2;
	};

	std::map<Point,triangle> chevrons;

	for (auto dir: std::array<Point,4>{Point::South,Point::West,Point::North,Point::East}) {
		auto bump = (Point::North*0.05).transform(dir.rotation());
		auto c0 = (Point::South*0.25).transform(dir.rotation());
		auto c1 = c0.transform(Mat4::rotateY(glm::radians(120.0)));
		auto c2 = c0.transform(Mat4::rotateY(glm::radians(240.0)));
		chevrons[dir] = {c0+bump,c1+bump,c2+bump};
	}

	auto chevron = [&](Point pos, Point dir) {
		auto it = chevrons.find(dir);
		if (it == chevrons.end()) return;
		auto v0 = it->second.v0 + pos;
		auto v1 = it->second.v1 + pos;
		auto v2 = it->second.v2 + pos;
		auto p0 = ImVec2(centroid.x + x(v0.x), centroid.y + y(v0.z));
		auto p1 = ImVec2(centroid.x + x(v1.x), centroid.y + y(v1.z));
		auto p2 = ImVec2(centroid.x + x(v2.x), centroid.y + y(v2.z));
		GetWindowDrawList()->AddTriangleFilled(p0, p1, p2, offwhite);
	};

	auto specIcon = [&](Spec* spec, ImVec2 p0, ImVec2 p1) {
		float w = p1.x-p0.x;
		float h = p1.y-p0.y;
		float ipix = std::min(w, h) * 0.8;
		float apix = std::min(ipix, Config::toolbar.icon.sizes[iconTier(ipix)]);
		auto pc = ImVec2(p0.x + w*0.5, p0.y + h*0.5);
		auto p0i = ImVec2(pc.x - apix*0.5, pc.y - apix*0.5);
		auto p1i = ImVec2(pc.x + apix*0.5, pc.y + apix*0.5);
		GetWindowDrawList()->AddImage(specIconChoose(spec, ipix), p0i, p1i, ImVec2(0, 1), ImVec2(1, 0));
	};

	for (uint i = 0, l = plan->entities.size(); i < l; i++) {
		auto ge = plan->entities[i];
		auto offset = plan->offsets[i];
		auto box = ge->spec->box(offset, ge->dir(), ge->spec->collision);
		auto p0 = ImVec2(centroid.x + x(box.x) - x(box.w*0.5) + pad, centroid.y + y(box.z) - y(box.d*0.5) + pad);
		auto p1 = ImVec2(centroid.x + x(box.x) + x(box.w*0.5) - pad, centroid.y + y(box.z) + y(box.d*0.5) - pad);
		GetWindowDrawList()->AddRect(p0, p1, white);

		if (ge->spec->powerpole) {
			for (uint ii = 0, ll = plan->entities.size(); ii < ll; ii++) {
				if (i == ii) continue;
				auto oe = plan->entities[ii];
				auto ooffset = plan->offsets[ii];
				if (!oe->spec->powerpole) continue;
				float d = offset.distance(ooffset);
				if (d < oe->spec->powerpoleRange+0.001 && d < ge->spec->powerpoleRange+0.001) {
					auto grey = GetColorU32(Color(0x999999ff));
					auto obox = oe->spec->box(ooffset, oe->dir(), oe->spec->collision);
					auto t0 = ImVec2(centroid.x + x(box.x), centroid.y + y(box.z));
					auto t1 = ImVec2(centroid.x + x(obox.x), centroid.y + y(obox.z));
					GetWindowDrawList()->AddLine(t0, t1, grey, pad);
				}
			}
		}

		if (ge->spec->tube && ge->settings && ge->settings->tube) {
			auto grey = GetColorU32(Color(0xffffff44));
			auto t0 = ImVec2(centroid.x + x(box.x), centroid.y + y(box.z));
			auto t1 = ImVec2(t0.x + x(ge->settings->tube->target.x), t0.y + y(ge->settings->tube->target.z));
			GetWindowDrawList()->AddLine(t0, t1, grey, pad*3);
		}

		bool showSpecIcon = false
			|| ge->spec->crafter
			|| ge->spec->depot
			|| ge->spec->storeSetUpper
			|| ge->spec->storeSetLower
			|| ge->spec->turret
			|| ge->spec->generateElectricity
			|| ge->spec->bufferElectricity
			|| ge->spec->launcher;

		if (ge->spec->conveyor || ge->spec->arm) chevron(offset, ge->dir());
		else if (showSpecIcon) specIcon(ge->spec, p0, p1);

		if (tipBegin(IsMouseHoveringRect(p0, p1))) {
			PushFont(Config::sidebar.font.imgui);
			Image(specIconChoose(ge->spec, maxIcon), ImVec2(maxIcon, maxIcon), ImVec2(0, 1), ImVec2(1, 0));

			if (ge->spec->crafter) {
				if (ge->settings && ge->settings->crafter && ge->settings->crafter->recipe) {
					recipeIcon(ge->settings->crafter->recipe); SameLine();
					Print(ge->settings->crafter->recipe->title.c_str());
				}
				else {
					Print("(no recipe)");
				}
			}

			if (ge->spec->storeSetUpper && ge->settings && ge->settings->store) {
				for (auto level: ge->settings->store->levels) {
					itemIcon(Item::get(level.iid)); SameLine();
					Print(Item::get(level.iid)->title.c_str());
				}
			}

			if (ge->spec->arm && ge->settings && ge->settings->arm) {
				for (auto iid: ge->settings->arm->filter) {
					itemIcon(Item::get(iid)); SameLine();
					Print(Item::get(iid)->title.c_str());
				}
			}

			if (ge->spec->loader && ge->settings && ge->settings->loader) {
				for (auto iid: ge->settings->loader->filter) {
					itemIcon(Item::get(iid)); SameLine();
					Print(Item::get(iid)->title.c_str());
				}
			}

			if (ge->spec->pipe && ge->settings && ge->settings->pipe && ge->settings->pipe->filter) {
				fluidIcon(Fluid::get(ge->settings->pipe->filter)); SameLine();
				Print(Fluid::get(ge->settings->pipe->filter)->title.c_str());
			}

			PopFont();
			tipEnd();
		}
	}

	auto h = GetFontSize();
	auto w = CalcTextSize("W").x;

	GetWindowDrawList()->AddText(ImVec2(centroid.x, centroid.y - size.y*0.5 + h), white, "N");
	GetWindowDrawList()->AddText(ImVec2(centroid.x, centroid.y + size.y*0.5 - h - h), white, "S");
	GetWindowDrawList()->AddText(ImVec2(centroid.x - size.x*0.5 + w*2, centroid.y - h*0.5), white, "W");
	GetWindowDrawList()->AddText(ImVec2(centroid.x + size.x*0.5 - w*3, centroid.y - h*0.5), white, "E");

	GetWindowDrawList()->PopClipRect();
}
