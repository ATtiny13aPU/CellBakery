module;
#include "monolith_ogl_imgui_header.h";

module Context;
import osl;
using namespace osl::types;
import shad;
import std;

void Context::sync() {
	// TODO: рефакторинг
	if (!gui_s.no_update_flag) {
		// предварительное создание события окончания использования текущего vbo
		// world_snapshots_storage[current_vbo_index].fence_release();
		world.last_capture()->frame_index = frame_counter - 1u;

		// получение нового кадра из мира и перерисчёт параметров интерполяции
		if (const auto world_state = world.capture()) {
			// обновление графики
			framePerUpdate.push(frame_counter - last_update_frame, 1.);
			last_update_frame = frame_counter;
			current_vbo_index = world_state->vbo_index;
			current_vbo_gl_id = world_snapshots_storage[current_vbo_index].id();
			// Связываем VBO как SSBO для случайного доступа к графическим данным из под пост-процессора
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, current_vbo_gl_id);

			time_lerp -= 1.;
			delta_time_lerp = (1. - time_lerp) / framePerUpdate.get();
			if (!(delta_time_lerp > 0. && delta_time_lerp < 1.))
				delta_time_lerp = 0., time_lerp = 1.;
		} else
			// икремент времени интерполяции
			time_lerp += delta_time_lerp;
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
					std::cout << "Key: " << key << ", Value (size_t): " << *v << '\n';
				}
			}
		}
	}

	// Разблокировка критической секции, если была запрошена
	if (was_critical)
		world.unblock_critical();
}