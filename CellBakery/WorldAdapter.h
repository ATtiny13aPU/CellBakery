#pragma once

#include <OSL/include.h>

using namespace osl;

class WorldAdapter {
public:
	struct WorldSettings;
	void run(const WorldSettings &ws);

	void stop() {
		isRunning = false;
		wait_to_close();
	}

	struct RenderData;
	struct RenderCellData;
	inline const RenderData* capture() {
		return world_data_snapshots.capture();
	}

private:	
	void wait_to_close() const {
		while (!isSafeToClose)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::atomic_bool isRunning;
	std::atomic_bool isSafeToClose;
	osl::MultiThreadContainer<RenderData> world_data_snapshots;
};


struct WorldAdapter::WorldSettings {
	// максимальное число клеток, предполагается динамическое управление памятью
	size_t cells_limit;
	// условый размер мира, убивает клетки за пределом
	vec2 world_size;
	// число субшагов на шаг
	size_t sub_steps;
};

struct WorldAdapter::RenderData {
	std::vector<RenderCellData> cells;
	std::map<std::string, double> bench;
};

struct WorldAdapter::RenderCellData {
	osl::fvec4 position;	// позиция + скорость в мировых координатах
	osl::fvec4 color;		// RGB + effect
	osl::fvec4 debug;		// Зарезервировано
};
