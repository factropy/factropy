#pragma once

#include "common.h"
#include "channel.h"

namespace Log {
	extern channel<std::string,-1> log;
}

#define notef(...) { Log::log.send(fmt(__VA_ARGS__)); infof(__VA_ARGS__); }
