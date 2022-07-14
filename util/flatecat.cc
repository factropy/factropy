#include "../src/flate.h"
#include <cstring>

#define SDEFL_IMPLEMENTATION
#define SINFL_IMPLEMENTATION
#include "../src/flate.cc"

int main(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		inflation inf;
		inf.load(argv[i]);
		for (auto part: inf.parts()) {
			std::cout << part << std::endl;
		}
	}
}
