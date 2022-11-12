#version 330 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in int id[];

uniform vec4 ViewWorld;
uniform ivec2 WinSize;
uniform isampler2D Collision;
uniform isampler2D CollisionData;
uniform sampler2D CellsPos;
uniform sampler2D CellsMeta;
uniform float fTime;

uniform int GM;

out vec2 Pos;
out vec4 c_meta;
out vec4 c_pos;

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
	vec4 cell_pos_data = texelFetch(CellsPos, ivec2(cid & 0xFFF, cid / 0x1000), 0);

	// отсекаем вне экрана
	if (lengthRect(ViewWorld.xy, ViewWorld.zw, cell_pos_data.xy) < cell_pos_data.z) {
		vec2 WinK = (cell_pos_data.xy - ViewWorld.xy) / (ViewWorld.zw - ViewWorld.xy) * 2. - 1.;
		vec2 WinR = cell_pos_data.z / (ViewWorld.zw - ViewWorld.xy);

		vec4 cell_meta = texelFetch(CellsMeta, ivec2(cid & 0xFFF, cid / 0x1000), 0);

		gl_Position.zw = vec2(0., 1.);
		c_meta = cell_meta;
		c_pos = cell_pos_data;

		int t = int(c_meta.a * 256.);
		float r = cell_pos_data.z * t2kr[t];
		WinR *= t2kr[t];

		// 4-угольник
		if (GM == 0) {
			if(t != 1) {
				vec2 p1 = WinK - WinR, p2 = WinK + WinR;

				// 1:bottom-left
				Pos = cell_pos_data.xy - r;
				gl_Position.xy = p1;
				EmitVertex();
				// 2:bottom-right
				Pos = vec2(cell_pos_data.x + r, cell_pos_data.y - r);
				gl_Position.xy = vec2(p2.x, p1.y);
				EmitVertex();
				// 3:top-left
				Pos = vec2(cell_pos_data.x - r, cell_pos_data.y + r);
				gl_Position.xy = vec2(p1.x, p2.y);
				EmitVertex();
				// 4:top-right
				Pos = cell_pos_data.xy + r;
				gl_Position.xy = p2;
				EmitVertex();
			} else {
				float kWR = WinR.x / WinR.y;
				WinR.x /= kWR;

				WinR *= rot(-c_pos.a * 3.14159265358 * 2.);
				vec2 vr = vec2(r) * rot(-c_pos.a * 3.14159265358 * 2.);

				// 1:bottom-left
				Pos = cell_pos_data.xy - vr;
				gl_Position.xy = WinK - vec2(WinR.x * kWR, WinR.y);
				EmitVertex();
				// 2:bottom-right
				Pos = cell_pos_data.xy + vec2(vr.y, -vr.x);
				gl_Position.xy = WinK + vec2(WinR.y * kWR, -WinR.x);
				EmitVertex();
				// 3:top-left
				Pos = cell_pos_data.xy + vec2(-vr.y, vr.x);
				gl_Position.xy = WinK + vec2(-WinR.y * kWR, WinR.x);
				EmitVertex();
				// 4:top-right
				Pos = cell_pos_data.xy + vr * 2.;
				gl_Position.xy = WinK + vec2(WinR.x * kWR, WinR.y) * 2.;
				EmitVertex();
			}
		}
		// 3-угольник
		else {
			// 1:bottom-left
			Pos = vec2(cell_pos_data.x - r * 1.7320508, cell_pos_data.y - r);
			gl_Position.xy = vec2(WinK.x - WinR.x * 1.7320508, WinK.y - WinR.y);
			EmitVertex();
			// 2:bottom-right
			Pos = vec2(cell_pos_data.x + r * 1.7320508, cell_pos_data.y - r);
			gl_Position.xy = vec2(WinK.x + WinR.x * 1.7320508, WinK.y - WinR.y);
			EmitVertex();
			// 4:top
			Pos = vec2(cell_pos_data.x, cell_pos_data.y + r * 2.);
			gl_Position.xy = vec2(WinK.x, WinK.y + WinR.y * 2.);
			EmitVertex();
		}
		EndPrimitive();

	}
}