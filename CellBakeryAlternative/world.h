#pragma once

using namespace osl;

enum CellType {
	Phago,		// �������
	Flagello,	// ��������
	Photo,		// �������
	Devoro,		// ���������
	Lipo,		// �������
	Keratino,	// �����������
	Buoyo,		// ������
	Glueo,		// ���������
	Viro,		// �������
	Nitro,		// ��������
	Stereo,		// ���������
	Senseo,		// ���������
	Myo,		// ������
	Neuro,		// ��������
	Secro,		// ��������
	Stemo,		// ���������
	Gamete,		// ������
	Cilio		// ��������
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
		vec2 Pos = vec2(0, 0); // ������� ������ ������ (0 - ����� ���������)
		frac mst = 1.; // ������� ������ (��� ������, ��� ������ �������� ��������)
	};
	View view;
	class Setup {
	public:
		std::string name = "Unnamed";
		frac Dp; // ������� ���� � �� ��� ��������
		frac Ac = 0.03; // ������ ����� ���� (�� ������� �������� ��������, ������ ��� ������)
		int out_mapSize = 1;
		int out_max�ollisionData = 1;
		int set_max_ec = 1000;
		int set_max_efp = 10000;
		uint64_t main_seed = 0x12067B01A37EE0E2;
	};
	Setup setup;
	class Control {
	public:
		int out_ready = 0; // ���������� ������ ���� � ������������� ������� �����
		int out_safe_shutdown = 0; // ���������� � ���������� ������ ����
		int out_run_step = 0; // ����� ����������� ����� � �������
		bool in_auto_run_mode = 0; // ����� ������� ������ world.run_step(1); ��� ����� �������������� ��������� �� ��������� ������
		uint64_t out_counter_of_step = 0; // ����� ����� � ������� ������ ���������

		void shutdown() { p_shutdown = 1; } // ������� ���������� ������

		int out_debug1 = 0; // ��������� ���������� ��� ������

		int p_start = 0;
		int p_shutdown = 0;
	};
	Control control;
	int run_step(int n); // ��������� ��������� �� ����������� ����������� �����
	void RenderWorld();
	void RunWorld();
	void newCell(int);
	bool newCell(vec2, frac);
	void newCell();
	const int max_collision = 30; // ����������� ���������� �������� �������� �� ������
	std::vector<int32_t> collision_data;
	int32_t* collision_map;
	uint8_t* light_map;
	CellCS* cells;

	uint8_t* buffByteMetaCells;
	float* buffFloatPosCells;
	double debugF1, debugF2, debugF3;
private:
	Randomaizer RAND;
	int ec = 0;
	int mec;
	int mefp;
	frac Rp, Dp, Rc, Ac, hdAc;
	static const int32_t nullID = 0x7FFFFFFF;
	static const int32_t nullCD = 0x7FFFFFFE;
};

int WorldCS::run_step(int n) {

	if (control.out_run_step > 9)
		return(1); // �����������
	else
		control.out_run_step += n;

	return(0); // �� ������
}

void WorldCS::RunWorld() {
}

inline void compute_lightMap(uint8_t* light_map, int Dm, frac Dp, uint64_t time) {
	for (int y = 0; y < Dm; y++) {
		for (int x = 0; x < Dm; x++) {
			vec2 fPos = vec2(x / (Dm - 1.), y / (Dm - 1.)) * Dp;
			light_map[x + y * Dm] = (sin(length(vec2(fPos - Dp / 2.)) * 10. - time / 10.) + 1.) * mix(length(vec2(fPos / Dp - 0.5)), 1., 0.6) * 150.;
			//light_map[x + y * Dm] = (1. - abs(fPos.x + fPos.y - Dp)) * 250.;
			//light_map[x + y * Dm] = ((x + y) & 1) * 100 + 50;
		}
	}
}

inline void WorldCS::newCell(int c1) { // �������������� �������� ������

}

