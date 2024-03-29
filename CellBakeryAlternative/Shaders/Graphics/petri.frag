#version 430 core

in vec2 WinK;
out vec3 pixel;

uniform vec4 ViewWorld;
uniform vec4 deltaViewWorld;

uniform ivec2 WinSize;
uniform int Dm;
uniform float Dp;

struct Chunk {
	int first_list_ID;
	int linked_list;
	float brightness;
};


readonly buffer ssbo_grid {
    Chunk chunks[];
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

	//vec2 lPos = mix(deltaViewWorld.xy, deltaViewWorld.zw, WinK);

	pixel = vec3(0.64, 0.70, 0.98);
	float Rp = Dp * 0.5;
	
	ivec2 ipos = ivec2(Pos) + 1;
	vec2 frpos = fract(Pos);
	

	float ling = chunks[ipos.x + ipos.y * Dm].brightness;

	if (compareDistanse(Pos - Rp, Rp)) {
//		{
//			mat4x4 p;
//			for (int x = 0; x < 4; x++)
//				for (int y = 0; y < 4; y++)
//					p[x][y] = chunks[ipos.x + x - 1 + (ipos.y + y - 1) * Dm].brightness;
//			ling = clamp(bicubicInterpolate(frpos, p), 0., 1.);
//		}
		ling *= sqrt(ling);
		pixel = vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3);
		pixel = mix(pixel, vec3(1.), ling * ling * 0.5);

	//	pixel *= pow(min(1., frpos.x * 10.) * min(1., frpos.y * 10.), 0.2);
	} 
	else if (compareDistanse(Pos - Rp, Rp + 0.01 / 0.03)) { // если это край чаши
		ling *= sqrt(ling);
		pixel = vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3) * 0.5;
		//pixel = vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3);
		//pixel = mix(pixel, vec3(1.), ling * ling * 0.5);

		pixel = mix(pixel * 0.5, vec3(0.5), 0.5);
	}
}