#version 430 core

in vec2 WinK;
out vec3 pixel;
const int nullID = 0x7FFFFFFF;

uniform vec4 ViewWorld;
uniform vec4 ViewWindow;

uniform ivec2 WinSize;
uniform int Dm;
uniform float Dp;
uniform float onePixelRadius;

struct Chunk {
	int first_list_ID;
	int linked_list;
	float brightness;
};


readonly buffer ssbo_grid {
    Chunk chunks[];
};

readonly buffer ssbo_cellspp {
    int nearest[];
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





float circleGradient(float dist, float halfPixelRadius, float circleRadius) {
    return smoothstep(circleRadius - halfPixelRadius, circleRadius + halfPixelRadius, dist);
}
vec4 edge_color = vec4(0.);

vec3 render_background(vec2 Pos) {
	vec3 pixel;

	float Rp = Dp * 0.5;
	float drp = length(Pos - Rp);
	float ling;
	ivec2 ipos;
	if (drp > Rp) {
		ipos = ivec2(Pos) + 1;
		ling = chunks[ipos.x + ipos.y * Dm].brightness;
	} else {
		ipos = ivec2(normalize(Pos - Rp) * Rp + Rp) + 1;
		ling = chunks[ipos.x + ipos.y * Dm].brightness;
	}
	
	ling *= sqrt(ling);

	vec3 ling_color1 = clamp(mix(vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3), vec3(1.), ling * ling * 0.5), 0., 1.);
	vec3 ling_color2 = clamp(vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3) * 0.5, 0., 1.);
	vec3 ling_color3 = mix(vec3(0.03, 0.05, 0.08), vec3(0.64, 0.70, 0.98), smoothstep(0., 0.4, 1. / (sqrt(drp / Rp))));
	float k1 = circleGradient(drp, onePixelRadius * 0.5, Rp);
	float k2 = circleGradient(drp, onePixelRadius * 0.5, Rp + 0.333);
	
	pixel = ling_color1 * (1. - k1) + ling_color2 * (k1 - k2) + ling_color3 * k2;
	edge_color.rgb = pixel;
	edge_color.a = k1;
	return pixel;
}



vec4 render_cells(vec2 lWinK, vec2 lPixS) {
	vec4 pixel;
	return vec4(0.);
}


void main() {
	vec2 delta = 1. / WinSize;
	vec2 Pos = mix(ViewWorld.xy, ViewWorld.zw, WinK);
	pixel = render_background(Pos);
	if (edge_color.a == 1.)
		return;
	uint tc = (int(gl_FragCoord.x) + int(gl_FragCoord.y) * WinSize.x) * 2;

	if (nearest[tc] != nullID){
		int cid1 = nearest[tc];
		int cid2 = nearest[tc + 1];
		vec2 dp1 = Pos - cells[cid1].pos - cells[cid1].ipos;
		vec2 dp2 = vec2(0.);

		float r1 = length(dp1);

		float r2 = 10e10;

		float nr1 = r1 / cells[cid1].radius * 2.;
		float nr2 = 10e10;

		if (cid2 != nullID) {
			dp2 = Pos - cells[cid2].pos - cells[cid2].ipos;
			r2 = length(dp2);
			nr2 = r2 / cells[cid2].radius * 2.;
		}
		
		vec4 color = vec4(cells[cid1].color_rgb, 1.);

		float mux_onePixelRadius = onePixelRadius / cells[cid1].radius;
		if (nr2 < 1. && nr1 > 0.2) { // изгибы
			mux_onePixelRadius += (2. - nr1 - nr2);

			nr1 += 1. - nr2;
		}
		
		
		float a_mux = 1. - circleGradient(nr1, mux_onePixelRadius, 1.);
		if (a_mux == 0.)
			return;

		// фагоцит
		float edge_1 = circleGradient(nr1, mux_onePixelRadius, 0.2);
		float edge_2 = circleGradient(nr1, mux_onePixelRadius, 0.9);
		color = vec4(color.rgb * 0.5, 1.) * (1. - edge_1) + vec4(color.rgb, 0.67) * (edge_1 - edge_2) + vec4(color.rgb * 0.5, 1.) * edge_2;
		int b = int(nr1 < 0.20 || nr1 > 0.90);
		
		switch (b) {
			case(3): // пустота
				color.a = 0.;
				break;
			case(2): // жир липоцита
				color = vec4((color.rgb + vec3(0.5, 0.25, 0.25) * 3.) * 0.25, 0.67);
				break;
			case(1):
				color.rgb = color.rgb * 0.5;
				break;
			default:
				color.a = 0.67;
		}

		pixel = mix(pixel, mix(mix(mix(cells[cid2].color_rgb * 0.5, pixel, step(1., nr2)), edge_color.rgb, edge_color.a), color.rgb, a_mux), color.a);
	}
}



/*
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
*/