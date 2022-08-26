
#include "../src/gl-ex.h"

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM

#include "imgui.cpp"
#include "imgui_draw.cpp"
#include "imgui_widgets.cpp"
#include "imgui_tables.cpp"
#include "imgui_freetype.cpp"
#include "imgui_impl_sdl.cpp"
#include "imgui_impl_opengl3.cpp"

#include "implot.cpp"
#include "implot_items.cpp"

#include "IconsFontAwesome4.h"

#include <string>
#include <vector>

namespace ImGui {
	ImU32 ImColorSRGB(unsigned int hexValue) {

		unsigned char ru = ((unsigned int)hexValue >> 24) & 0xFF;
		unsigned char gu = ((unsigned int)hexValue >> 16) & 0xFF;
		unsigned char bu = ((unsigned int)hexValue >>  8) & 0xFF;
		unsigned char au = ((unsigned int)hexValue >>  0) & 0xFF;

		float rf = (float)ru / 255.0f;
		float gf = (float)gu / 255.0f;
		float bf = (float)bu / 255.0f;
		float af = (float)au / 255.0f;

		return GetColorU32((ImVec4){rf,gf,bf,af});
	}

	void TextColored(unsigned int hexValue, const char* text) {

		unsigned char ru = ((unsigned int)hexValue >> 24) & 0xFF;
		unsigned char gu = ((unsigned int)hexValue >> 16) & 0xFF;
		unsigned char bu = ((unsigned int)hexValue >>  8) & 0xFF;
		unsigned char au = ((unsigned int)hexValue >>  0) & 0xFF;

		float rf = (float)ru / 255.0f;
		float gf = (float)gu / 255.0f;
		float bf = (float)bu / 255.0f;
		float af = (float)au / 255.0f;

		TextColored((ImVec4){rf,gf,bf,af}, "%s", text);
	}

	void TextCentered(std::string text) {
		auto regionWidth = GetContentRegionAvail().x;
		auto textWidth   = CalcTextSize(text.c_str(), nullptr, true).x;
		SetCursorPosX(GetCursorPosX() + ((regionWidth - textWidth) * 0.5f));
		TextUnformatted(text.c_str());
	}

	void Print(std::string s) {
		TextUnformatted(s.c_str());
	}

	void PrintLeft(std::string s) {
		SetCursorPosX(GetCursorPosX() + GetStyle().ItemSpacing.x);
		TextUnformatted(s.c_str());
	}

	void PrintRight(std::string s, bool spacing) {
		ImVec2 size = CalcTextSize(s.c_str(), nullptr, true);
		ImVec2 space = GetContentRegionAvail();
		SetCursorPosX(GetCursorPosX() + space.x - size.x - (spacing ? GetStyle().ItemSpacing.x: 0));
		TextUnformatted(s.c_str());
	}

	void SpacingH() {
		SetCursorPos(ImVec2(GetCursorPos().x + GetStyle().ItemSpacing.x, GetCursorPos().y));
	}

	void SpacingV() {
		SetCursorPos(ImVec2(GetCursorPos().x, GetCursorPos().y + GetStyle().ItemSpacing.y));
	}

	void panel(std::string s, bool spacing, bool center, ImU32 bg, ImU32 fg = 0) {
		ImVec2 size = CalcTextSize(s.c_str(), nullptr, true);
		ImVec2 space = GetContentRegionAvail();

		auto top = GetCursorPos();

		auto origin = top;
		origin.x += GetWindowPos().x;
		origin.y += GetWindowPos().y;
		origin.y -= GetScrollY();

		auto p0 = ImVec2(origin.x, origin.y);
		auto p1 = ImVec2(origin.x + space.x, origin.y + size.y + GetStyle().FramePadding.y*2);
		GetWindowDrawList()->AddRectFilled(p0, p1, bg, GetStyle().FrameRounding);

		if (center) SetCursorPos(ImVec2(top.x + space.x/2 - size.x/2, top.y + GetStyle().FramePadding.y));
		else SetCursorPos(ImVec2(top.x + GetStyle().FramePadding.x, top.y + GetStyle().FramePadding.y));

		if (fg) PushStyleColor(ImGuiCol_Text, fg);
		Print(s.c_str());
		if (fg) PopStyleColor(1);

		SetCursorPos(ImVec2(top.x, top.y + size.y + GetStyle().FramePadding.x*2));
		if (spacing) SpacingV();
	}

	void Header(std::string s, bool spacing) {
		panel(s, spacing, true, GetColorU32(ImGuiCol_TitleBgActive));
	}

	void Section(std::string s, bool spacing) {
		panel(s, spacing, true, ImColorSRGB(0x333333ff));
	}

	void Alert(std::string s, bool spacing) {
		panel(s, spacing, true, ImColorSRGB(0xcc0000ff), ImColorSRGB(0xffffffff));
	}

	void Notice(std::string s, bool spacing) {
		panel(s, spacing, true, ImColorSRGB(0x006600ff), ImColorSRGB(0xffffffff));
	}

	void Warning(std::string s, bool spacing) {
		panel(s, spacing, true, ImColorSRGB(0xdd8800ff), ImColorSRGB(0xffffffff));
	}

