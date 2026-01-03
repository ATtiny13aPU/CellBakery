module World;
import osl;
using namespace osl::types;


//#pragma optimize("", off)
//__declspec(noinline)
void process_collision(Cell &a, Cell &b, const vec2 &dp, const frac& sum_r) {
	if (dp == vec2(0.))
		return;
	/*
		Сейчас два тела взаимодействую друг с другом, т.е. сумма их радиусов меньше расстояния между центрами.
		Нужно вычислить силу отталкивания и применить её к обеим клеткам.
		Применяется классческая модель пружины, но с ограничением на максимальную силу и экспоненциальным ростом силы.

		Демпфирование реализовано как динамический коэффициент жёсткости, зависящий от скорости сближения клеток.
	*/

	// 1. Вычисляем относительную скорость: v_rel = v_a - v_b
	const vec2 rel_v = a.velocity - b.velocity;

	// 2. Вычисляем нормализованный вектор направления (от b к a)
	// r — это нормализованное расстояние, мы используем его для получения нормали n
	const frac sqr_r = osl::dot(dp, dp);
	const frac dist = sqrt(sqr_r);
	const vec2 n = dp / dist;

	// 3. Проекция относительной скорости на нормаль (скалярная скорость сближения)
	// Если v_approach < 0, клетки движутся навстречу друг другу
	// Если v_approach > 0, клетки разлетаются
	const frac v_approach = osl::dot(rel_v, n);

	const frac k = 4. + std::clamp(v_approach * 0.2, -0.2, +2.);	// коэффициент жёсткости (при 4 выстраиваются соты)

	const frac r = dist / sum_r;			// расстояние между центрами клеток, нормализованное на сумму радиусов
	const vec2 nfv = dp / r;				// нормализованный вектор силы
	const frac f = (1. / k) / r - (1. / k);	// сила в ньютонах (скаляр)
	const vec2 fv = nfv * std::min(2., f * 16.) * 4.;	// сила в ньютонах (вектор)

	a.force += fv;
	b.force -= fv;
}
//#pragma optimize("", on)

void World::update_cells() {
	const frac substeps = 20.;
	const frac delta_time = 1. / substeps;  // время подшага (секунды)
	const frac viscosity = 0.01;  // вязкость


	for (const auto cid : cells_pc.enabled) {
		auto &cell = cells_pc.storage[cid];

		// Динамическое трение вязкости
		cell.force -= viscosity * cell.velocity;

		// Стягивание к центру для отладки
		cell.force = osl::mix(cell.force, vec2(100.) - cell.pos, 0.01);

		// Вычисление ускорения: a = F / m
		vec2 acceleration = cell.force / cell.weight;

		// Обновление скорости: v += a * dt
		cell.velocity += acceleration * delta_time;

		// Обновление позиции: pos += v * dt
		cell.pos += cell.velocity * delta_time;

		// Обновление импульса: p = m * v
		cell.impulse = cell.weight * cell.velocity;
	}
}

