#pragma once

#include <OSL/include.h>

using namespace osl;

class World {
public:
	struct WorldSettings;
	void run(const WorldSettings ws);

	struct RenderData;
	struct RenderCellData;
	inline const RenderData* capture() {
		return render_data_snapshots.capture();
	}

	void stop() {
		isRunning = false;
	}
private:
	bool isRunning;
	osl::Random rand;
	osl::MultiThreadContainer<RenderData> render_data_snapshots;
};


struct World::WorldSettings {
	// максимальное число клеток, предполагается динамическое управление памятью
	size_t cells_limit;
	// условый размер мира, убивает клетки за пределом
	vec2 world_size;
	// число субшагов на шаг
	size_t sub_steps;
};

struct World::RenderData {
	std::vector<RenderCellData> cells;
	std::map<std::string, double> bench;
};

struct World::RenderCellData {
	osl::fvec4 position;	// позиция + скорость в мировых координатах
	osl::fvec4 color;		// RGB + effect
	osl::fvec4 debug;		// Зарезервировано
};
