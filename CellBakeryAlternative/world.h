#pragma once

using namespace osl;

enum CellType {
	Phago,		// Фагоцит
	Flagello,	// Жгутоцит
	Photo,		// Фотоцит
	Devoro,		// Девороцит
	Lipo,		// Липоцит
	Keratino,	// Кекатиноцит
	Buoyo,		// Буецит
	Glueo,		// Клейкоцит
	Viro,		// Вироцит
	Nitro,		// Нитроцит
	Stereo,		// Стереоцит
	Senseo,		// Сенсеоцит
	Myo,		// Миоцит
	Neuro,		// Нейроцит
	Secro,		// Секроцит
	Stemo,		// Стволоцит
	Gamete,		// Гамета
	Cilio		// Цилиоцит
};

class WorldCS {
public:
	struct View {
		vec2 Pos = vec2(0, 0); // Позиция центра камеры (0 - центр субстрата)
		frac mst = 1.02; // Масштаб зрения (чем больше, тем меньше выглядит субстрат)
	};
	View view;

	std::string name = "Unnamed";
	frac Dp; // Диаметр чаши в мм при создании
	int Dm = 1;
	int mec = 100000; // максимальное число клеток
	int mefp = 10000; // максимальное число свободных частиц
	
	void iniWorld();

private:
};

void WorldCS::iniWorld() {
	frac Rp = Dp / 2.;
	view.mst *= Dp;
	view.Pos = vec2(Dp / 2.);

	Dm = ceil(Dp);

	mec = ceil(mec / 1024.) * 1024;
}