void World::run(const WorldAdapter::WorldSettings &ws) {
	const auto cells_limit = static_cast<uint32_t>(ws.cells_limit);
	WorldKeyValueCommands wkv_commands;
	ups_limiter.set(20.);

	std::vector<std::pair<vec2, id>> lines;
	auto &cells = cells_pc.storage;
	// инициализация первого шага (временный код)
	{
		lines.reserve(static_cast<size_t>(cells_limit) + 1u);
		cells.reserve(static_cast<size_t>(cells_limit));

		for (id i = 0; i < cells_limit; i++) {
			// создаём новую клетку
			auto &c = cells[cells_pc.get_new()];
			c.pos = vec2(rand.pf(), rand.pf()) * ws.world_size;
			// начальная инициализация, возможно нужно переработать цикл чтобы она не требовалась
			lines.emplace_back(c.pos, static_cast<id>(lines.size()));
			c.color = fvec4(osl::HSV2RGB(vec3(rand.pf(), 1. - std::pow(rand.pf(), 4.), 1. - 0.7 * std::pow(rand.pf(), 2.))), 1.f);
		}

		// инициализация физических параметров клеток
		for (id i = 0; i < cells_limit; i++) {
			auto &c = cells[i];
			c.force = vec2(0.);		// начальная сила = 0
			c.velocity = vec2(0.);	// начальная скорость = 0
			c.impulse = vec2(0.);	// начальный импульс = 0
			c.radius = 0.2 + rand.pd() * 0.3;		// радиус в м
			c.weight = c.radius * c.radius * 4.;	// масса в кг
		}
	}

	osl::DeltaTimeMark dtm;
	osl::DeltaTimeMark ups;
	ups.get();
	uint32_t world_step_counter = (-1);
	while (wa.isRunning.load()) {
		// Обновление симуляции
		world_step_counter++;

		// Сброс суммы сил
		for (const auto cid : cells_pc.enabled)
			cells_pc.storage[cid].force = vec2(0.);


		uint32_t check_counter = 0u; // считаем общее число проверок потенциальных пар
		uint32_t collis_counter = 0u; // считаем число действительных коолизий

		// Этап подготовки данных для поиска коллизий
		dtm.get();
		for (auto &c : lines)
			c.first = cells[c.second].pos;

		// Сортировка событий (лексикографическая по целой части Y, затем по X, затем по id)
		std::sort(lines.begin(), lines.end(), [](const std::pair<vec2, id>& a, const std::pair<vec2, id>& b) {
			struct compare_struct_t { // tie заменён на структуру, чтобы облегчить работу компилятору
				const double x, y; const id i;
				auto operator<=>(const compare_struct_t&) const = default;
			};
			return compare_struct_t{ std::floor(a.first[1]), a.first[0], a.second } <
				compare_struct_t{ std::floor(b.first[1]), b.first[0], b.second };
		});

		bench["GaP_1"].push(dtm.get(), 1.);

		// Для простой обработки краевого случая итераторами
		lines.emplace_back(vec2(std::numeric_limits<double>::infinity()), nullID);
		lines.emplace_back(vec2(std::numeric_limits<double>::infinity()), nullID);
		

		// Этап поиска коллизий
		const double max_diameter = 1.; // Максимально возможная сумма двух радиусов

		auto c3_it = lines.cbegin();
		// проход по линиям (простые коллизии)
		for (auto c1_it = lines.cbegin(); c1_it != (lines.cend() - 1u); ++c1_it) {
			const auto&[c1_pos, c1] = *c1_it;

			// на 2R дальше по x и до ближайшего по Y вверх
			const auto stop = vec2(c1_pos[0] + max_diameter, std::ceil(c1_pos[1]));
			{
				const auto sub_stop = vec2(c1_pos[0] - max_diameter, std::ceil(c1_pos[1]));
				// если итератор на той же линии
				while (c3_it->first[1] < sub_stop[1])
					++c3_it;
				// если итератор не переходит на следующую линию но опаздывает по x
				while (std::floor(c3_it->first[1]) == std::floor(std::next(c3_it)->first[1]) && c3_it->first[0] < sub_stop[0])
					++c3_it;
			}
			// по текущей линии
			for (auto c2_it = std::next(c1_it); c2_it->first[0] < stop[0] && c2_it->first[1] < stop[1]; ++c2_it) {
				check_counter++;
				const vec2 dp = c1_pos - c2_it->first;
				const double sum_r = cells[c1].radius + cells[c2_it->second].radius;
				if (osl::dot(dp, dp) < sum_r * sum_r)
					process_collision(cells[c1], cells[c2_it->second], dp, sum_r), collis_counter++;
			}
			// по линии над
			for (auto c2_it = c3_it; c2_it->first[0] < stop[0] && c2_it->first[1] < stop[1] + 1.; ++c2_it) {
				check_counter++;
				const vec2 dp = c1_pos - c2_it->first;
				const double sum_r = cells[c1].radius + cells[c2_it->second].radius;
				if (osl::dot(dp, dp) < sum_r * sum_r)
					process_collision(cells[c1], cells[c2_it->second], dp, sum_r), collis_counter++;
			}
		}
		lines.pop_back();
		lines.pop_back();

		bench["GaP_2"].push(dtm.get(), 1.);
		bench["GaP_cc"].push(double(check_counter) / cells_limit, 1.);
		bench["avr_collision"].push(double(collis_counter) / cells_limit, 1.);

		update_cells();
		bench["update_cells"].push(dtm.get(), 1.);
		// Обработка очереди событий
		{
			auto &q = *wa.wkv_queue_ptr.get();
			while (q.pop(wkv_commands)) {
				for (const auto &c : wkv_commands) {
					// Получение ключа
					std::string_view key = c.get_key();
					const auto &value = c.get_value();

					if (const uint64_t* v = std::get_if<uint64_t>(&value)) {
						std::cout << "Key: " << key << ", Value (uint64_t): " << *v << '\n';
					}
					else if (const double* v = std::get_if<double>(&value)) {
						if (key == std::string_view("ups"))
							ups_limiter.set(*v);

						std::cout << "Key: " << key << ", Value (double): " << *v << '\n';
					}
				}
			}
		}


		// Обновление графических данных
		{
			auto &cells_wb = wa.world_data_snapshots.get_current_write()->cells;
			cells_wb.resize(cells_limit);
			float time_cycle = (world_step_counter % 20) / 20.; // временно для отладки

			for (auto wb_it = cells_wb.begin(); wb_it != cells_wb.end(); wb_it++) {
				auto &wb = (*wb_it);
				const auto &cell = cells[std::distance(cells_wb.begin(), wb_it)];
				wb.position = fvec4(fvec2(cell.pos), fvec2(cell.velocity));
				if (cell.force == vec2(0.))
					wb.color = cell.color * -0.1f;
				else
					wb.color = cell.color;
				wb.meta = fvec4(fvec2(cell.force)//fvec2(sinf(cell.angle), cosf(cell.angle))
					, time_cycle, cell.radius * 2.);

			}

			{
				auto &b = wa.world_data_snapshots.get_current_write()->bench;

				b["avr_c"] = bench["avr_collision"].get();
				b["gap1"] = bench["GaP_1"].get();
				b["gap2"] = bench["GaP_2"].get();
				b["gap"] = bench["GaP_1"].get() + bench["GaP_2"].get();
				b["gapcc"] = bench["GaP_cc"].get();
				b["mspu"] = bench["mspu"].push(ups.get(), 1.);
			}
		}
		wa.world_data_snapshots.swap();

		ups_limiter.sync(true);
	}
}