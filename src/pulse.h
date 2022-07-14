#pragma once

#include <mutex>
#include <condition_variable>

class pulser {
private:
	std::mutex mutex;
	std::condition_variable condition;
	bool running;

public:
	pulser() {
		running = true;
	}

	~pulser() {
		stop();
	}

	void stop() {
		std::unique_lock<std::mutex> m(mutex);
		if (running) {
			running = false;
			condition.notify_all();
		}
		m.unlock();
	}

	bool wait() {
		std::unique_lock<std::mutex> m(mutex);
		if (running) condition.wait(m);
		bool run = running;
		m.unlock();
		return run;
	}

	void now() {
		std::unique_lock<std::mutex> m(mutex);
		condition.notify_all();
		m.unlock();
	}
};