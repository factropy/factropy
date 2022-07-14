#pragma once

#include "common.h"
#include "glm-ex.h"
#include "../imgui/setup.h"

struct Color : glm::vec4 {
	Color() : glm::vec4(0.0f) {}
	Color(uint32_t hexValue);
	Color(float r, float g, float b, float a);

	Color gamma() const;
	Color degamma() const;

	operator const ImVec4() const {
		return ImVec4(r, g, b, a);
	};

	operator const std::string() const {
		return fmt("#%02X%02X%02X",
			std::min(255u, std::max(0u, (uint)std::round(r*255.0))),
			std::min(255u, std::max(0u, (uint)std::round(g*255.0))),
			std::min(255u, std::max(0u, (uint)std::round(b*255.0)))
		);
	};

	bool operator==(const Color& o) const {
		uint ar = std::min(255u, std::max(0u, (uint)std::round(r*255.0)));
		uint ag = std::min(255u, std::max(0u, (uint)std::round(g*255.0)));
		uint ab = std::min(255u, std::max(0u, (uint)std::round(b*255.0)));
		uint aa = std::min(255u, std::max(0u, (uint)std::round(a*255.0)));
		uint br = std::min(255u, std::max(0u, (uint)std::round(o.r*255.0)));
		uint bg = std::min(255u, std::max(0u, (uint)std::round(o.g*255.0)));
		uint bb = std::min(255u, std::max(0u, (uint)std::round(o.b*255.0)));
		uint ba = std::min(255u, std::max(0u, (uint)std::round(o.a*255.0)));
		bool req = ar == br;
		bool geq = ag == bg;
		bool beq = ab == bb;
		bool aeq = aa == ba;
		return req && geq && beq && aeq;
	};

	bool operator<(const Color& o) const {
		uint ar = std::min(255u, std::max(0u, (uint)std::round(r*255.0)));
		uint ag = std::min(255u, std::max(0u, (uint)std::round(g*255.0)));
		uint ab = std::min(255u, std::max(0u, (uint)std::round(b*255.0)));
		uint aa = std::min(255u, std::max(0u, (uint)std::round(a*255.0)));
		uint br = std::min(255u, std::max(0u, (uint)std::round(o.r*255.0)));
		uint bg = std::min(255u, std::max(0u, (uint)std::round(o.g*255.0)));
		uint bb = std::min(255u, std::max(0u, (uint)std::round(o.b*255.0)));
		uint ba = std::min(255u, std::max(0u, (uint)std::round(o.a*255.0)));
		bool req = ar == br;
		bool geq = ag == bg;
		bool beq = ab == bb;
		if (ar < br) return true;
		if (req && ag < bg) return true;
		if (req && geq && ab < bb) return true;
		if (req && geq && beq && aa < ba) return true;
		return false;
	};
};
