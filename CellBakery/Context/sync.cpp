module;
#include "monolith_ogl_imgui_header.h";

module Context;
import osl;
using namespace osl::types;
import shad;
import std;

void Context::sync() {
	if (!gui_s.no_update_flag) {
		world.last_capture()->frame_index = frame_counter - 1u;
		// Получение нового кадра из мира и перерисчёт параметров интерполяции
		if (const auto world_state = world.capture()) {
			// Получение отношения числа кадров к числу обновлений буфера
			update_rate_counter.push();
			const double frame_per_update =
				frame_rate_counter.count_rate(std::chrono::milliseconds(200)) /
				update_rate_counter.count_rate(std::chrono::milliseconds(500));
			// Вычисление нового дельта времени интерполяции
			// delta_time_lerp вычисляется такая, чтобы за frame_per_update шагов значение time_lerp стремилось к 1.
			time_lerp -= 1.;
			delta_time_lerp = (1. - time_lerp) / frame_per_update;

			// Обновление индекса текущего отрисовываемого VBO и привязка его к VAO
			current_vbo_index = world_state->vbo_index;
			current_vbo_gl_id = world_snapshots_storage[current_vbo_index].id();
			cells_vao.bind_buffer_range(current_vbo_gl_id, 0, 0);

			// Обновляем связку VBO как SSBO для случайного доступа к графическим данным из под пост-процессора
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, current_vbo_gl_id);
		}
		// икремент времени интерполяции
		time_lerp = std::max<frac32>(-0.3, std::min<frac32>(time_lerp + delta_time_lerp, 1.4));
	}

	/*
		Проверка на запрос критической секции от мира до обработки команд,
		чтобы гарантировать, что после выхода из критической секции все команды
		критической секции будут обработаны.
	*/
	bool was_critical = world.was_request_critical();

	// Обработка команд ключ-значение из мира
	while (world.pull_wkv_commands(wkv_pull_commands)) {
		for (const auto& c : wkv_pull_commands) {
			// Получение ключа
			std::string_view key = c.get_key();
			const auto& value = c.value;

			if (const auto v = value.get_if<size_t>()) {
				// запрос на изменение размера тройной буферизации
				if (key == std::string_view("reallock_tb")) {
					auto &storge = *world.critical_capture();

					uint32_t index_counter = 0;
					// Применение нового размера к каждому из трёх буферов
					for (auto&& [frame, vbo] : std::ranges::views::zip(storge, world_snapshots_storage)) {
						frame->vbo_index = index_counter++;
						frame->frame_index_ptr = frame_counter_mapped_ptr;
						// Изменение размера хранилища тройной буферизации
						vbo.allocate(*v * sizeof(WorldAdapter::cell_render_data_t));
						// Привязка хранилища к std::span
						frame.get()->cells_vram_storge = vbo.as_span<WorldAdapter::cell_render_data_t>();
					}
					std::println("G: queue [{}, {}](size_t)", key, *v);
				}
			}
		}
	}

	// Разблокировка критической секции, если была запрошена
	if (was_critical)
		world.unblock_critical();
}