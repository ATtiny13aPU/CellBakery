#pragma once


typedef uint32_t id;

class Cell;

// Константы для обозначения состояния клеток
inline const id nullID = static_cast<id>(-1);  // Нет следующей клетки
inline const id deadID = static_cast<id>(-2);  // Клетка "мёртвая"

// Класс имплементации мира
class World {
public:
	World() {
		RAND.init("3523dgfsdg", 256u);
	}

	// Основной цикл симуляции
	void run(std::atomic_bool &, osl::MultiThreadContainer<WorldAdapter::RenderData> &);

	std::map<std::string, osl::fastMovingAverageW<120>> bench;
private:
	osl::Random RAND;
	void update_cells();

	osl::UpdateRateLimiter ups_limiter;
	osl::PoolContainer<Cell> cells_pc;
};