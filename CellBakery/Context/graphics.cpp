module;
#include "monolith_ogl_imgui_header.h"
module Context;
import osl;
using namespace osl::types;
import shad;

void Context::graphics() {
	// Очистка экрана
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	fvec4 worldView = camera.direct_view();
	fvec4 windowView = camera.inverse_view();

	auto& cell_vbo = world_snapshots_storage[current_vbo_index];

	// Привязка и отчистка текстуры и привязка VBO к SSBO
	{
		glBindImageTexture(0, frame_texture_id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
		static std::array<uint32_t, 4> color_reset = { -1, -1, -1, -1 };
		glClearTexImage(
			frame_texture_id, // ID текстуры (в вашем случае 17)
			0,                // level (мип-уровень)
			GL_RGBA_INTEGER,  // format (ОБЯЗАТЕЛЬНО с суффиксом _INTEGER)
			GL_UNSIGNED_INT,  // type (тип данных в массиве clear_color)
			color_reset.data()
		);
	}

	// Отрисовка клеток
	if (gui_s.show_cells) {
		cellsShader.use();
		cellsShader.uniform("TimeLerp", time_lerp - 1.f);
		cellsShader.uniform("ViewWorld", worldView);
		cellsShader.uniform("ViewWindow", windowView);
		cellsShader.uniform("WinSize", win_size);

		cellsMesh.draw(cell_vbo, shad::draw_primitive::gl_points);
	}

	// Отрисовка чашки Петри (Пост-эффект отрисовка)
	{
		petriShader.use(); 
		petriShader.uniform("TimeLerp", time_lerp - 1.f);
		petriShader.uniform("ViewWorld", worldView);
		petriShader.uniform("WinSize", win_size);
		petriShader.uniform("MSAA", gui_s.MSAA);
		petriShader.uniform("MSAA_quasi_start", float(gui_s.MSAA_quasi_start));

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		screenMesh.draw(shad::draw_primitive::gl_triangle_strip);
	}

	// Запрос захвата пикселя экрана для упрощения поиска коллизии курсора с клетками
	if (pbo.active) {
		// если позиция мыши внутри окна
		if (camera.mouse_screen_pos() > vec2(0) && camera.mouse_screen_pos() < vec2(1)) {
			// Если предыдущий запрос еще не обработан, лучше не спамить новыми
			ivec2 mp = ivec2(camera.mouse_screen_pos() * win_size);
			if (!pbo.pending) {
				// Если предыдущий запрос еще не обработан, лучше не спамить новыми
				glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo.id);
				// Асинхронное чтение из текстуры в PBO
				glGetTextureSubImage(frame_texture_id, 0, mp[0], mp[1], 0, 1, 1, 1,
					GL_RGBA_INTEGER, GL_UNSIGNED_INT, 16, nullptr);
				glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

				pbo.sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
				pbo.pending = true;
			}
		}
	}

	// Отрисовка коробок
	if (gui_s.show_boxes) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		boxShader.use();
		boxShader.uniform("TimeLerp", time_lerp - 1.f);
		boxShader.uniform("ViewWorld", worldView);
		boxShader.uniform("ViewWindow", windowView);
		boxShader.uniform("WinSize", win_size);

		cellsMesh.draw(cell_vbo, shad::draw_primitive::gl_points);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Отрисовка сил
	if (gui_s.show_forces && (gui_s.scale_force_draw > 0.01 || gui_s.scale_vel_draw > 0.01)) {
		forceShader.use();
		glLineWidth(1.8f);
		forceShader.uniform("ViewWorld", worldView);
		forceShader.uniform("ViewWindow", windowView);
		forceShader.uniform("ScaleForce", static_cast<float>(gui_s.scale_force_draw / 20.f));
		forceShader.uniform("ScaleVel", static_cast<float>(gui_s.scale_vel_draw / 20.f));

		cellsMesh.draw(cell_vbo, shad::draw_primitive::gl_points);
	}

	// Первый барьер гарантирует, что операция с vbo завершена до этой
	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	// Обновление счётчика кадров в immutable буфере
	glClearNamedBufferSubData(frame_id_immutable_buffer.id(), GL_R32UI,
		0, sizeof(uint32_t) * 4,
		GL_RED_INTEGER, GL_UNSIGNED_INT, &frame_counter_proxy);
	// Второй барьер гарантирует, что операция с immutable буфером завершена сразу же
	glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);


	// Попытка забрать запрошенные данные в конце кадра
	if (pbo.pending && pbo.sync) {
		// Проверяем статус без блокировки потока (timeout = 0)
		GLenum status = glClientWaitSync(pbo.sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);

		if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
			// Теперь glMapNamedBufferRange не вернет nullptr, так как буфер создан через glBufferStorage
			void* ptr = glMapNamedBufferRange(pbo.id, 0, 16, GL_MAP_READ_BIT);

			if (ptr) {
				pbo.last_sample = *static_cast<uvec4*>(ptr);
				glUnmapNamedBuffer(pbo.id);
			}

			glDeleteSync(pbo.sync);
			pbo.sync = nullptr;
			pbo.pending = false;
			pbo.active = false;
		}
	}
}