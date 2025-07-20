#version 430 core

in vec2 WinK;
out vec3 pixel;

const int nullID = 0x7FFFFFFF;

uniform vec4 ViewWorld;
uniform vec4 deltaViewWorld;

uniform ivec2 WinSize;
uniform int Dm;
uniform float Dp;

restrict buffer ssbo_cellspp {
    int counter[];
};

struct Chunk {
	int first_list_ID;
	int linked_list;
	float brightness;
};

readonly buffer ssbo_grid {
    Chunk chunks[];
};

struct Cell {
	ivec2 ipos;
	vec2 pos;
	float radius;
	float angle;
	float rotate_vel;
	vec3 color_rgb;
	vec3 color_hsv;
	int type_id;
	int linked_list;
	int chunk_id;
	int is_first;
	float weight;
	vec2 velocity;
	ivec2 force;
};

readonly buffer ssbo_cells {
    Cell cells[];
};

const ivec2 vectorID[8] = ivec2[8] (ivec2(-1, -1), ivec2(0, -1), ivec2(1, -1), ivec2(-1, 0), ivec2(1, 0), ivec2(-1, 1), ivec2(0, 1), ivec2(1, 1));

//length

bool compareDistanse(vec2 v, float l) {
	return v.x * v.x + v.y * v.y < l * l;
}

float cubicInterpolate(float x, vec4 p) {
	return p[1] + (-0.5 * p[0] + 0.5 * p[2]) * x
		+ (p[0] - 2.5 * p[1] + 2.0 * p[2] - 0.5 * p[3]) * x * x
		+ (-0.5 * p[0] + 1.5 * p[1] - 1.5 * p[2] + 0.5 * p[3]) * x * x * x;
}

float bicubicInterpolate(vec2 uv, mat4x4 p) {
	return cubicInterpolate(uv.x, vec4(
		cubicInterpolate(uv.y, p[0]),
		cubicInterpolate(uv.y, p[1]),
		cubicInterpolate(uv.y, p[2]),
		cubicInterpolate(uv.y, p[3]))
	);
}


// MAIN
///============================================================///
void main() {
	vec2 Pos = mix(ViewWorld.xy, ViewWorld.zw, WinK);

	uint tc = int(gl_FragCoord.x) + int(gl_FragCoord.y) * WinSize.x;
	if (counter[tc] != nullID){
		pixel = vec3(1);
		counter[tc] = nullID;
	}
	else {
		discard;
		return;
	}


	int cid = counter[tc];
	float qd = dot(dp, dp); // вкадрат расстояния до целевой клетки
	float c_r = cells[cid].radius;
	float qr = c_r * c_r; // вкадрат радиуса целевой клетки
	int t = cells[cid].type_id;

	if (qd < qr) {
		float r = length(dp / cells[cid].radius); // относительный радиус
		float a;
		int b = 0;

		// Тип структуры
		switch (t) {
			case(0): // фагоцит
			case(1): // жгутоцит
			case(3): // девороцит  (fract(a * 19.) > 0.8);
				b = int(r < 0.20 || r > 0.90);
				break;
			case(2): // фотоцит
				if (r < 0.20 || r > 0.90)
					b = 1;
				else {
					a = fract(atan(-dp.y, -dp.x) / 6.2831853 - cells[cid].angle);
					b = int(ihash2(uvec2(r * 5., a * 20.)) & 4u);
				}
				break;
			case(4): // липоцит
				b = (r < 0.20 || r > 0.93) ? 1 : int(r < 0.85) * 2;
				break;
			case(5): // кератиноцит
				b = int((r < 0.2) || ((r > 0.7) && (r < 0.8)) || (r > 0.9));
				break;
			default:
				b = int(r > 0.9);
		}

		// Тип заливки
		switch (b) {
			case(4): // хлоропласт
				pixel = vec4(normalize(cells[cid].color_rgb * vec3(0.5, 1.3, 0.3)) * 0.5, 0.90);
				break;
			case(3): // пустота
				discard;
			case(2): // жир липоцита
				pixel = vec4((cells[cid].color_rgb + vec3(0.5, 0.25, 0.25) * 3.) * 0.25, 0.90);
				break;
			case(1): // тёмная заливка
				pixel = vec4(cells[cid].color_rgb * 0.5, 1.);
				break;
			default: // светлая заливка
				pixel = vec4(cells[cid].color_rgb, 0.67);
		}

	} else if (qd < qr * t2kr[t] * t2kr[t] || t == 1) {
		float r = length(dp / c_r); // относительный радиус
		float a = fract(atan(-dp.y, -dp.x) / 6.2831853 - cells[cid].angle - 0.5);
		int b = 0;
		
		// Тип структуры
		switch (t) {
			case(1): // жгутоцит
				b = int(abs(0.5 - a) - 0.02 < 0.);
				break;
			case(3): // девороцит
				a = abs(fract(a * 19.) - 0.5);
				b = int((a - sqrt(r) * 1.2 > -0.865) || (1. / (0.85 - a) - (sqrt(r) - 1.) * 8. > 1.8));
				//b = int(mix(1. / (0.9 - a) - (r - 1.) * 3. + 1.7, a - sqrt(r) * 1.2 - 0.865, 0.5) > 0.79);
				//b = int((a - sqrt(r) * 1.2 > -0.865) || (a - r * 2. > -1.73));
				//b = int(1. / (0.9 - a) - (r - 1.) * 3. - 1.7 > 0. || a - sqrt(r) * 1.2 - 0.865 > 0.);
				break;
			case(8): // вироцит
				b = int(fract(a * 17.) - r * 0.1 > 0.7); //int((a + (0.1 / r) > 0.5)
				break;
		}

		// Тип заливки
		switch (b) {
			case(2): // тёмная заливка
				pixel = vec4(cells[cid].color_rgb * 0.5, 1.);
				break;
			case(1): // тёмная заливка
				pixel = vec4(cells[cid].color_rgb * 0.5, 1.);
				break;
			default: // пустота
				pixel = vec4(cells[cid].color_rgb * 0.5, 0.2);
				//discard;
		}
	} else discard;
}