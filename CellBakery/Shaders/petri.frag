#version 430 core
#extension GL_ARB_fragment_shader_interlock : enable

layout(rgba32ui, binding = 0) readonly uniform uimage2D frame_buffer;

struct cell_data_t {
	vec4 position;
	vec4 color;
	vec4 meta;
};

layout(std430, binding = 0) readonly buffer cells {
	cell_data_t data[];
};

in vec2 WinK;
out vec4 pixel;
const uint null_id = uint(-1);

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
	// „аша и фон
	pixel = vec4(render_background(mix(ViewWorld.xy, ViewWorld.zw, WinK)), 1.);
	uvec4 c_meta = imageLoad(frame_buffer, ivec2(gl_FragCoord.xy));
	
	if (c_meta.x != null_id) {
		vec3 g_color = data[c_meta.x].color.rgb;
		float r1 = sqrt(float(c_meta.z) / 2147483648.);
		float r2 = sqrt(float(c_meta.w) / 2147483648.);
		float r = mix(0., max(r1, r1 / r2), step(0.2, r1));
		vec4 out_cell_color = mix(vec4(g_color * 0.5, 0.8), vec4(g_color, 0.5), step(0.2, r) * step(r, 0.9));
		pixel = vec4(mix(pixel.rgb, out_cell_color.rgb, out_cell_color.a), 1.);
	}
}