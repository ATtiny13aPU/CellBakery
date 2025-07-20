#version 430 core

#include "structs.glsl"


const int nullID = 0x7FFFFFFF;

uniform float sqrOnePixelRadius;
uniform ivec2 WinSize;
uniform vec4 ViewWorld;
flat in int c_id;

in vec2 dp;	// Дельта позиции относительно центра в относительных координатах

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