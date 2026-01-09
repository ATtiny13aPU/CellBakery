module World;
import osl;
using namespace osl::types;


// Обработки коллизии между двумя клетками
void process_collision(cell_t &a, cell_t &b, const vec2 &dp, const frac& sum_r) {
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

	// коэффициент жёсткости
	// если k < -1, то это спадающая экспонента
	// если k > 0, то это растущая экспонента
	const frac k = -3. - std::clamp(v_approach * 0.2, -0.3, +3.);

	const frac r = dist / sum_r;					// расстояние между центрами клеток, нормализованное на сумму радиусов
	const vec2 nfv = dp / r;						// нормализованный вектор силы
	const frac f = k * (1. - r) / (k + r) * 32.;	// сила в ньютонах (скаляр)
	const vec2 fv = nfv * f;						// сила в ньютонах (вектор)
	//const vec2 fv = nfv * (1. - v_approach); // чисто порофлить
	a.force += fv;
	b.force -= fv;
}

// Обновление состояния агентов
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

// Первый этап физики: подготовка данных и сортировка
void World::physics_1() {
	// Сброс суммы сил
	for (const auto cid : cells_pc.enabled)
		cells_pc.storage[cid].force = vec2(0.);

	// Копирование позиций в массив событий
	for (auto& c : lines)
		c.first = cells_pc.storage[c.second].pos;

	// Сортировка событий (лексикографическая по целой части Y, затем по X, затем по id)
	std::sort(lines.begin(), lines.end(), [](const std::pair<vec2, id>& a, const std::pair<vec2, id>& b) {
		struct compare_struct_t { // tie заменён на структуру, чтобы облегчить работу компилятору
			const double x, y; const id i;
			auto operator<=>(const compare_struct_t&) const = default;
		};
		return compare_struct_t{ std::floor(a.first[1]), a.first[0], a.second } <
			compare_struct_t{ std::floor(b.first[1]), b.first[0], b.second };
		});

	// Для простой обработки краевого случая итераторами (два фиктивных элемента в конец)
	lines.emplace_back(vec2(std::numeric_limits<double>::infinity()), nullID);
	lines.emplace_back(vec2(std::numeric_limits<double>::infinity()), nullID);
}

