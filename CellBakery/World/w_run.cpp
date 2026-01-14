module World;
import osl;
using namespace osl::types;



// Обработка очереди событий и формирование графических данных в тройной буферизации
void World::sync(bool is_main_sync) {
	auto& current_write = *wa.world_snapshots.get_current_write();
	// Обработка очереди событий
	{
		auto& q = *wa.wkv_request_queue_ptr.get();
		while (q.pop(wkv_push_commands)) {
			for (const auto& c : wkv_push_commands) {
				// Получение ключа
				std::string_view key = c.get_key();
				const auto& value = c.value;

				if (const auto v = value.get_if<float>()) {
					if (key == std::string_view("ups")) {
						if (*v < float(-0.99f)) {
							is_paused = true;
						} else
							is_paused = false, ups_limiter.set(*v);
					}
					std::println("W: queue [{}, {}](float)", key, *v);
				}
			}
		}
	}

	bool do_sync = !is_paused && is_main_sync;
	if (do_sync) {
		ups_counter.push();
		int32_t rate = int32_t(sync_counter.count());
		if (ups_limiter.get_target_rate() > 300.) {
			if (skip_accum > 300) {
				skip_accum -= 300;
				skip_accum = std::max<int32_t>(skip_accum, -500);
				do_sync = false;
			}
			else {
				skip_accum += std::max(rate - 100, 0);
			}
		}
	}

	// Обновление графических данных
	if (do_sync) {
		sync_counter.push();
		auto& cells = cells_pc.storage;
		auto& cells_vram_storge = current_write.cells_vram_storge;

		// Если памяти VRAM для записи недостаточно, запрашиваем у хоста новый размер
		while (cells_pc.enabled.size() > cells_vram_storge.size()) {
			// Отправка запроса
			wkv_pull_commands.emplace_back("reallock_tb", size_t(cells_pc.enabled.size()));
			wa.wkv_response_queue_ptr.get()->push(wkv_pull_commands);

			// Ждём пока хост обработает запрос
			wa.critical_wait();
			if (!wa.is_running.load()) 
				return;
		}

		// Ожидание завершения использования текущего буфера со стороны GPU
		while ((*current_write.frame_index_ptr) < current_write.frame_index)
			std::this_thread::yield();

		// Загружаем данные в VRAM отражение буфера
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

			// Указываем сколько клеток записано
			current_write.cells = cells_vram_storge.subspan(0, cells_pc.enabled.size());
		}

		{
			auto& b = current_write.bench;

			b["total_ups"] = ups_counter.count_rate(std::chrono::seconds(4));
			b["avr_c"] = bench["avr_collision"].get();
			b["collision"] = bench["collision"].get();
			b["gap1"] = bench["GaP_1"].get();
			b["gap2"] = bench["GaP_2"].get();
			b["gap"] = bench["GaP_1"].get() + bench["GaP_2"].get();
			b["gapcc"] = bench["GaP_cc"].get();
			b["mspu"] = bench["mspu"].push(ups.get(), 1.);
			b["sync"] = bench["sync"].get();

			size_t memory_usage = 0u;
			memory_usage += sizeof(cell_t) * cells_pc.storage.capacity();
			memory_usage += sizeof(size_t) * cells_pc.enabled.capacity();
			memory_usage += sizeof(size_t) * cells_pc.disabled.capacity();
			memory_usage += sizeof(line_struct_t) * lines.capacity();
			memory_usage += sizeof(std::pair<id_t, id_t>) * detected_pair_collision_vec.capacity();
			b["mem"] = memory_usage / (1024. * 1024.);
		}

		// Отдаём текущий кадр мира
		current_write.swap_delta_ms = swap_delta.get();
		wa.world_snapshots.swap();
	}
}

void World::run(const WorldAdapter::world_settings_t &ws) {
	cells_limit = ws.cells_limit;

	// Выделяем видеопамять ещё до входа в симуляцию
	wkv_pull_commands.emplace_back("reallock_tb", size_t(cells_limit));
	wa.wkv_response_queue_ptr.get()->push(wkv_pull_commands);

	// Резервируем собственную память
	lines.reserve(static_cast<size_t>(cells_limit) + 2u);
	cells_pc.storage.reserve(static_cast<size_t>(cells_limit));
	detected_pair_collision_vec.reserve(cells_limit * 5u);

	wa.critical_wait(); // Ждём пока хост обработает запрос
	if (!wa.is_running.load())
		return;

	swap_delta.get();
	ups_limiter.set(20.);
	// Инициализация
	{
		// Инициализация первого шага (временный код)
		{
			auto& cells = cells_pc.storage;

			// резервируем память

			// создание клеток
			for (id_t i = 0; i < cells_limit; i++) {
				// создаём новую клетку
				auto& c = cells[enable_new_cell()];
				// начальная инициализация, возможно нужно переработать цикл чтобы она не требовалась
				c.type = cell_t::type_t::phago;
				c.pos = vec2(rand.nf(), rand.nf()) * ws.world_size * 0.5;
				c.color = fvec4(osl::HSV2RGB(vec3(rand.pf(), 1. - std::pow(rand.pf(), 4.), 1. - 0.7 * std::pow(rand.pf(), 2.))), 1.f);
			}


			// инициализация физических параметров клеток
			for (const auto& id : cells_pc.enabled) {
				auto& c = cells[id];
				c.velocity = vec2(0.);				// начальная скорость = 0
				c.impulse = vec2(0.);				// начальный импульс = 0
				c.radius = 0.4 + rand.pd() * 0.1;	// радиус в м
				c.weight = c.radius * c.radius;		// масса в кг
			}
		}
	}


	// Основной цикл симуляции
	while (wa.is_running.load()) {
		// Начало нового шага симуляции
		dtm.get(); // Сброс таймера (начало сегмента)
		world_step_counter++;

		if (!is_paused) {
			// 1. Подготовка данных для поиска коллизий
			physics_1();
			bench["GaP_1"].push(dtm.get(), 1.);

			// 2. Поиск коллизий
			physics_2();
			bench["GaP_2"].push(dtm.get(), 1.);
			
			// 3. Обработка коллизий
			physics_3();
			bench["collision"].push(dtm.get(), 1.);

			// 4. Обновление состояний клеток
			update_cells();
			bench["update_cells"].push(dtm.get(), 1.);
		}

		// 4. Синхронизация и подготовка графических данных
		while (ups_limiter.sync(true, 100.))
			sync(false); // Промежуточная синхронизация для обработки пользовательских событий
		dtm.get(); // Учитываем только время полной синхронизации
		sync(true); // Полная синхронизация с подготовкой графических данных
		bench["sync"].push(dtm.get(), 1.);
	}

	std::println("World was stop");
}