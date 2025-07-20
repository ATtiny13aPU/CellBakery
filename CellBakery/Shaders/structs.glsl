
const float t2kr[18] = float[](1.02, 1., 1., 1.3, 1., 1., 1., 1.3, 1.2, 1.15, 1.15, 1.15, 1., 1., 1., 1., 1., 1.1);

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


struct Cell {
	// Общие данные
	int type_id;
	ivec2 ipos;
	vec2 pos;
	float radius;
	float angle;
	float rotate_vel;
	vec3 color_rgb;
	vec3 color_hsv;

	// Данные для симуляции
	int chunk_id; // мемоизация для ipos.x + ipos.y * Dm
	int linked_list;
	int is_first;
	float weight;
	vec2 velocity;
	ivec2 force;
	// Данные для визуализации
	vec2 visual_force;
};