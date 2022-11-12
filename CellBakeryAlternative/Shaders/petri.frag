#version 330 core

in vec2 WinK;
out vec3 pixel;

uniform vec4 ViewWorld;
uniform ivec2 WinSize;
uniform int mapSize;
uniform float Dp;
uniform float Ac;

uniform sampler2D Light;

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

	pixel = vec3(0.64, 0.70, 0.98);
	float hDp = Dp * 0.5;
	if (compareDistanse(Pos - hDp, hDp)) {
		//float ling = texelFetch(Light, ivec2(Pos / Ac), 0).r;
		float ling = texture2D(Light, Pos / Ac / mapSize).r; 
		//	{
		//		mat4x4 p;
		//		for (int x = 0; x < 4; x++)
		//			for (int y = 0; y < 4; y++)
		//				p[x][y] = texelFetch(Light, ivec2(Pos / Ac) + ivec2(x, y) - 1, 0).r;
		//		ling = bicubicInterpolate(fract(Pos / Ac), p);
		//	}
		ling *= sqrt(ling);
		pixel = vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3);
		pixel = mix(pixel, vec3(1.), ling * ling * 0.5);
	} 
	else if (compareDistanse(Pos - hDp, hDp + 0.01)) { // если это край чаши
		float ling = texture(Light, Pos / Ac / mapSize).r; 
		ling *= ling;
		pixel = vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3) * 0.5;
		pixel = mix(pixel * 0.5, vec3(0.5), 0.5);
	}
}