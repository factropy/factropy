#include "common.h"
#include "time-series.h"
#include <chrono>

TimeSeries::TimeSeries() {
	clear();
}

void TimeSeries::clear() {
	tickMax = 0.0f;
	secondMax = 0.0f;
	minuteMax = 0.0f;
	hourMax = 0.0f;
	for (uint i = 0; i < 60; i++) {
		ticks[i] = 0;
		seconds[i] = 0;
		minutes[i] = 0;
		hours[i] = 0;
	}
}

bool TimeSeries::empty() {
	return !(tickMax > 0.0f || secondMax > 0.0f || minuteMax > 0.0f || hourMax > 0.0f);
}

uint TimeSeries::tick(uint64_t t) {
	return t%60;
}

uint TimeSeries::second(uint64_t t) {
	return (t/60)%60;
}

uint TimeSeries::minute(uint64_t t) {
	return (t/60/60)%60;
}

uint TimeSeries::hour(uint64_t t) {
	return (t/60/60/60)%60;
}

void TimeSeries::set(uint64_t t, double v) {
	ticks[tick(t)] = v;
}

double TimeSeries::get(uint64_t t) {
	return ticks[tick(t)];
}

void TimeSeries::add(uint64_t t, double v) {
	ticks[tick(t)] += v;
}

void TimeSeries::update(uint64_t t) {
	tickMax = 0.0;
	secondMax = 0.0;
	minuteMax = 0.0;
	hourMax = 0.0;

	double tsum = 0.0;
	for (uint i = 0; i < 60; i++) {
		tsum += ticks[i];
		tickMax = std::max(tickMax, ticks[i]);
	}
	uint s = second(t);
	seconds[s] = agg(tsum);

	double msum = 0.0;
	for (uint i = 0; i < 60; i++) {
		msum += seconds[i];
		secondMax = std::max(secondMax, seconds[i]);
	}
	uint m = minute(t);
	minutes[m] = agg(msum);

	double hsum = 0;
	for (uint i = 0; i < 60; i++) {
		hsum += minutes[i];
		minuteMax = std::max(minuteMax, minutes[i]);
	}
	uint h = hour(t);
	hours[h] = agg(hsum);

	for (uint i = 0; i < 60; i++) {
		hourMax = std::max(hourMax, hours[i]);
	}
}

double TimeSeries::agg(double v) {
	return v/60.0;
}

double TimeSeriesSum::agg(double v) {
	return v;
}

void TimeSeries::track(uint64_t t, std::function<void(void)> fn) {
	StopWatch watch;
	track(t, watch.time(fn));
}

void TimeSeries::track(uint64_t t, const StopWatch& watch) {
	set(t, watch.milliseconds());
	update(t);
}

void TimeSeries::track(uint64_t t, const std::vector<StopWatch>& watches) {
	double n = 0;
	for (auto& watch: watches) {
		n += watch.milliseconds();
	}
	set(t, n/(double)watches.size());
	update(t);
}

StopWatch::StopWatch() {
	running = false;
	begin = std::chrono::steady_clock::now();
	finish = begin;
}

void StopWatch::start() {
	running = true;
	begin = std::chrono::steady_clock::now();
	finish = begin;
}

void StopWatch::stop() {
	running = false;
	finish = std::chrono::steady_clock::now();
}

double StopWatch::milliseconds() const {
	auto stamp = running ? std::chrono::steady_clock::now(): finish;
	uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(stamp-begin).count();
	return (double)us / 1000.0;
}

double StopWatch::seconds() const {
	auto stamp = running ? std::chrono::steady_clock::now(): finish;
	uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(stamp-begin).count();
	return (double)us / 1000000.0;
}

StopWatch& StopWatch::time(std::function<void(void)> fn) {
	start();
	fn();
	stop();
	return *this;
}
