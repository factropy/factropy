#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include "channel.h"

class workers {
private:
	std::vector<std::thread> threads;
	channel<std::function<void(void)>,-1> work;
	std::mutex mutex;
	uint64_t submitted = 0;
	uint64_t completed = 0;
	std::condition_variable waiting;

	void runner() {
		std::unique_lock<std::mutex> m(mutex);
		m.unlock();

		for (auto job: work) {
			lsanity();
			job();

			m.lock();
			completed++;
			waiting.notify_all();
			m.unlock();
		}

		linfo();
	}

public:
	workers() {
	}

	workers(int p) {
		start(p);
	}

	~workers() {
		stop();
	}

	uint size() {
		std::unique_lock<std::mutex> m(mutex);
		return threads.size();
	}

	void start(uint p = 1) {
		std::unique_lock<std::mutex> m(mutex);
		work.open();
		while (threads.size() < p) {
			threads.push_back(std::thread(&workers::runner, this));
		}
	}

	void stop() {
		std::unique_lock<std::mutex> m(mutex);
		work.close();
		m.unlock();
		for (auto& thread: threads) thread.join();
		m.lock();
		threads.clear();
		submitted = 0;
		completed = 0;
	}

	bool job(std::function<void(void)> fn) {
		std::unique_lock<std::mutex> m(mutex);
		submitted++;
		m.unlock();
		return work.send(fn);
	}

	// single-sender pattern
	// wait for jobs already submitted to complete
	void wait() {
		std::unique_lock<std::mutex> m(mutex);
		uint64_t count = submitted;
		while (count > completed) waiting.wait(m);
	}
};

extern workers async;
