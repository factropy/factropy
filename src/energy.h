#pragma once

// A "Watt" in the game is like a real Watt, roughly joule/second depending on
// the UPS rate. Energy consumption is usually done per tick however, so think
// of concrete energy values more like "Tick-Watts" or 1/60th of a real Watt.

struct Energy;

#include <string>
#include <set>

struct Energy {
	static Energy J(double j);
	static Energy kJ(double j);
	static Energy MJ(double j);
	static Energy GJ(double j);
	static Energy W(double j); // joules/tick
	static Energy kW(double j); // joules/tick
	static Energy MW(double j); // joules/tick
	static Energy GW(double j); // joules/tick

	int64_t value;

	bool operator==(const Energy& o) const;
	bool operator!=(const Energy& o) const;
	bool operator<(const Energy& o) const;
	bool operator<=(const Energy& o) const;
	bool operator>(const Energy& o) const;
	bool operator>=(const Energy& o) const;
	Energy operator+(const Energy& o) const;
	Energy operator-(const Energy& o) const;
	Energy operator*(float n) const;
	Energy operator*(double n) const;
	void operator+=(const Energy& o);
	void operator-=(const Energy& o);

	//float operator/(const Energy& o) const; // ratio

	operator bool() const;
	operator int64_t() const;
	operator double() const;
	//operator float() const;
	//operator int() const;

	Energy();
	Energy(int64_t);
	std::string format() const;
	std::string formatRate() const;
	float portion(Energy o);
	Energy magnitude();
};
