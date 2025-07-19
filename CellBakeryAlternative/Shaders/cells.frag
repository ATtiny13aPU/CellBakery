#version 430 core

#include "structs.glsl"
/*
uniform float fTime;
uniform ivec2 WinSize;
uniform float sqrOnePixelRadius;
in vec2 dp; // Дельта позиции относительно центра в мировых координатах
flat in int c_id;

restrict buffer ssbo_cellspp {
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

uint ihash2(uvec2 s) {
	uint s1 = ((s.y ^ s.x) * 0xEC7269E5u + 0x4C8A248Du) ^ ((s.x >> 5u) * 0xC5EB9396u);
	s.x = ((s1 / 867u) * (s.x * 0x6C8FBCAFu)) ^ (s1 * (s.y & 0xF465C8F3u));
	
	return s.x;
}


const float t2kr[18] = float[](1.02, 1., 1., 1.3, 1., 1., 1., 1.3, 1.2, 1.15, 1.15, 1.15, 1., 1., 1., 1., 1., 1.1);
*/

const int nullID = 0x7FFFFFFF;

uniform float sqrOnePixelRadius;
uniform ivec2 WinSize;
uniform vec4 ViewWorld;
flat in int c_id;

in vec2 dp;	// Дельта позиции относительно центра в относительных координатах

//struct Cell {
//	ivec2 ipos;
//	vec2 pos;
//	float radius;
//	float angle;
//	float rotate_vel;
//	vec3 color_rgb;
//	vec3 color_hsv;
//	int type_id;
//	int linked_list;
//	int chunk_id;
//	int is_first;
//	float weight;
//	vec2 velocity;
//	ivec2 force;
//};

readonly buffer ssbo_cells {
    Cell cells[];
};

restrict buffer ssbo_cellspp {
    int nearest[];
};

void main() {
	
	// если клетка предположительно должна быть нарисована в этом фрагменте
	if (dot(dp, dp) < 1.) {
		int lastID = nullID, lastID2;
		int shift = 0;
		vec2 Pos = mix(ViewWorld.xy, ViewWorld.zw, gl_FragCoord.xy / vec2(WinSize));
		float l1 = length(Pos - cells[c_id].pos - cells[c_id].ipos) / cells[c_id].radius;
		float l2;
		int nindex = (int(gl_FragCoord.x) + int(gl_FragCoord.y) * WinSize.x) * 2;
		int correntId = c_id;
		do {
			lastID2 = lastID;
			// если у фрагмента nullID или если мы ближе и проходим повторную проверку, пробуем свапнуть, если ID не поменялся
			lastID = atomicCompSwap(nearest[nindex + shift], lastID2, correntId);
			// если полученное ID не изменилось, значит запись прошла успешно
			if (lastID == lastID2) {
				correntId = lastID;
				l1 = length(Pos - cells[correntId].pos - cells[correntId].ipos) / cells[correntId].radius;
				l2 = l1;
				shift++;
			} else
				l2 = length(Pos - cells[lastID].pos - cells[lastID].ipos) / cells[lastID].radius;
			// если мы находимся дальше текущей клетки, чьё id установлено, пытаемся делать сравнения со следующими позициями
			if (l1 > l2) {
				shift++;
				lastID = nullID;
				lastID2 = nullID;
			}
		} while (shift < 2);
	}
	discard;
}

/*
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
};*/