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

class CellCS {
public:
	frac angle;
	frac r;
	vec2 pos;
	ivec2 ipos;
	int32_t mapID;
	int32_t buffID;
	int32_t buffID2;
	int32_t buffCount;
	vec3 color;
	uint8_t type; // CellType

	CellCS() {
		pos = vec2(0.);
		r = 0.;
	}
private:
};

class WorldCS {
public:
	struct View {
		vec2 Pos = vec2(0, 0); // Позиция центра камеры (0 - центр субстрата)
		frac mst = 1.; // Масштаб зрения (чем больше, тем меньше выглядит субстрат)
	};
	View view;
	class Setup {
	public:
		std::string name = "Unnamed";
		frac Dp; // Диаметр чаши в мм при создании
		frac Ac = 0.03; // Размер сетки мира (не трогать внешними методами, только для чтения)
		int out_mapSize = 1;
		int out_maxСollisionData = 1;
		int set_max_ec = 100000;
		int set_max_efp = 10000;
		uint64_t main_seed = 0x12067B01A37EE0E2;
	};
	Setup setup;
	class Control {
	public:
		int out_ready = 0; // готовность данных мира к использованию внешним кодом
		int out_safe_shutdown = 0; // готовность к выключению потока мира
		int out_run_step = 0; // колво запрошенных шагов в очереди
		bool in_auto_run_mode = 0; // после первого вызова world.run_step(1); мир будет обрабатываться автономно не дожидаясь ответа
		uint64_t out_counter_of_step = 0; // колво шагов с момента старта симуляции

		void shutdown() { p_shutdown = 1; } // указать завершение работы

		int out_debug1 = 0; // буфферная переменная для дебага

		int p_start = 0;
		int p_shutdown = 0;
	};
	Control control;
	
	void RenderWorld();
	void RunWorld();
	void newCell(int);
	bool newCell(vec2, frac);
	void newCell();

	double debugF1, debugF2, debugF3;
private:
	Randomaizer RAND;
	int ec = 0;
	int mec;
	int mefp;
	frac Rp, Dp, Rc, Ac, hdAc;
};

void WorldCS::RunWorld() {
}


void WorldCS::RenderWorld() {
	RAND.ini(setup.main_seed);
	std::ostringstream buff;

	Rp = setup.Dp / 2.; // Радиус чаши в мм
	Dp = Rp * 2; // Диаметр чаши в мм
	view.mst *= Dp;
	view.Pos = vec2(Dp / 2.);

	int Dm = floor(Dp / setup.Ac / 4.) * 4;

	Rc = setup.Ac;
	setup.Ac = Dp / Dm;
	Ac = setup.Ac;
	hdAc = 1. / Ac;
	setup.out_mapSize = Dm;


	mec = setup.set_max_ec;
	setup.set_max_ec = ceil(mec / 4096.) * 4096;
	mec = setup.set_max_ec;
	setup.out_maxСollisionData = ceil(mec * 20. / 4096.) * 4096;
	int mecl = setup.out_maxСollisionData;
	
	mefp = setup.set_max_efp;

	std::chrono::time_point<std::chrono::steady_clock> benchPoint[10];
	benchPoint[0] = std::chrono::steady_clock::now();

	benchPoint[3] = std::chrono::steady_clock::now();

	debugF1 = std::chrono::duration<double, std::milli>(benchPoint[1] - benchPoint[0]).count();
	debugF2 = std::chrono::duration<double, std::milli>(benchPoint[2] - benchPoint[1]).count();
	debugF3 = std::chrono::duration<double, std::milli>(benchPoint[3] - benchPoint[2]).count();

	control.out_ready = 1; // разрешаем графике идти дальше
	// главный цикл рендера мира
	while (control.p_shutdown == 0) {

		if (control.out_run_step == 0)
			std::this_thread::sleep_for(std::chrono::microseconds(10));

		while (control.out_run_step > 0) {
			benchPoint[1] = std::chrono::steady_clock::now();
			benchPoint[2] = std::chrono::steady_clock::now();

			debugF3 = std::chrono::duration<double, std::milli>(benchPoint[2] - benchPoint[1]).count();

			control.out_counter_of_step++;
			control.out_run_step--;
		}
	}
	control.out_safe_shutdown = 1;
}