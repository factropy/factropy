#include "common.h"
#include "item.h"
#include "fluid.h"
#include "mass.h"

const Mass Mass::Inf = (1000000000);

Mass Mass::g(int n) {
	return Mass(n);
}

Mass Mass::kg(int n) {
	return Mass(n*1000);
}

Mass::Mass() {
	value = 0;
}

Mass::Mass(int m) {
	value = m;
}

bool Mass::operator==(const Mass& o) const {
	return value == o.value;
}

bool Mass::operator!=(const Mass& o) const {
	return value != o.value;
}

bool Mass::operator<(const Mass& o) const {
	return value < o.value;
}

bool Mass::operator<=(const Mass& o) const {
	return value <= o.value;
}

bool Mass::operator>(const Mass& o) const {
	return value > o.value;
}

bool Mass::operator>=(const Mass& o) const {
	return value >= o.value;
}

Mass Mass::operator+(const Mass& o) const {
	return value + o.value;
}

Mass Mass::operator*(uint n) const {
	return value * n;
}

Mass Mass::operator-(const Mass& o) const {
	return value - o.value;
}

void Mass::operator+=(const Mass& o) {
	value += o.value;
}

void Mass::operator-=(const Mass& o) {
	value -= o.value;
}

Mass::operator bool() const {
	return value != 0;
}

std::string Mass::format() {
	if (value < 1000) {
		return fmt("%d g", value);
	}
	if (value < 1000*1000) {
		return fmt("%d kg", value/1000);
	}
	return fmt("%d t", value/1000/1000);
}

float Mass::portion(Mass o) {
	if (o.value == 0) return 1.0f;
	return std::max(0.0f, std::min(1.0f, (float)value / (float)o.value));
}

uint Mass::items(uint iid) {
	return value > 0 ? value / Item::get(iid)->mass.value: 0;
}

Liquid Liquid::l(int l) {
	return Liquid(l);
}

Liquid::Liquid() {
	value = 0;
}

Liquid::Liquid(int l) {
	value = l;
}

uint Liquid::fluids(uint fid) {
	return value > 0 ? value / Fluid::get(fid)->liquid.value: 0;
}

std::string Liquid::format() {
	if (value < 1000) {
		return fmt("%d L", value);
	}
	if (value < 10000) {
		return fmt("%0.1f kL", (float)value/1000.0);
	}
	if (value < 1000000) {
		return fmt("%d kL", value/1000);
	}
	return fmt("%d ML", value/1000000);
}
