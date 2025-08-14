#include "World.h"

typedef uint32_t id;

// Константы для обозначения состояния клеток
inline const id nullID = static_cast<id>(-1);  // Нет следующей клетки
inline const id deadID = static_cast<id>(-2);  // Клетка "мёртвая"




void World::run(const WorldSettings ws) {
	isRunning = true;
	osl::UpdateRateLimiter ups_limiter(20.);


	while (isRunning) {


		ups_limiter.sync();
	}
}