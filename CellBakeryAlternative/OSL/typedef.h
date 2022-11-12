#pragma once

#include <chrono>

namespace osl {
	typedef float frac32;
	typedef double frac64;

	typedef frac64 frac;
}


typedef std::chrono::time_point<std::chrono::steady_clock> ch_tp;
auto chGetTime = std::chrono::steady_clock::now;
double chDurationMillis(ch_tp st, ch_tp et) {
	return std::chrono::duration<double, std::milli>(st - et).count();
}