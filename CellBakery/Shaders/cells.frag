#version 430 core
#extension GL_ARB_fragment_shader_interlock : enable

layout(rgba32ui, binding = 0) coherent uniform uimage2D frame_buffer;

in vec2 dp;
in flat uint id;
const uint null_id = uint(-1);

void main() {
	float qd = dot(dp, dp);
	beginInvocationInterlockARB();
	if (qd < 1.) {
		uvec4 pixel = imageLoad(frame_buffer, ivec2(gl_FragCoord.xy));
		uint my_distance = uint(qd * 2147483648.);
		/*
			in_data содержит состояние хранилища до вызова критической секции фрагмента
			при первом вызове там будет содержаться uvec4(-1) aka uvec4(0xFFFFFFFFu)
			вход в секцию записи имеет смысл только если:
			1) второй индекс ещё не заполнен
			2) наше расстояние меньше второго расстояния
			данные хранятся как uvec4(два id, две дистанции)
		*/
		if (pixel.y == null_id || my_distance < pixel.w) {
			pixel = my_distance < pixel.z ? 
				uvec4(id, pixel.x, my_distance, pixel.z) :
				uvec4(pixel.x, id, pixel.z, my_distance);
			imageStore(frame_buffer, ivec2(gl_FragCoord.xy), pixel);
		}
	}
	endInvocationInterlockARB();
}