#pragma once

#include "catenate.h"
#include <string>
#include <vector>

struct deflation {
	int quality = 3;
	std::vector<char> data;
	deflation();
	deflation(int quality);
	void save(const std::string& path);
	void push(const std::string& part);
};

struct inflation {
	std::vector<char> data;

	inflation();
	inflation& load(std::string path);
	discatenate parts();
};