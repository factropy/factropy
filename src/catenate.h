#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <cassert>

template <typename IT>
std::string concatenate(IT begin, IT end, std::string sep = ",") {
	std::stringstream ss;
	int i = 0;
	for (auto it = begin; it != end; ++it) {
		if (i++ > 0) {
			ss << sep;
		}
		ss << *it;
	}
	return ss.str();
}

template <typename C>
std::string concatenate(C& c, std::string sep = ",") {
	return concatenate(c.begin(), c.end(), sep);
}

class discatenate {
	std::string_view doc;
	std::string_view sep;
	std::string _doc;
	std::string _sep;

	class iterator {
		std::size_t pos, end;
		const discatenate* dis;

	public:
		typedef std::string value_type;
		typedef uint difference_type;
		typedef std::string* pointer;
		typedef std::string& reference;
		typedef std::input_iterator_tag iterator_category;

		iterator() {
			dis = nullptr;
			pos = std::string::npos;
			end = std::string::npos;
		}

		explicit iterator(const discatenate* ddis, std::size_t ppos) {
			dis = ddis;
			pos = ppos == std::string::npos || ppos >= dis->doc.size() ? std::string::npos: ppos;
			end = pos == std::string::npos ? std::string::npos: dis->doc.find(dis->sep, pos);
		}

		std::string_view operator*() const {
			if (pos != std::string::npos && end == std::string::npos) {
				return dis->doc.substr(pos);
			}
			if (pos != std::string::npos && end != std::string::npos) {
				return dis->doc.substr(pos, end-pos);
			}
			assert(pos == std::string::npos);
			assert(end == std::string::npos);
			return "";
		}

		bool operator==(const iterator& other) const {
			return dis == other.dis && pos == other.pos;
		}

		bool operator!=(const iterator& other) const {
			return dis != other.dis || pos != other.pos;
		}

		iterator& operator++() {
			if (pos != std::string::npos && end == std::string::npos) {
				pos = std::string::npos;
				return *this;
			}
			if (pos != std::string::npos && end != std::string::npos) {
				pos = end + dis->sep.length();
				end = dis->doc.find(dis->sep, pos);
				return *this;
			}
			assert(pos == std::string::npos);
			assert(end == std::string::npos);
			return *this;
		}

		iterator operator++(int) {
			iterator tmp(*this);
			++*this;
			return tmp;
		};
	};

public:
	discatenate(const char* ddoc, const char* ssep) {
		_doc = ddoc;
		doc = _doc;
		_sep = ssep;
		sep = _sep;
	}

	discatenate(const std::string_view& ddoc, const char* ssep) {
		doc = ddoc;
		_sep = ssep;
		sep = _sep;
	}

	discatenate(const std::string_view& ddoc, const std::string_view& ssep) {
		doc = ddoc;
		sep = ssep;
	}

	iterator begin() const {
		return iterator(this, 0);
	}

	iterator end() const {
		return iterator(this, std::string::npos);
	}
};
