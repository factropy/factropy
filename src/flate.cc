#include "common.h"
#include "flate.h"
#include <fstream>

#include "sdefl.h"
#include "sinfl.h"

deflation::deflation() {
}

deflation::deflation(int q) {
	quality = std::max(0, std::min(9, q));
}

void deflation::save(const std::string& path) {
	std::size_t bounds = sdefl_bound(data.size());

	std::vector<char> cdata;
	cdata.insert(cdata.begin(), bounds, 0);

	struct sdefl sdefl = { 0 };
	std::size_t clen = sdeflate(&sdefl, (unsigned char*)cdata.data(), (unsigned char*)data.data(), data.size(), quality);

	auto out = std::ofstream(path, std::ios::binary);
	out << fmt("%lu\n", data.size());
	out.write(cdata.data(), clen);
	out.close();
}

void deflation::push(const std::string& part) {
	if (data.size()) data.push_back('\n');
	data.insert(data.end(), part.begin(), part.end());
}

inflation::inflation() {
}

inflation& inflation::load(std::string path) {
	auto in = std::ifstream(path);

	std::string line;
	throwf(std::getline(in, line), "inflation %s", path);

	uint size;
	throwf(1 == std::sscanf(line.c_str(), "%u", &size), "inflation %s malformed", path);

	in.close();

	in = std::ifstream(path, std::ios::binary);

	in.seekg(0, std::ios::end);
	std::size_t clen = in.tellg();
	in.seekg(line.length()+1);
	clen -= line.length()+1;

	std::vector<char> cdata;
	cdata.insert(cdata.begin(), clen, 0);

	in.read(cdata.data(), clen);
	in.close();

	data.clear();
	data.insert(data.begin(), size, 0);

	std::size_t dlen = sinflate((unsigned char*)data.data(), (unsigned char*)cdata.data(), clen);
	throwf(dlen == size, "inflation %s incorrect size", path);

	return *this;
}

discatenate inflation::parts() {
	return discatenate(std::string_view(data.data(), data.size()), "\n");
}
