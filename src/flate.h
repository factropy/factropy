#pragma once

#include "catenate.h"
#include "minivec.h"
#include <string>

struct deflation {
	int quality = 3;
	minivec<char> data;
	deflation();
	deflation(int quality);
	void save(const std::string& path);
	void push(const std::string& part);
};

struct inflation {
	minivec<char> data;

	inflation();
	inflation& load(std::string path);
	discatenate parts();
};