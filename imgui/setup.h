#pragma once

#include "../src/gl-ex.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include "IconsFontAwesome4.h"
#include "implot.h"

#include <string>
#include <vector>

typedef unsigned int ImU32;

namespace ImGui {
	ImU32 ImColorSRGB(unsigned int hexValue);
	void TextColored(unsigned int hexValue, const char* text);
	void TextCentered(std::string text);
	void Print(std::string s);
	void PrintLeft(std::string s);
	void PrintRight(std::string s, bool spacing = false);
	void SpacingH();
	void SpacingV();
	void Header(std::string s, bool spacing = true);
	void Section(std::string s, bool spacing = true);
	void Alert(std::string s, bool spacing = true);
	void Notice(std::string s, bool spacing = true);
	void Warning(std::string s, bool spacing = true);
	void Title(std::string s);
	void LevelBar(float n, std::string s);
	void LevelBar(float n);
	void OverflowBar(float n, ImU32 ok = 0, ImU32 bad = 0);
	void MultiBar(float fraction, std::vector<float> splits, std::vector<ImU32> colors);
	bool InputIntClamp(const char* label, int* v, int low, int high, int step = 1, int step_fast = 100);
	void TrySameLine(const char* label, int margin = 0);
	bool WideButton(const char* label);
	bool ButtonStrip(int i, const char* label);
	void IconLabelStrip(int i, GLuint icon, const char* label);
	bool SmallButtonInline(const char* label);
	bool SmallButtonInlineRight(const char* label, bool spacing = false);
	void SmallBar(float n);
	bool ExpandingHeader(bool* state, const char* label, ImGuiTreeNodeFlags flags = 0);
	void ImageBanner(GLuint id, int w, int h);
	ImVec2 GetCursorAbs();
}