	void Title(std::string s) {
		PushStyleColor(ImGuiCol_Text, ImColorSRGB(0xffffccff));
		TextUnformatted(s.c_str());
		PopStyleColor(1);
	}

	void MultiBar(float fraction, std::vector<float> splits, std::vector<ImU32> colors) {
		fraction = std::max(0.01f, fraction);

		auto origin = GetCursorPos();

		auto aorigin = origin;
		aorigin.x += GetWindowPos().x;
		aorigin.y += GetWindowPos().y;

		ImVec2 space = {GetContentRegionAvail().x, GetFontSize()/3};
		float cursor = 0;

		GetWindowDrawList()->AddRectFilled(aorigin, {aorigin.x + space.x, aorigin.y + space.y}, GetColorU32(ImGuiCol_FrameBg));

		for (int i = 0, l = splits.size(); i < l && cursor < fraction; i++) {
			float left = space.x * cursor;
			float split = std::min(splits[i], fraction);
			float right = space.x * split;
			ImVec2 min = {aorigin.x + left, aorigin.y};
			ImVec2 max = {aorigin.x + right, aorigin.y + space.y};
			GetWindowDrawList()->AddRectFilled(min, max, colors[i]);
			cursor = splits[i];
		}

		SetCursorPos({GetCursorPos().x, origin.y + space.y});
		SpacingV();
	}

	void OverflowBar(float n, ImU32 ok, ImU32 bad) {
		n = std::max(0.01f, n);

		auto top = GetCursorPos();

		auto origin = top;
		origin.x += GetWindowPos().x - GetScrollX();
		origin.y += GetWindowPos().y - GetScrollY();

		ImVec2 space = {GetContentRegionAvail().x, GetFontSize()/3};

		GetWindowDrawList()->AddRectFilled(origin, {origin.x + space.x, origin.y + space.y}, GetColorU32(ImGuiCol_FrameBg));
		GetWindowDrawList()->AddRectFilled(origin, {origin.x + space.x * n, origin.y + space.y}, n > 0.999f ? bad: ok);

		SetCursorPos(ImVec2(top.x, top.y + space.y));
		SpacingV();
	}

	void LevelBar(float n, std::string s) {
		ProgressBar(std::max(0.01f, n), ImVec2(-1,0), s.c_str());
	}

	void LevelBar(float n) {
		LevelBar(n, "");
	}

	bool InputIntClamp(const char* label, int* v, int low, int high, int step, int step_fast) {
		bool f = InputInt(label, v, step, step_fast);
		if (f) *v = std::max(low, std::min(*v, high));
		return f;
	}

	void TrySameLine(const char* label, int margin) {
		SameLine();
		ImVec2 size = CalcTextSize(label, nullptr, true);
		ImVec2 space = GetContentRegionAvail();
		if (space.x < size.x + margin) NewLine();
	}

	bool WideButton(const char* label) {
		return Button(label, ImVec2(-1,0));
	}

	bool ButtonStrip(int i, const char* label) {
		if (i) TrySameLine(label, GetStyle().ItemSpacing.x + GetStyle().FramePadding.x*2);
		return Button(label);
	}

	void IconLabelStrip(int i, GLuint icon, const char* label) {
		if (i) {
			SameLine();
			ImVec2 size = CalcTextSize(label, nullptr, true);
			size.x += GetFontSize();
			ImVec2 space = GetContentRegionAvail();
			if (space.x < size.x + GetStyle().ItemSpacing.x) NewLine();
		}
		Image(icon, ImVec2(GetFontSize(), GetFontSize()), ImVec2(0, 1), ImVec2(1, 0));
		SameLine();
		Print(label);
	}

	bool SmallButtonInline(const char* label) {
		TrySameLine(label, GetStyle().ItemSpacing.x + GetStyle().FramePadding.x*2);
		return SmallButton(label);
	}

	bool SmallButtonInlineRight(const char* label, bool spacing) {
		auto pos = GetCursorPos();
		TrySameLine(label, GetStyle().ItemSpacing.x + GetStyle().FramePadding.x*2);

		ImVec2 size = CalcTextSize(label, nullptr, true);
		ImVec2 space = GetContentRegionAvail();
		SetCursorPosX(GetCursorPosX() + space.x - size.x - (spacing ? GetStyle().ItemSpacing.x: 0) - GetStyle().FramePadding.x*2);
		auto press = SmallButton(label);
		SetCursorPos(pos);
		return press;
	}

	void SmallBar(float n) {
		ProgressBar(std::max(0.01f, n), ImVec2(-1,GetFontSize()/3), "");
	}

	bool ExpandingHeader(bool* state, const char* label, ImGuiTreeNodeFlags flags) {
		*state = CollapsingHeader(label, flags);
		return *state;
	}

	void ImageBanner(GLuint id, int w, int h) {
		float iw = ImGui::GetContentRegionAvail().x;
		float ih = iw * ((float)h/(float)w);
		Image((ImTextureID)id, ImVec2(iw, ih));
	}

	ImVec2 GetCursorAbs() {
		auto cursor = GetCursorPos();
		cursor.x += GetWindowPos().x - GetScrollX();
		cursor.y += GetWindowPos().y - GetScrollY();
		return cursor;
	}
}
