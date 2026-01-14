module World;
import osl;
using namespace osl::types;

// Вычисление силы взаимодействия между двумя клетками
// r - нормализированный радиус (0..1), v - скорость сближения (отрицательная при сближении, положительная при расхождении)
inline vec2 compute_pair_force(const vec2& delta_pos, const frac& r, const frac& v) {
	// Демпфирование реализовано как динамический коэффициент жёсткости, зависящий от скорости сближения клеток.
	// Когда клетки сближаются, коэффициент немного растёт, чтобы сделать удар жёстче, но при разлёте сила мгновенно падает.
	// Т.е. в момент удара тела ведут себя как упругие, но в момент разлёта сила быстро падает, что имитирует потерю энергии на деформацию.
	// Основная проблема метода, это дискретность шагов, из-за которой клетка может успеть за один шаг сменить вектор скорости
	// на противоположный, сделав удар абсолютно упругим (или даже хуже), что может вызвать осциляцию в группе сжатых клеток
	// потому, для корректного демпфирования нужно интегрировать скорость сближения минимум дважды

	constexpr frac k1 = 0.2; // коэффициент демпфирования при схождении
	constexpr frac k2 = 1.; // коэффициент демпфирования при расхождении
	constexpr frac a_factor = 1.; // ширина перехода
	const frac v_factor = v < 0 ? k1 * v :
		v < a_factor ? k1 * v + ((k2 - k1) / (2 * a_factor)) * v * v :
		k2 * v - ((k2 - k1) * a_factor / 2);

	// коэффициент жёсткости (чем больше k, тем выше жёсткость)
	//const frac k = std::clamp(-v_factor, -.9, .3); // компилятор не смог это оптимизировать
	const frac k = -(v_factor > 0.9 ? 0.9 : (v_factor < -0.3 ? -0.3 : v_factor));
	// нормализованный вектор силы
	const vec2 nfv = delta_pos / r;
	// сила в ньютонах (скаляр)
	const frac f = (1. - (r * (1. - k) / (1 - k * (r * 2. - 1.)))) * 12.;
	// сила в ньютонах (вектор)
	const vec2 fv = nfv * f;
	//const vec2 fv = nfv * (1. - v); // чисто порофлить

	return fv;
}

// Обработки коллизии между двумя клетками
template<bool predictive>
bool process_collision(cell_t &a, cell_t &b) {
	static_assert(std::is_trivially_copyable_v<osl::types::vec2>);
	static_assert(std::is_standard_layout_v<osl::types::vec2>);
	const vec2 delta_pos = a.pos - b.pos;
	const double sum_r = a.radius + b.radius;
	const frac sqr_r = osl::dot(delta_pos, delta_pos);
	if constexpr (predictive) // Для второго прохода выражение всегда false
		if (sqr_r > sum_r * sum_r || a.pos == b.pos)
			return false; // отсекаем r > 1. и r == 0.
	/*
		Сейчас два тела взаимодействую друг с другом, т.е. сумма их радиусов меньше расстояния между центрами.
		Нужно вычислить силу отталкивания и применить её к обеим клеткам.
	*/
	
	// 1. Вычисляем относительную скорость: v_rel = v_a - v_b
	const vec2 rel_v = predictive ? a.velocity - b.velocity :
		(((a.velocity_predict - b.velocity_predict) + (a.velocity - b.velocity)) * 0.5);

	// 2. Вычисляем нормализованный вектор направления (от b к a)
	// r — это нормализованное расстояние, мы используем его для получения нормали n
	const frac dist = sqrt(sqr_r);
	const vec2 n = delta_pos / dist;

	// 3. Проекция относительной скорости на нормаль (скалярная скорость сближения)
	// Если v_approach < 0, клетки сближаются
	// Если v_approach > 0, клетки разлетаются
	const frac v = osl::dot(rel_v, n) / sum_r;

	const frac r = dist / sum_r; // расстояние между центрами клеток, нормализованное на сумму радиусов

	const vec2 fv = compute_pair_force(delta_pos, r, v);

	if constexpr (predictive) {
		a.force_predict += fv;
		b.force_predict -= fv;
	}
	else {
		//frac l_fv = osl::length(fv);
		//a.forve_abs += l_fv;
		//b.forve_abs += l_fv;
		a.force += fv;
		b.force -= fv;
	}

	return true;
}

// Первый этап физики: подготовка данных и сортировка
void World::physics_1() {
	// Копирование позиций в массив событий (с пропуском неактивным клеток)
	auto write_it = lines.begin();
	for (auto read_it = lines.begin(); read_it != lines.end(); ++read_it) {
		const auto& cell = cells_pc.storage[read_it->index];
		if (cell.type != cell_t::type_t::none) { // Если клетка активная
			auto& w = *write_it;
			w.index = read_it->index;
			// меняем местами для упрощения лексикографического сравнения (сначала по y, потом по x)
			w.x = std::floor(cell.pos[1]);
			w.y = cell.pos[0];
			++write_it;
		} // Если деактивная, то индекс чтения будет пропущен, индекс записи останется
	}
	lines.resize(std::distance(lines.begin(), write_it)); // По количеству записанных клеток


	// Сортировка событий (лексикографическая по целой части Y, затем по X, затем по id)
	std::sort(lines.begin(), lines.end());

	// Возвращаем позиции обратно для этапа 2
	for (auto& c : lines) {
		const auto& cell_pos = cells_pc.storage[c.index].pos;
		c.y_floor = c.x; // в x содержался std::floor(cell_pos[1])
		c.x = cell_pos[0];
		c.y = cell_pos[1];
	}

	// Для простой обработки краевого случая итераторами (два фиктивных элемента в конец)
	lines.push_back(line_struct_t{});
	lines.push_back(line_struct_t{});
}

