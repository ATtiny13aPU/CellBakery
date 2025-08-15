#pragma once
#include "WorldAdapter.h"

typedef uint32_t id;

class Cell;

// Константы для обозначения состояния клеток
inline const id nullID = static_cast<id>(-1);  // Нет следующей клетки
inline const id deadID = static_cast<id>(-2);  // Клетка "мёртвая"

// Класс имплементации мира
class World {
public:
	World(WorldAdapter &wa) : wa(wa) {
		rand.init("3523dgfsdg", 256u);
	}

	// Основной цикл симуляции
	void run(const WorldAdapter::WorldSettings &);

	std::map<std::string, osl::fastMovingAverageW<120>> bench;
private:
	WorldAdapter &wa;

	osl::Random rand;
	void update_cells();

	osl::UpdateRateLimiter ups_limiter;
	osl::PoolContainer<Cell> cells_pc;
};

// Враппер функция для запуска мира
void WorldAdapter::run(const WorldSettings &ws) {
	World world(*this);
	isRunning = true;
	isSafeToClose = false;
	world.run(ws);
	isSafeToClose = true;
}