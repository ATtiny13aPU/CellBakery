module;
#include "monolith_ogl_imgui_header.h";
module Context;
import osl;
using namespace osl::types;
import shad;

int Context::run() {
	// Включение прозрачности
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Настройка всех атрибутов мешей
	{
		std::array<shad::attribute_layout, 3> atr = {
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4},
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4},
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4}
		};
		cells_vao.link_attributes(0, atr);

		screen_mesh.link_attributes(0, shad::attribute_layout{ .type = shad::attribute_type::gl_float_t, .count = 2 });
	}

	// Настройка всех vbo
	{
		std::vector<float> m = { -1., -1., -1., 1., 1., -1., 1., 1. };
		screen_mesh.vbo.emplace(m);

		for (auto& vbo : world_snapshots_storage)
			vbo.setup(shad::usage_order::by_default, shad::storage_flags::gl_map_coherent_write);

		frame_id_immutable_buffer.setup(shad::usage_order::by_default, shad::storage_flags::gl_map_coherent_read);
		frame_id_immutable_buffer.allocate(sizeof(uint32_t) * 4u);
		std::span<uint32_t> s = frame_id_immutable_buffer.as_span<uint32_t>();
		frame_counter_mapped_ptr = s.data();
	}

	// Загрузка шейдеров
	{
		cellsShader.name = "cellsShader";
		load_shader_from_files(cellsShader, "Shaders/cells.vert", "Shaders/cells.frag", "Shaders/cells.geom");

		boxShader.name = "boxShader";
		load_shader_from_files(boxShader, "Shaders/cells.vert", "Shaders/box.frag", "Shaders/box.geom");

		forceShader.name = "forceShader";
		load_shader_from_files(forceShader, "Shaders/force.vert", "Shaders/force.frag", "Shaders/force.geom");

		petriShader.name = "petriShader";
		load_shader_from_files(petriShader, "Shaders/petri.vert", "Shaders/petri.frag");

		petriShader.location("TimeLerp");
		petriShader.location("ViewWorld");
		petriShader.location("WinSize");
		petriShader.location("MSAA");
		petriShader.location("MSAA_quasi_start");
		shad::link_uniform_block(petriShader.id(), "cells", 0);

		cellsShader.location("TimeLerp");
		cellsShader.location("ViewWorld");
		cellsShader.location("ViewWindow");
		cellsShader.location("WinSize");

		boxShader.location("TimeLerp");
		boxShader.location("ViewWorld");
		boxShader.location("ViewWindow");
		boxShader.location("WinSize");

		forceShader.location("ViewWorld");
		forceShader.location("ViewWindow");
		forceShader.location("ScaleForce");
		forceShader.location("ScaleVel");
	}

	// Ручная настройка PBO для получения одного экранного пикселя
	{
		glGenBuffers(1, &pbo.id);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo.id);
		glBufferStorage(GL_PIXEL_PACK_BUFFER, 16, nullptr, GL_MAP_READ_BIT | GL_DYNAMIC_STORAGE_BIT);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}

	wkv_push_commands.emplace_back("ups", gui_s.ups_world_set);
	wkv_push_commands.emplace_back("sync_limit", 300);

	/*
		Создание и запуск мира. Сейчас это происходит сразу при запуске Context,
		но планируется более сложное поведение
		включая возможность создания нескольких миров
		а так же автоматизированное создание миров логикой скриптов
	*/
	WorldAdapter::world_settings_t ws;
	ws.cells_limit = 100000; // Это не жёсткий лимит, а лишь рекомендация.
	ws.world_size = vec2(sqrt(ws.cells_limit) * (2. / sqrt(10.)));

	// Направляем камеру на "центр" мира
	camera.set(vec2(0.), ws.world_size * 1.2);
	// Запуск симуляции в отдельном потоке
	std::jthread simulationThread(&WorldAdapter::run, &world, std::ref(ws));

	// Цикл графики
	glfw::swapInterval(Vsync);

	while (!window.shouldClose()) {
		frame_counter++;
		frame_counter_proxy = uvec4(frame_counter);
		frame_rate_counter.push();

		/*
		control();
			принимаем эвенты от OpenGL контекста, обновляем состояние мыши и прочий пользовательский ввод
			обрабатываем состояние смены разрешения окна и запрос на изменение размера экранной текстуры

		sync();
			принимаем эвенты мира и проверяем состояние world_snapshots на наличие новых кадров симуляции
			в случае наличия таковых вызывает функцию загрузки данных на видеокарту

		graphics();
			содержит основные вызовы графики в цикле

		gui();
			содержит взаимодействие с ImGui
		*/
		control();

		sync();

		graphics();

		gui();


		// отправка накопленных команд в мир, генерируемых из GUI и автоматизированные запросы
		if (!wkv_push_commands.empty()) {
			world.push_wkv_commands(wkv_push_commands);
			wkv_push_commands.clear();
		}

		window.swapBuffers();
		glfw::pollEvents();
	}

	// Остановка симуляции и ожидание завершения потока симуляции
	world.stop();

	return 0;
}