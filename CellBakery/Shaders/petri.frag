#version 430 core
#extension GL_ARB_fragment_shader_interlock : enable

layout(rgba32ui, binding = 0) readonly uniform uimage2D frame_buffer;

struct cell_data_t {
	vec4 position;
	vec4 color;
	vec4 meta;
};
/*
	gl_Position = vec4(position.xy, 0.0, 1.0);
	v_color = color.rgb;
	v_radius = color.a;
	v_vel = position.zw;
*/

layout(std430, binding = 0) readonly buffer cells {
	cell_data_t data[];
};

in vec2 WinK;
out vec4 pixel;
const uint null_id = uint(-1);

uniform vec4 ViewWorld;
uniform vec2 WinSize;
uniform float TimeLerp;
uniform float MSAA;
uniform float MSAA_quasi_start;

vec2 quasi_random(float i) {
	const float a1 = 0.7548776662;
	const float a2 = 0.5698402909;
	return fract(vec2(a1, a2) * i + vec2(0.5));
}

float grid1(vec2 uv) {
	uv = fract(uv * 0.25);
	return abs(step(uv.x, 0.5) - step(uv.y, 0.5));
}

// Функция отрисовывает чашу и фон (освещение в будущем)
vec3 render_background(const vec2 w_uv) {
	vec3 pixel;


	float ling = 0.3;

	ling *= sqrt(ling);

	vec3 ling_color1 = clamp(mix(vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3), vec3(1.), ling * ling * 0.5), 0., 1.);
	vec3 ling_color2 = clamp(vec3(0.745 + ling / 2., 0.745 + ling / 5., 1. - ling * ling * 0.3) * 0.5, 0., 1.);

	pixel = ling_color1;// * step(fract(w_uv.y * 0.5), 0.5) * 0.02 + 0.1;

	return pixel;
}


// Функция отрисовывает геометрию клеток
vec4 render_cells(const uvec4 pixel_meta, const cell_data_t cell_1, const cell_data_t cell_2, const vec2 w_uv) {
	vec4 pixel = vec4(0.);

	
	cell_data_t cell;
	float r;
	if (pixel_meta.y == null_id) {
		r = distance(cell_1.position.xy, w_uv) / cell_1.color.a * 2.;
		cell = cell_1;
	} else {
		float r1 = distance(cell_1.position.xy, w_uv) / cell_1.color.a * 2.;
		float r2 = distance(cell_2.position.xy, w_uv) / cell_2.color.a * 2.;
		if (r1 < r2) {
			cell = cell_1;
		} else {
			r = r2; r2 = r1; r1 = r;
			cell = cell_2;
		}
		r = mix(0., max(r1, r1 / r2), step(0.2, r1));
	}
	if (r > 1.)
		return pixel;
	pixel = mix(vec4(cell.color.rgb * 0.5, 0.8), vec4(cell.color.rgb, 0.5), step(0.2, r) * step(r, 0.9));
	return pixel;
}



void main() {
	vec2 world_uv = mix(ViewWorld.xy, ViewWorld.zw, WinK);
	vec2 world_pixel_vector = (ViewWorld.zw - ViewWorld.xy) / WinSize;
	ivec2 i_uv = ivec2(gl_FragCoord.xy);
	// пытаемся вынести дорогую операцию чтения за пределы цикла
	const uvec4 pixel_meta = imageLoad(frame_buffer, i_uv);

	// Чаша и фон
	pixel = vec4(render_background(world_uv), 1.);

	// Если клеток нет, то и нечего рисовать
	if (pixel_meta.x == null_id)
		return;
	cell_data_t cell_1 = data[pixel_meta.x];
	cell_1.position.xy += cell_1.position.zw * (TimeLerp / 20.);

	// Вторая клетка в случае отсутствия информации заполняется значениями по умолчанию
	cell_data_t cell_2 = (pixel_meta.y == null_id) 
		? cell_data_t(vec4(0.), vec4(0.0), vec4(0.0)) // Значения можно настроить при надобности
		: data[pixel_meta.y];
	cell_2.position.xy += cell_2.position.zw * (TimeLerp / 20.);

	float start = (gl_FragCoord.x + gl_FragCoord.y * WinSize.x) * MSAA_quasi_start;
	// Клетка с MSAA
	vec4 cell_pixel = vec4(0.);
	for (float x = 0; x < MSAA; x += 1.)
		cell_pixel += render_cells(pixel_meta, cell_1, cell_2, world_uv + (quasi_random(x + start) - 0.5) * world_pixel_vector);
	cell_pixel /= MSAA;

	// Смешивание
	pixel = vec4(mix(pixel.rgb, cell_pixel.rgb, cell_pixel.a), 1.);
}