#pragma once

#include "scene.h"
#include "recipe.h"
#include "item.h"
#include "fluid.h"
#include "time-series.h"
#include "../imgui/setup.h"

struct HUD {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav
		| ImGuiWindowFlags_NoSavedSettings
	;

	std::vector<Spec*> specs;

	struct {
		std::vector<Spec*> generators;
		std::vector<Spec*> consumers;
	} electricity;

	HUD() = default;
	void draw();
};

