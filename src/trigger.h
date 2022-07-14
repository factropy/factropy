#pragma once
#include "channel.h"

// cross between a monitor and a zero-length channel
// https://en.wikipedia.org/wiki/Monitor_(synchronization)

class trigger {
	std::mutex mutex;
	std::condition_variable recvCond;
	uint signals = 0;

public:
	void now() {
		std::unique_lock<std::mutex> m(mutex);
		signals++;
		recvCond.notify_all();
		m.unlock();
	}

	void wait() {
		std::unique_lock<std::mutex> m(mutex);
		while (!signals) recvCond.wait(m);
		signals--;
		m.unlock();
	}

	void wait(uint n) {
		for (uint i = 0; i < n; i++) wait();
	}
};
