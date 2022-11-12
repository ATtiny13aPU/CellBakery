#pragma once

namespace osl {

	class fastLinearFilter {
	public:
		fastLinearFilter(int s = 10) : buffSize(s) {
			buffSize = buffSize < 1 ? 1 : buffSize;
			buff.resize(buffSize);
			for (int i = 0; i < buffSize; i++)
				buff[i] = 0.;
			sum = 0.;
			mul = 1. / buffSize;
		}

		void push(double v = 0.) {
			sum += v - buff[p];
			buff[p] = v;
			p = (p + 1) % buffSize;
		}

		void resum() {
			sum = 0;
			for (int i = 0; i < buffSize; i++)
				sum += buff[i];
		}

		double get() {
			return sum * mul;
		}

	private:
		double sum = 0;
		int p = 0;
		int buffSize;
		double mul;
		std::vector<double> buff;
	};

	bool compareDistanse(vec2 v, frac l) {
		return v[0] * v[0] + v[1] * v[1] < l * l;
	}

	bool compareDistanse(vec3 v, frac l) {
		return v[0] * v[0] + v[1] * v[1] + v[2] * v[2] < l * l;
	}

	inline frac clamp(frac v, frac in_min, frac in_max) {
		return v < in_min ? in_min : (in_max < v ? in_max : v);
	}

	inline frac remap(frac x, frac in_min, frac in_max, frac out_min, frac out_max) {
		return (clamp(x, in_min, in_max) - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	}

	inline frac fract(frac v) {
		return v - std::floor(v);
	}

	inline frac mix(frac v1, frac v2, frac k) {
		return v1 * (1. - k) + v2 * k;
	}


	using JIO::abs;
	using JIO::clamp;
	using JIO::dot;
	using JIO::mix;
	using JIO::remap;
	using JIO::floor;
	using JIO::fract;

	vec3 hue(frac x) {
		x = fract(x);
		frac x1 = x * 6.;
		x = fract(x1);
		frac r = 0., g = 0., b = 0.;

		if (x1 < 3.) {
			if (x1 < 1.) {
				r = 1.; g = x; // 1
			}
			else if (x1 < 2.) {
				g = 1.; r = 1. - x;  // 2
			}
			else {
				g = 1.; b = x;  // 3
			}
		}
		else {
			if (x1 < 4.) {
				b = 1.; g = 1. - x;  // 4
			}
			else if (x1 < 5.) {
				b = 1.; r = x;  // 5
			}
			else {
				r = 1.; b = 1. - x;  // 6
			}
		}
		return vec3(r, g, b);
	}

	vec3 sat(vec3 col, frac s) {
		return mix(vec3(1.), col, s);
	}

	vec3 vel(vec3 col, frac v) {
		return col * v;
	}

	vec3 HSV2RGB(frac h, frac s, frac v) {
		vec3 col = hue(h);
		col = sat(col, s);
		col = vel(col, v);
		return col;
	}

	vec3 HSV2RGB(vec3 hsv) {
		vec3 col = hue(hsv[0]);
		col = sat(col, hsv[1]);
		col = vel(col, hsv[2]);
		return col;
	}

}