#include "color.h"

Color::Color(uint32_t hexValue) {
	uint8_t ru = (hexValue >> 24) & 0xFF;
	uint8_t gu = (hexValue >> 16) & 0xFF;
	uint8_t bu = (hexValue >>  8) & 0xFF;
	uint8_t au = (hexValue >>  0) & 0xFF;

	// web srgb hex codes will look wrong when
	// shaders apply final gamma correction, so
	// adjust back to linear
	float rf = pow((float)ru / 255.0f, 2.2f);
	float gf = pow((float)gu / 255.0f, 2.2f);
	float bf = pow((float)bu / 255.0f, 2.2f);
	float af = (float)au / 255.0f;

	x = rf;
	y = gf;
	z = bf;
	w = af;
}

Color Color::degamma() const {
	float rf = pow(x, 2.2f);
	float gf = pow(y, 2.2f);
	float bf = pow(z, 2.2f);
	return Color(rf, gf, bf, w);
}

Color Color::gamma() const {
	float rf = pow(x, 1.0f/2.2f);
	float gf = pow(y, 1.0f/2.2f);
	float bf = pow(z, 1.0f/2.2f);
	return Color(rf, gf, bf, w);
}

Color::Color(float r, float g, float b, float a) {
	x = r;
	y = g;
	z = b;
	w = a;
}



