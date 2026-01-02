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

	// Настройка CustomMesh для клеток
	{
		std::array<shad::attribute_layout, 3> atr = {
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4},
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4},
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4}
		};
		cellsMesh.link_attributes(0, atr);
	}
	// Настройка экранного меша
	{
		std::vector<float> m = { -1., -1., -1., 1., 1., -1., 1., 1. };
		screenMesh.vbo.emplace(m);
		screenMesh.link_attributes(0, shad::attribute_layout{ .type = shad::attribute_type::gl_float_t, .count = 2 });
	}


	// Загрузка шейдеров
	// Шейдеры графики
	{
		cellsShader.name = "cellsShader";
		load_shader_from_files(cellsShader, "Shaders/cells.vert", "Shaders/cells.frag", "Shaders/cells.geom");

		boxShader.name = "boxShader";
		load_shader_from_files(boxShader, "Shaders/box.vert", "Shaders/box.frag", "Shaders/box.geom");

		forceShader.name = "forceShader";
		load_shader_from_files(forceShader, "Shaders/force.vert", "Shaders/force.frag", "Shaders/force.geom");

		petriShader.name = "petriShader";
		load_shader_from_files(petriShader, "Shaders/petri.vert", "Shaders/petri.frag");

		petriShader.location("TimeLerp");
		petriShader.location("ViewWorld");
		petriShader.location("WinSize");
		petriShader.location("MSAA");
		petriShader.location("MSAA_quasi_start");

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
		forceShader.location("Scale");
	}

	// Связываем VBO как SSBO для случайного доступа к графическим данным из под пост-процессора
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cellsMesh.vbo.id());
		shad::link_uniform_block(petriShader.id(), "cells", 0);
		///GLuint resource_index = glGetProgramResourceIndex(petriShader.id(), GL_SHADER_STORAGE_BLOCK, "cells");
		///if (resource_index != GL_INVALID_INDEX) {
		///	glShaderStorageBlockBinding(petriShader.id(), resource_index, 0);
		///}
	}
	WorldAdapter::WorldSettings ws;
	ws.cells_limit = 100000;
	ws.world_size = vec2(sqrt(ws.cells_limit) * (2. / sqrt(10.)));

	camera.set(ws.world_size / 2., ws.world_size);
	// Запуск симуляции в отдельном потоке
	std::jthread simulationThread(&WorldAdapter::run, &world, std::ref(ws));



	// Цикл графики
	glfw::swapInterval(Vsync);

	while (!window.shouldClose()) {

		control();

		sync();

		graphics();

		gui();

		window.swapBuffers();
		glfw::pollEvents();
	}

	// Остановка симуляции и ожидание завершения потока симуляции
	world.stop();

	return 0;
}