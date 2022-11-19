#version 430 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in int id[];

uniform vec4 ViewWorld;
uniform ivec2 WinSize;
uniform float fTime;

uniform int GM;

out vec2 dp;
flat out int c_id;

struct Cell {
	ivec2 ipos;
	vec2 pos;
	float radius;
	float angle;
	vec3 color_rgb;
	vec3 color_hsv;
	int type_id;
	int linked_list;
	int is_first;
	float weight;
	vec2 velocity;
	vec2 force;
};

readonly buffer ssbo_cells {
    Cell cells[];
};


mat2 rot(float a) { //матрица поворота по заданному углу
	float s = sin(a);
	float c = cos(a);
	return mat2(c, -s, s, c);
}

float lengthRect(vec2 tl, vec2 br, vec2 uv) {
	vec2 d = max(tl - uv, uv - br);
	return length(max(vec2(0.0), d)) + min(0.0, max(d.x, d.y));
}

struct CellType {
int
	Phago,		// Фагоцит 0
	Flagello,	// Жгутоцит 1
	Photo,		// Фотоцит 2
	Devoro,		// Девороцит 3
	Lipo,		// Липоцит 4
	Keratino,	// Кератиноцит 5
	Buoyo,		// Буецит 6
	Glueo,		// Клейкоцит 7
	Viro,		// Вироцит 8
	Nitro,		// Нитроцит 9
	Stereo,		// Стереоцит 10
	Senseo,		// Сенсеоцит 11
	Myo,		// Миоцит 12
	Neuro,		// Нейроцит 13
	Secro,		// Секроцит 14
	Stemo,		// Стволоцит 15
	Gamete,		// Гамета 16
	Cilio;		// Цилиоцит 17
};

const float t2kr[18] = float[](1., 1., 1., 1.3, 1., 1., 1., 1.3, 1.2, 1.15, 1.15, 1.15, 1., 1., 1., 1., 1., 1.1);

void main() {
	int cid = id[0];
	
	vec2 pos = cells[cid].pos + cells[cid].ipos;
	float r = cells[cid].radius;

	// отсекаем вне экрана
	if (lengthRect(ViewWorld.xy, ViewWorld.zw, pos) < cells[cid].radius) {
		vec2 WinK = (pos - ViewWorld.xy) / (ViewWorld.zw - ViewWorld.xy) * 2. - 1.;
		vec2 WinR = cells[cid].radius / (ViewWorld.zw - ViewWorld.xy);

		gl_Position.zw = vec2(0., 1.);
		
		//type_id = cells[cid].type_id;
		c_id = cid;

		//c_meta = cells[cid].color_rgb;
		//c_pos = vec4(pos, r, cells[cid].angle);

		int t = cells[cid].type_id;
		r *= t2kr[t];
		WinR *= t2kr[t];

		// 4-угольник
		if(t != 1) {
			vec2 p1 = WinK - WinR, p2 = WinK + WinR;

			// 1:bottom-left
			dp = vec2(-r);
			gl_Position.xy = p1;
			EmitVertex();
			// 2:bottom-right
			dp = vec2(r, -r);
			gl_Position.xy = vec2(p2.x, p1.y);
			EmitVertex();
			// 3:top-left
			dp = vec2(-r, r);
			gl_Position.xy = vec2(p1.x, p2.y);
			EmitVertex();
			// 4:top-right
			dp = vec2(+r);
			gl_Position.xy = p2;
			EmitVertex();
		} else {
			float kWR = WinR.x / WinR.y;
			WinR.x /= kWR;

			mat2x2 mr = rot((cells[cid].angle - 0.625) * 3.14159265358 * 2.);
			WinR *= mr;
			vec2 vr = vec2(r) * mr;

			// 1:bottom-left
			dp = vec2(-vr);
			gl_Position.xy = WinK - vec2(WinR.x * kWR, WinR.y);
			EmitVertex();
			// 2:bottom-right
			dp = vec2(vr.y, -vr.x);
			gl_Position.xy = WinK + vec2(WinR.y * kWR, -WinR.x);
			EmitVertex();
			// 3:top-left
			dp = vec2(-vr.y, vr.x);
			gl_Position.xy = WinK + vec2(-WinR.y * kWR, WinR.x);
			EmitVertex();
			// 4:top-right
			dp = vec2(vr * 2.);
			gl_Position.xy = WinK + vec2(WinR.x * kWR, WinR.y) * 2.;
			EmitVertex();
		}
		EndPrimitive();

	}
}