// Второй этап физики: поиск коллизий и вызов функции обработки
void World::physics_2() {
	// Количество живых клеток
	const auto cells_num = static_cast<id>(cells_pc.enabled.size());
	// Ссылка на хранилище всех клеток
	auto& cells = cells_pc.storage;

	// Счётчики производительности
	uint32_t check_counter = 0u; // считаем общее число проверок потенциальных пар
	uint32_t collis_counter = 0u; // считаем число действительных коолизий

	// Максимально возможная сумма двух радиусов
	constexpr double max_diameter = 1.;

	auto c3_it = lines.cbegin();
	// проход по линиям (простые коллизии)
	for (auto c1_it = lines.cbegin(); c1_it != (lines.cend() - 1u); ++c1_it) {
		const auto& [c1_pos, c1] = *c1_it;

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
	// Удаление фиктивных элементов
	lines.pop_back();
	lines.pop_back();

	// Запись в бенчмарки
	bench["GaP_cc"].push(double(check_counter) / cells_num, 1.); // Проверок на клетку
	bench["avr_collision"].push(double(collis_counter) / cells_num, 1.); // Коллизий на клетку
}

// Обработка очереди событий и формирование графических данных в тройной буферизации
void World::sync() {
	// Обработка очереди событий
	{
		auto& q = *wa.wkv_request_queue_ptr.get();
		while (q.pop(wkv_push_commands)) {
			for (const auto& c : wkv_push_commands) {
				// Получение ключа
				std::string_view key = c.get_key();
				const auto& value = c.value;

				if (const auto v = value.get_if<float>()) {
					if (key == std::string_view("ups"))
						ups_limiter.set(*v);

					std::cout << "Key: " << key << ", Value (double): " << *v << '\n';
				}
			}
		}
	}

	// Обновление графических данных
	{
		auto& cells = cells_pc.storage;

		auto& cells_vram_storge = wa.world_snapshots.get_current_write()->cells_vram_storge;

		// Если памяти для записи недостаточно, запрашиваем у хоста увеличение тройной буферизации
		while (cells_pc.enabled.size() > cells_vram_storge.size()) {
			wkv_pull_commands.emplace_back("reallock_tb", size_t(cells_pc.enabled.size()));
			wa.wkv_response_queue_ptr.get()->push(wkv_pull_commands);

			// Ждём пока хост обработает запрос на изменение размера тройной буферизации
			wa.critical_wait();
			if (!wa.is_running.load()) 
				return;
		}

		// Ожидание завершения использования текущего буфера со стороны GPU
		{
			auto& ws = *wa.world_snapshots.get_current_write();
			while ((*ws.frame_index_ptr) < ws.frame_index) {
				std::this_thread::yield();
				//std::print("Waiting for GPU to finish with frame index: {} \n", ws.frame_index);
			}
		}

		// Загружаем данные в тройной буфер
		{
			float time_cycle = (world_step_counter % 20) / 20.; // временно для отладки

			auto wb_it = cells_vram_storge.begin();
			for (const auto cid : cells_pc.enabled) {
				const auto& cell = cells[cid];
				auto& vram_cell = *wb_it;

				vram_cell.position = fvec4(fvec2(cell.pos), fvec2(cell.velocity));
				vram_cell.color = cell.color;
				vram_cell.meta = fvec4(fvec2(cell.force), time_cycle, cell.radius * 2.);

				wb_it++;
			}

			wa.world_snapshots.get_current_write()->cells = cells_vram_storge.subspan(0, cells_pc.enabled.size());
		}

		{
			auto& b = wa.world_snapshots.get_current_write()->bench;

			b["avr_c"] = bench["avr_collision"].get();
			b["gap1"] = bench["GaP_1"].get();
			b["gap2"] = bench["GaP_2"].get();
			b["gap"] = bench["GaP_1"].get() + bench["GaP_2"].get();
			b["gapcc"] = bench["GaP_cc"].get();
			b["mspu"] = bench["mspu"].push(ups.get(), 1.);
			b["sync"] = bench["sync"].get();
		}

		wa.world_snapshots.swap();
	}
}

void World::run(const WorldAdapter::WorldSettings &ws) {

	// Инициализация
	{
		// Инициализация первого шага (временный код)
		{
			cells_limit = ws.cells_limit;
			auto& cells = cells_pc.storage;

			// резервируем память
			lines.reserve(static_cast<size_t>(cells_limit) + 2u);
			cells.reserve(static_cast<size_t>(cells_limit));

			// создание клеток
			for (id i = 0; i < cells_limit; i++) {
				// создаём новую клетку
				auto& c = cells[cells_pc.get_new()];
				c.pos = vec2(rand.pf(), rand.pf()) * ws.world_size;
				// начальная инициализация, возможно нужно переработать цикл чтобы она не требовалась
				lines.emplace_back(c.pos, static_cast<id>(lines.size()));
				c.color = fvec4(osl::HSV2RGB(vec3(rand.pf(), 1. - std::pow(rand.pf(), 4.), 1. - 0.7 * std::pow(rand.pf(), 2.))), 1.f);
			}

			// инициализация физических параметров клеток
			for (id i = 0; i < cells_limit; i++) {
				auto& c = cells[i];
				c.force = vec2(0.);		// начальная сила = 0
				c.velocity = vec2(0.);	// начальная скорость = 0
				c.impulse = vec2(0.);	// начальный импульс = 0
				c.radius = 0.4 + rand.pd() * 0.1;		// радиус в м
				c.weight = c.radius * c.radius * 4.;	// масса в кг
			}
		}

		ups_limiter.set(20.);
	}


	// Основной цикл симуляции
	while (wa.is_running.load()) {
		// Начало нового шага симуляции
		dtm.get(); // Сброс таймера (начало сегмента)
		world_step_counter++;

		// 1. Подготовка данных для поиска коллизий
		physics_1();
		bench["GaP_1"].push(dtm.get(), 1.);

		// 2. Поиск и обработка коллизий
		physics_2();
		bench["GaP_2"].push(dtm.get(), 1.);

		// 3. Обновление состояний клеток
		update_cells();
		bench["update_cells"].push(dtm.get(), 1.);
		
		// 4. Синхронизация и подготовка графических данных
		sync();
		bench["sync"].push(dtm.get(), 1.);

		// Стабилизация UPS с повышенной точностью
		ups_limiter.sync(true);
	}
}