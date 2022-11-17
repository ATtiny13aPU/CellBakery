#version 430 core

uniform float fTime;

in vec2 dp; // Дельта позиции относительно центра в мировых координатах
flat in int c_id;

out vec4 pixel;

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

uint ihash2(uvec2 s) {
	uint s1 = ((s.y ^ s.x) * 0xEC7269E5u + 0x4C8A248Du) ^ ((s.x >> 5u) * 0xC5EB9396u);
	s.x = ((s1 / 867u) * (s.x * 0x6C8FBCAFu)) ^ (s1 * (s.y & 0xF465C8F3u));
	
	return s.x;
}


const float t2kr[18] = float[](1., 1., 1., 1.3, 1., 1., 1., 1.3, 1.2, 1.15, 1.15, 1.15, 1., 1., 1., 1., 1., 1.1);

void main() {
	int cid = c_id;
	float qd = dot(dp, dp); // вкадрат расстояния до целевой клетки
	float c_r = cells[cid].radius;
	float qr = c_r * c_r; // вкадрат радиуса целевой клетки
	int t = cells[cid].type_id;

	if (qd < qr) {
		float r = length(dp / cells[cid].radius); // относительный радиус
		float a;
		int b = 0;

		// Тип структуры
		switch (t) {
			case(0): // фагоцит
			case(1): // жгутоцит
			case(3): // девороцит  (fract(a * 19.) > 0.8);
				b = int(r < 0.20 || r > 0.90);
				break;
			case(2): // фотоцит
				if (r < 0.20 || r > 0.90)
					b = 1;
				else {
					a = fract(atan(-dp.y, -dp.x) / 6.2831853 - cells[cid].angle);
					b = int(ihash2(uvec2(r * 5., a * 20.)) & 4u);
				}
				break;
			case(4): // липоцит
				b = (r < 0.20 || r > 0.93) ? 1 : int(r < 0.85) * 2;
				break;
			case(5): // кератиноцит
				b = int((r < 0.2) || ((r > 0.7) && (r < 0.8)) || (r > 0.9));
				break;
			default:
				b = int(r > 0.9);
		}

		// Тип заливки
		switch (b) {
			case(4): // хлоропласт
				pixel = vec4(normalize(cells[cid].color_rgb * vec3(0.5, 1.3, 0.3)) * 0.5, 0.90);
				break;
			case(3): // пустота
				discard;
			case(2): // жир липоцита
				pixel = vec4((cells[cid].color_rgb + vec3(0.5, 0.25, 0.25) * 3.) * 0.25, 0.90);
				break;
			case(1): // тёмная заливка
				pixel = vec4(cells[cid].color_rgb * 0.5, 1.);
				break;
			default: // светлая заливка
				pixel = vec4(cells[cid].color_rgb, 0.67);
		}

	} else if (qd < qr * t2kr[t] * t2kr[t] || t == 1) {
		float r = length(dp / c_r); // относительный радиус
		float a = fract(atan(-dp.y, -dp.x) / 6.2831853 - cells[cid].angle - 0.5);
		int b = 0;
		
		// Тип структуры
		switch (t) {
			case(1): // жгутоцит
				b = int(abs(0.5 - a) - 0.02 < 0.);
				break;
			case(3): // девороцит
				a = abs(fract(a * 19.) - 0.5);
				b = int((a - sqrt(r) * 1.2 > -0.865) || (1. / (0.85 - a) - (sqrt(r) - 1.) * 8. > 1.8));
				//b = int(mix(1. / (0.9 - a) - (r - 1.) * 3. + 1.7, a - sqrt(r) * 1.2 - 0.865, 0.5) > 0.79);
				//b = int((a - sqrt(r) * 1.2 > -0.865) || (a - r * 2. > -1.73));
				//b = int(1. / (0.9 - a) - (r - 1.) * 3. - 1.7 > 0. || a - sqrt(r) * 1.2 - 0.865 > 0.);
				break;
			case(8): // вироцит
				b = int(fract(a * 17.) - r * 0.1 > 0.7); //int((a + (0.1 / r) > 0.5)
				break;
		}

		// Тип заливки
		switch (b) {
			case(2): // тёмная заливка
				pixel = vec4(cells[cid].color_rgb * 0.5, 1.);
				break;
			case(1): // тёмная заливка
				pixel = vec4(cells[cid].color_rgb * 0.5, 1.);
				break;
			default: // пустота
				pixel = vec4(cells[cid].color_rgb * 0.5, 0.2);
				//discard;
		}
	} else discard;
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