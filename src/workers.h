#pragma once

#include <thread>
#include <mutex>
#include <functional>
#include "channel.h"
#include "lalloc.h"

class workers {
private:
	uint pool = 0;
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

	~workers() {
		stop();
	}

	uint size() {
		return pool;
	}

	void start(uint p) {
		std::unique_lock<std::mutex> m(mutex);
		while (pool < p) {
			threads.push_back(std::thread(&workers::runner, this));
			pool++;
		}
	}

	void stop() {
		std::unique_lock<std::mutex> m(mutex);
		if (pool) {
			m.unlock();
			work.close();
			for (uint i = 0; i < pool; i++) {
				threads[i].join();
			}
			m.lock();
			threads.clear();
			submitted = 0;
			completed = 0;
			pool = 0;
		}
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
