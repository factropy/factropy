#pragma once
#include <iostream>
#include <functional>
#include <cstring>
#include <string>
#include <memory>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#undef near
#undef far
#undef interface
typedef uint32_t uint;
#endif

void wtf(const char*, const char*, int, const char*);
#define WTF(msg) wtf(__BASE_FILE__, __FUNCTION__, __LINE__, (msg))

#include "channel.h"
#include "trigger.h"
#include "log.h"

// Convert all std::strings to const char* using constexpr if (C++17)
template<typename T>
auto fmtConvert(T&& t) {
	if constexpr (std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, std::string>::value) {
		return std::forward<T>(t).c_str();
	}
	else {
		return std::forward<T>(t);
	}
}

// printf like formatting for C++ with std::string
// https://gist.github.com/Zitrax/a2e0040d301bf4b8ef8101c0b1e3f1d5
template<typename ... Args>
std::string fmtInternal(const std::string& format, Args&& ... args)
{
	size_t size = std::snprintf(nullptr, 0, format.c_str(), std::forward<Args>(args) ..., NULL) + 1;
	if( size <= 0 ) { WTF("fmtInternal"); }
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args ..., NULL);
	return std::string(buf.get(), buf.get() + size - 1);
}

template<typename ... Args>
std::string fmt(std::string fmt, Args&& ... args) {
	return fmtInternal(fmt, fmtConvert(std::forward<Args>(args))...);
}

#define fmtc(...) fmt(__VA_ARGS__).c_str()

#define infof(...) { fprintf(stderr, "%s", fmt(__VA_ARGS__).c_str()); fputc('\n', stderr); }
#define fatalf(...) { WTF(fmtc(__VA_ARGS__)); }

#define ensure(cond,...) if (!(cond)) { WTF("(no detail)"); }
#define ensuref(cond,...) if (!(cond)) { WTF(fmtc(__VA_ARGS__)); }
#define throwf(cond,...) if (!(cond)) { throw std::runtime_error(fmt(__VA_ARGS__)); }

template <typename T, typename B>
void if_is(B* value, std::function<void(T*)> action) {
	auto cast_value = dynamic_cast<T*>(value);
	if (cast_value != nullptr) {
		action(cast_value);
	}
}

template <typename C, typename V>
bool contains(const C& c, const V& v) {
	return std::find(c.begin(), c.end(), v) != c.end();
}

template <typename C>
void deduplicate(C& c) {
	std::sort(c.begin(), c.end());
	c.erase(std::unique(c.begin(), c.end()), c.end());
}

template <typename C, typename V>
void discard(C& c, const V& v) {
	c.erase(std::remove(c.begin(), c.end(), v), c.end());
}

template <typename C, typename F>
void discard_if(C& c, F fn) {
	c.erase(std::remove_if(c.begin(), c.end(), fn), c.end());
}

template <typename C, typename F>
void reorder(C& c, F fn) {
	std::sort(c.begin(), c.end(), fn);
}

template <typename C>
void reorder(C& c) {
	std::sort(c.begin(), c.end());
}
