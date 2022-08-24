#include "../src/flate.h"
#include <cstring>
#include <filesystem>

#define SDEFL_IMPLEMENTATION
#define SINFL_IMPLEMENTATION
#include "../src/flate.cc"

void wtf(const char* file, const char* func, int line, const char* err) {
	fprintf(stderr, "abort: %s:%d %s()\n%s",
		file ? (char*)std::filesystem::path(file).filename().c_str(): "",
		line, func, err ? err: ""
	);
	std::terminate();
}

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		inflation inf;
		inf.load(argv[i]);
		for (auto part: inf.parts()) {
			std::cout << part << std::endl;
		}
	}
}
