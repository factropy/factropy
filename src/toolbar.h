#pragma once

#include "scene.h"
#include "recipe.h"
#include "item.h"
#include "fluid.h"
#include "time-series.h"
#include "../imgui/setup.h"

struct Toolbar {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoTitleBar
	;

	std::set<Spec*> specs;
	std::vector<Spec*> buttons;
	ImVec2 size = {1,1};

	Toolbar();
	void draw();
	void add(Spec* spec);
	void drop(Spec* spec);
	bool has(Spec* spec);
};

