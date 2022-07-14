#include "common.h"
#include "energy.h"

// A "Watt" in the game is like a real Watt, roughly joule/second depending on
// the UPS rate. Energy consumption is usually done per tick however, so think
// of concrete energy values more like "Tick-Watts" or 1/60th of a real Watt.

Energy Energy::J(double j) {
	return Energy((int64_t)std::ceil(j));
}

Energy Energy::kJ(double j) {
	return Energy((int64_t)std::ceil(j*1000.0));
}

Energy Energy::MJ(double j) {
	return Energy((int64_t)std::ceil(j*1000000));
}

Energy Energy::GJ(double j) {
	return Energy((int64_t)std::ceil(j*1000000000));
}

Energy Energy::W(double j) {
	return Energy((int64_t)std::ceil(j/60.0));
}

Energy Energy::kW(double j) {
	return Energy((int64_t)std::ceil((j*1000.0)/60.0));
}

Energy Energy::MW(double j) {
	return Energy((int64_t)std::ceil((j*1000000.0)/60.0));
}

Energy Energy::GW(double j) {
	return Energy((int64_t)std::ceil((j*1000000000.0)/60.0));
}

Energy::Energy() {
	value = 0;
}

Energy::Energy(int64_t n) {
	value = n;
}

bool Energy::operator==(const Energy& o) const {
	return value == o.value;
}

bool Energy::operator!=(const Energy& o) const {
	return value != o.value;
}

bool Energy::operator<(const Energy& o) const {
	return value < o.value;
}

bool Energy::operator<=(const Energy& o) const {
	return value <= o.value;
}

bool Energy::operator>(const Energy& o) const {
	return value > o.value;
}

bool Energy::operator>=(const Energy& o) const {
	return value >= o.value;
}

Energy Energy::operator+(const Energy& o) const {
	return value + o.value;
}

Energy Energy::operator-(const Energy& o) const {
	return value - o.value;
}

Energy Energy::operator*(float n) const {
	return (int64_t)((double)value * n);
}

Energy Energy::operator*(double n) const {
	return (int64_t)(value * n);
}

void Energy::operator+=(const Energy& o) {
	value += o.value;
}

void Energy::operator-=(const Energy& o) {
	value -= o.value;
}

Energy::operator bool() const {
	return value != 0;
}

Energy::operator int64_t() const {
	return value;
}

Energy::operator double() const {
	return value;
}

//Energy::operator float() const {
//	return value;
//}
//
//Energy::operator int() const {
//	return value;
//}

std::string Energy::format() const {
	int64_t v = value;
	if (v < 1000) {
		return fmt("%ld J", v);
	}
	if (v < 10000) {
		return fmt("%0.1f kJ", (double)v/1000.0);
	}
	if (v < 1000000) {
		return fmt("%ld kJ", v/1000);
	}
	if (v < 10000000) {
		return fmt("%0.1f MJ", (double)v/1000000.0);
	}
	if (v < 1000000000) {
		return fmt("%ld MJ", v/1000000);
	}
	return fmt("%ld GJ", v/1000000000);
}

std::string Energy::formatRate() const {
	int64_t v = value*60;
	if (v < 1000) {
		return fmt("%ld W", v);
	}
	if (v < 10000) {
		return fmt("%0.1f kW", (double)v/1000.0);
	}
	if (v < 1000000) {
		return fmt("%ld kW", v/1000);
	}
	if (v < 10000000) {
		return fmt("%0.1f MW", (double)v/1000000.0);
	}
	if (v < 1000000000) {
		return fmt("%ld MW", v/1000000);
	}
	return fmt("%ld GW", v/1000000000);
}

float Energy::portion(Energy o) {
	if (o.value == 0) return 0.0f;
	return std::max(0.0f, std::min(1.0f, (float)value / (float)o.value));
}

Energy Energy::magnitude() {
	if (value == 0) return 1;
	double n = value*60;
	bool negative = n < 0;
	double log = std::log10(std::abs(n));
	double decimalPlaces = ((log > 0)) ? (std::ceil(log)) : (std::floor(log) + 1);
	double rounded = std::pow(10, decimalPlaces)/60;
	return std::round(negative ? -rounded : rounded);
}
