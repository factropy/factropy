#pragma once

#include <string>
#include <map>
#include "item.h"

// On-screen messages visible to the player. Not a log file.

struct Message {
	static void reset();
	static void saveAll(const char *path);
	static void loadAll(const char *path);
	static void tick();

	static inline std::map<std::string,Message*> all;
	static Message* byName(std::string name);

	std::string name;
	std::string text;
	bool sent = false;

	uint64_t when = 0;

	Message(std::string name);
	void check();
	void complete();
	bool active();
};