inline bool WorldCS::newCell(vec2 pos, frac r) { // �������� ������ ��������� ������ � ����� ������� ������
	if (ec < mec) {
		cells[ec].pos = pos;
		cells[ec].r = r;
		cells[ec].angle = (rand() % 3001) / 3000.;
		cells[ec].buffID = nullID;
		cells[ec].color = vec3(RAND.pd(), RAND.pd() * 0.5 + 0.5, RAND.pd() * 0.5 + 0.5);
		ec++;
		return true;
	}
	return false;
}

inline void WorldCS::newCell() { // �������� ��������� ������
	if (ec < mec) {
		vec2 pos = -vec2(Dp);
		while (!compareDistanse(pos - Rp, Rp))
			pos = vec2(RAND.nd(), RAND.nd()) * Dp;
		frac r = remap(RAND.pd(), 0., 1., Rc * 0.6, Rc * 1.);
		newCell(pos, r);
	}
}

void WorldCS::RenderWorld() {
	RAND.ini(setup.main_seed);
	std::ostringstream buff;

	Rp = setup.Dp / 2.; // ������ ���� � ��
	Dp = Rp * 2; // ������� ���� � ��
	view.mst *= Dp;
	view.Pos = vec2(Dp / 2.);

	int Dm = floor(Dp / setup.Ac / 4.) * 4;

	Rc = setup.Ac;
	setup.Ac = Dp / Dm;
	Ac = setup.Ac;
	hdAc = 1. / Ac;
	setup.out_mapSize = Dm;

	light_map = new uint8_t[Dm * Dm];
	compute_lightMap(light_map, Dm, Dp, 20);

	int32_t* full_collision_map = new int32_t[(Dm + 2) * (Dm + 2)];
	collision_map = &full_collision_map[Dm];
	for (int i = 0; i < (Dm + 2) * (Dm + 2); i++)
		full_collision_map[i] = nullID; // *(RAND.u8() % 40 != 0);

	mec = setup.set_max_ec;
	setup.set_max_ec = ceil(mec / 4096.) * 4096;
	mec = setup.set_max_ec;
	setup.out_max�ollisionData = ceil(mec * 20. / 4096.) * 4096;
	int mecl = setup.out_max�ollisionData;
	collision_data.resize(mecl);
	for (int c = 0; c < mecl; c++)
		collision_data[c] = nullID;
	/* ������ ������ ��������
	���-�� ������ ������� ������
	���-�� ������ ������ ������
	�� ���������� ������ ������� ������ ��������������� � �����������. � ����� ���������� � ������ �����
	�� ���������� ������� ������ ������ id ������ � ������� ������ � ����������� ������ �� �� �������� � ��������
	*/
	mefp = setup.set_max_efp;
	cells = new CellCS[mec];
	int32_t* mainCellID = new int32_t[mec];
	buffFloatPosCells = new float[mec * 4];
	buffByteMetaCells = new uint8_t[mec * 4];
	for (int c = 0; c < mec; c++) { // ������������� ������ �� �����
		newCell();
		buffFloatPosCells[c * 4] = cells[c].pos[0];
		buffFloatPosCells[c * 4 + 1] = cells[c].pos[1];
		buffFloatPosCells[c * 4 + 2] = cells[c].r;
		buffFloatPosCells[c * 4 + 3] = cells[c].angle;
		vec3 rgbc = HSV2RGB(cells[c].color) * 255.;
		buffByteMetaCells[c * 4] = rgbc[0];
		buffByteMetaCells[c * 4 + 1] = rgbc[1];
		buffByteMetaCells[c * 4 + 2] = rgbc[2];
	}
	for (int c = 0; c < mec; c++)
		buffByteMetaCells[c * 4 + 3] = RAND.u8() % CellType::Buoyo;


	int vectorID[8];
	vectorID[0] = -Dm - 1;
	vectorID[1] = -Dm;
	vectorID[2] = -Dm + 1;
	vectorID[3] = -1;
	vectorID[4] = +1;
	vectorID[5] = Dm - 1;
	vectorID[6] = Dm;
	vectorID[7] = Dm + 1;

	std::chrono::time_point<std::chrono::steady_clock> benchPoint[10];
	benchPoint[0] = std::chrono::steady_clock::now();

	// �������� ���������
	{
		// ������ ������ ���� ����� �� ������ ����, ��� �������� ������� ������
		for (int c = 0; c < mec; c++) {
			CellCS &cell = cells[c];
			cell.ipos = ivec2(cell.pos * hdAc);
			cell.mapID = cell.ipos[0] + cell.ipos[1] * Dm;
			cell.buffID = collision_map[cell.mapID];
			collision_map[cell.mapID] = c;
		}
		benchPoint[1] = std::chrono::steady_clock::now();
		int emcID = 0;
		// ������� ������ ���������� ������ � ���������� id �� ���� ������
		if (1)
			for (int c = 0; c < mec; c++)
				if (collision_map[cells[c].mapID] == c) {
					CellCS &cell = cells[c];
					mainCellID[emcID] = c;
					emcID++;
					int i = 1;
					int32_t c1 = cell.buffID;
					while (c1 != nullID) {
						i++;
						c1 = cells[c1].buffID;
					}
					cell.buffCount = i;
				}
		benchPoint[2] = std::chrono::steady_clock::now();


		// ������������ ������� ������� ������ � ���������
		if (1) {
			int cID = 0; // ��������� ��������� �� ������ ��������
			for (int mc = 0; mc < emcID; mc++) {
				int32_t mcID = mainCellID[mc];
				CellCS &cell = cells[mcID];
				int Ni = cell.buffCount, No = 0;
				int lcID = 2; // ��������� ��������� �������� �������������� ������
				for (int v = 0; v < 8; v++) {
					int32_t mcID1 = collision_map[cell.mapID + vectorID[v]];
					if (mcID1 != nullID) {
						if (mcID1 != nullCD) {

							CellCS &cell2 = cells[mcID1];
							No += cell2.buffCount; // ��������� ������ �������� ������
							int32_t c1 = mcID1;
							do {
								collision_data[cID + lcID] = c1;
								c1 = cells[c1].buffID;
								lcID++;
							}
							while (c1 != nullID);
						}
					}
					else {
						collision_map[cell.mapID + vectorID[v]] = nullCD;
					}
				}
				collision_data[cID] = No; // ���������� � ������� 0 ���-�� ������� ������������ ������
				collision_data[cID + 1] = Ni; // ���������� � ������� 1 ���-�� ���������� ������


				int32_t c1 = mcID;
				do {
					collision_data[cID + lcID] = c1;
					c1 = cells[c1].buffID;
					lcID++;
				}
				while (c1 != nullID);
				cell.buffID2 = cID;
				cID += lcID;
			}
			for (int mc = 0; mc < emcID; mc++)
				collision_map[cells[mainCellID[mc]].mapID] = cells[mainCellID[mc]].buffID2;
		}
	}
	benchPoint[3] = std::chrono::steady_clock::now();

	debugF1 = std::chrono::duration<double, std::milli>(benchPoint[1] - benchPoint[0]).count();
	debugF2 = std::chrono::duration<double, std::milli>(benchPoint[2] - benchPoint[1]).count();
	debugF3 = std::chrono::duration<double, std::milli>(benchPoint[3] - benchPoint[2]).count();

	control.out_ready = 1; // ��������� ������� ���� ������
	// ������� ���� ������� ����
	while (control.p_shutdown == 0) {

		if (control.out_run_step == 0)
			std::this_thread::sleep_for(std::chrono::microseconds(10));

		while (control.out_run_step > 0) {
			benchPoint[1] = std::chrono::steady_clock::now();
			compute_lightMap(light_map, Dm, Dp, control.out_counter_of_step);
			benchPoint[2] = std::chrono::steady_clock::now();

			debugF3 = std::chrono::duration<double, std::milli>(benchPoint[2] - benchPoint[1]).count();

			control.out_counter_of_step++;
			control.out_run_step--;
			if (control.in_auto_run_mode)
				run_step(1);
		}
	}
	control.out_safe_shutdown = 1;
}