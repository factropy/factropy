#pragma once

#include <vector>
#include <chrono>
#include <functional>

struct StopWatch {
	bool running = false;
	std::chrono::time_point<std::chrono::steady_clock> begin;
	std::chrono::time_point<std::chrono::steady_clock> finish;
	StopWatch();
	void start();
	void stop();
	double seconds() const;
	double milliseconds() const;
	StopWatch& time(std::function<void(void)> fn);
};

struct TimeSeries {
	double tickMax;
	double secondMax;
	double minuteMax;
	double hourMax;
	double tickMean;
	double secondMean;
	double minuteMean;
	double hourMean;
	// circular buffers
	double ticks[60];
	double seconds[60];
	double minutes[60];
	double hours[60];

	void clear();
	static uint tick(uint64_t t);
	static uint second(uint64_t t);
	static uint minute(uint64_t t);
	static uint hour(uint64_t t);
	void set(uint64_t t, double v);
	double get(uint64_t t);
	void add(uint64_t t, double v);
	void update(uint64_t t);
	void track(uint64_t t, std::function<void(void)> fn);
	void track(uint64_t t, const StopWatch& watch);
	void track(uint64_t t, const std::vector<StopWatch>& watches);
	virtual double agg(double v);
	bool empty();

	TimeSeries();
};

struct TimeSeriesSum : TimeSeries {
	double agg(double v);
};