// Второй этап физики: поиск коллизий
void World::physics_2() {
	/*
		Алгоритм пространственного поиска (Slab-based Sweep and Prune).
		* Названия итераторов и их роли:
		- c1_it: Клетка-инициатор. Основной цикл проходит по всем клеткам.
		- c2_it: Потенциальная цель. Ищет соседей справа на текущей линии и на линии выше.
		- c3_it: "Ленивый" маркер границы. Хранит левый край диапазона видимости
		на линии над c1_it, чтобы не начинать поиск по верхней линии с нуля.
		* Параметры отсечения (stop/sub_stop):
		- stop_x: Правая граница по горизонтали (X + 2R).
		- stop_y: Верхняя граница текущей полосы (floor(Y) + 1.0).
		- sub_stop_x/y: Левая и верхняя границы для синхронизации c3_it с текущим c1_it.
	*/
	// Количество живых клеток
	const auto cells_num = static_cast<id_t>(cells_pc.enabled.size());
	// Ссылка на хранилище всех клеток
	auto& cells = cells_pc.storage;

	// Вектор коллизий
	detected_pair_collision_vec.clear();

	// Максимально возможная сумма двух радиусов
	constexpr double max_diameter = 1.;

	auto c3_it = lines.cbegin();
	// проход по линиям (простые коллизии)
	for (auto c1_it = lines.cbegin(); c1_it != (lines.cend() - 1u); ++c1_it) {
		const auto& c1 = *c1_it;
		// Вычисляем диапазоны
		// на 2R вправо по X и до ближайшего по Y вверх (при переходе на следующую линию)
		const double stop_x_right = c1.x + max_diameter;
		const double stop_y = c1.y_floor + 1.;
		// аналогично для c3_it (верхний левый угол границы коллизии с c1_it)
		const double sub_stop_x_left = c1.x - max_diameter;
		const double sub_stop_x_right = c1.x + max_diameter;
		const double sub_stop_y = c1.y_floor + 1.;

		// Применяем диапазоны
		// если итератор на той же линии
		while (c3_it->y < sub_stop_y)
			++c3_it;

		// если итератор не переходит на следующую линию но опаздывает по x
		while (c3_it->y_floor == std::next(c3_it)->y_floor && c3_it->x < sub_stop_x_left)
			++c3_it;

		// по текущей линии
		for (auto c2_it = std::next(c1_it); c2_it->x < stop_x_right && c2_it->y < stop_y; ++c2_it)
			detected_pair_collision_vec.emplace_back(c1.index, c2_it->index);

		// по линии над
		for (auto c2_it = c3_it; c2_it->x < sub_stop_x_right && c2_it->y < stop_y + 1.; ++c2_it)
			detected_pair_collision_vec.emplace_back(c1.index, c2_it->index);
	}

	// Удаление фиктивных элементов
	lines.pop_back();
	lines.pop_back();
}

// Третий этап физики: обработка всех найденных коллизий
void World::physics_3() {

	// Сброс суммы сил
	for (const auto cid : cells_pc.enabled) {
		cells_pc.storage[cid].force_predict = vec2(0.);
		cells_pc.storage[cid].force = vec2(0.);
		cells_pc.storage[cid].forve_abs = 0.;
	}

	// Счётчик действительных пар
	size_t collision_counter = 0u;
	// Вычисление сумм сил для всех найденных коллизий
	for (const auto [a_id, b_id] : detected_pair_collision_vec)
		if (process_collision<true>(cells_pc.storage[a_id], cells_pc.storage[b_id]))
			detected_pair_collision_vec[collision_counter++] = std::pair<id_t, id_t>{a_id, b_id};



	// Запись счётчиков производительности
	{
		this->collision_counter = collision_counter;
		check_counter = detected_pair_collision_vec.size();

		const double dev = static_cast<double>(std::max<size_t>(cells_pc.enabled.size(), 1));
		bench["GaP_cc"].push(double(check_counter) / dev, 1.); // Проверок на клетку
		bench["avr_collision"].push(double(collision_counter) / dev, 1.); // Коллизий на клетку

		// std::println("{} / {}", collision_counter, check_counter);
	}

	constexpr frac substeps = 20.;
	constexpr frac delta_time = 1. / substeps; // время подшага (секунды)
	constexpr frac viscosity = 0.01; // вязкость

	for (const auto cid : cells_pc.enabled) {
		auto& cell = cells_pc.storage[cid];
		cell.velocity_predict = cell.velocity + cell.force_predict / cell.weight * delta_time * 0.5;
	}

	// Усекаем размер до действительных пар
	detected_pair_collision_vec.resize(collision_counter);
	// Повторный проход с коррекцией взаимной скорости

	for (const auto [a_id, b_id] : detected_pair_collision_vec) {
		process_collision<false>(cells_pc.storage[a_id], cells_pc.storage[b_id]);
	}

	for (const auto cid : cells_pc.enabled) {
		auto& cell = cells_pc.storage[cid];

		//	// Цветовое кодирование силы взаимодействия (для отладки)
		//const frac color_k = std::clamp(cell.forve_abs / 50. + 0.2, 0., 1.);
		//cell.color = fvec4(fvec3(color_k), 1.f);

		// Динамическое трение вязкости
		cell.force -= viscosity * cell.velocity;

		// Стягивание к центру для отладки
		cell.force = osl::mix(cell.force, -cell.pos, 0.001);

		// Вычисление ускорения: a = F / m
		const vec2 acceleration = cell.force / cell.weight;

		// Обновление скорости: v += a * dt
		cell.velocity += acceleration * delta_time;

		// Обновление позиции: pos += v * dt
		cell.pos += cell.velocity * delta_time;

		// Обновление импульса: p = m * v
		cell.impulse = cell.weight * cell.velocity;
	}
}