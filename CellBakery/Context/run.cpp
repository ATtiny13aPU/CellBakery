module;
#include "monolith_ogl_imgui_header.h";
module Context;
import osl;
using namespace osl::types;
import shad.base;

int Context::run() {
	// Включение прозрачности
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Сглаживание (только вот как...)
	//glfwWindowHint(GLFW_SAMPLES, 16);
	//glEnable(GL_MULTISAMPLE);

	// Настройка CustomMesh для клеток
	{
		std::array<shad::attribute_layout, 3> atr = {
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4},
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4},
			shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 4}
		};
		cellsMesh.link_attributes(0, atr);
	}
	// Настройка SimpleMesh для чашки Петри
	{
		std::vector<float> m = {-1., -1., -1., 1., 1., -1., 1., 1.};
		petriMesh.vbo.emplace(m);
		petriMesh.link_attributes(0, shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 2});
	}
	

	// Загрузка шейдеров
	// Шейдеры графики
	{
		cellsShader.name = "cellsShader";
		load_shader_from_files(cellsShader, "Shaders/cells.vert", "Shaders/cells.frag", "Shaders/cells.geom");

		forceShader.name = "forceShader";
		load_shader_from_files(forceShader, "Shaders/force.vert", "Shaders/force.frag", "Shaders/force.geom");

		petriShader.name = "petriShader";
		load_shader_from_files(petriShader, "Shaders/petri.vert", "Shaders/petri.frag");

		petriShader.location("ViewWorld");

		cellsShader.location("TimeLerp");
		cellsShader.location("ViewWorld");
		cellsShader.location("ViewWindow");
		cellsShader.location("WinSize");

		forceShader.location("ViewWorld");
		forceShader.location("ViewWindow");
		forceShader.location("Scale");
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