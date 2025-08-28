#version 430 core

in vec2 WinK;
out vec4 pixel;

uniform vec4 ViewWorld;
uniform vec4 ViewWindow;


float grid1(vec2 uv) {
	uv = fract(uv * 0.25);
    return abs(step(uv.x, 0.5) - step(uv.y, 0.5));
}

vec3 render_background(vec2 uv) {
	vec3 pixel;


	float ling = 0.3;

	ling *= sqrt(ling);

	vec3 ling_color1 = clamp(mix(vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3), vec3(1.), ling * ling * 0.5), 0., 1.);
	vec3 ling_color2 = clamp(vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3) * 0.5, 0., 1.);

	pixel = ling_color1;// * step(fract(uv.y * 0.5), 0.5) * 0.02 + 0.1;

	return pixel;
}

void main() {
	pixel = vec4(render_background(mix(ViewWorld.xy, ViewWorld.zw, WinK)), 1.);
}