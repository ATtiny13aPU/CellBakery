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
		cellsMesh.linkAttributes(0, atr);
	}
	// Настройка SimpleMesh для чашки Петри
	{
		std::vector<float> m = {-1., -1., -1., 1., 1., -1., 1., 1.};
		petriMesh.vbo.data(m);
		petriMesh.linkAttributes(0, shad::attribute_layout{.type = shad::attribute_type::gl_float_t, .count = 2});
	}
	

	// Загрузка шейдеров
	// Шейдеры графики
	{
		cellsShader.name = "cellsShader";
		loadShaderFromFiles(cellsShader, "Shaders/cells.vert", "Shaders/cells.frag", "Shaders/cells.geom");

		forceShader.name = "forceShader";
		loadShaderFromFiles(forceShader, "Shaders/force.vert", "Shaders/force.frag", "Shaders/force.geom");

		petriShader.name = "petriShader";
		loadShaderFromFiles(petriShader, "Shaders/petri.vert", "Shaders/petri.frag");

		petriShader.setUniform("ViewWorld");

		cellsShader.setUniform("TimeLerp");
		cellsShader.setUniform("ViewWorld");
		cellsShader.setUniform("ViewWindow");
		cellsShader.setUniform("WinSize");

		forceShader.setUniform("ViewWorld");
		forceShader.setUniform("ViewWindow");
		forceShader.setUniform("Scale");
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