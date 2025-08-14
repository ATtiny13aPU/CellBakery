#pragma once




int Context::run() {
	glfwSwapInterval(Vsync); // Включение вертикальной синхронизации

	// Включение прозрачности
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Сглаживание
	glfwWindowHint(GLFW_SAMPLES, 16);
	glEnable(GL_MULTISAMPLE);

	// Настройка CustomMesh для клеток
	cellsMesh.setAttribFloat({ 4, 4, 4 });

	// Настройка SimpleMesh для чашки Петри
	{
		std::vector<float> m = { -1., -1., -1., 1., 1., -1., 1., 1. };
		petriMesh.loadFrom(&m[0], m.size());
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
		cellsShader.setUniform("WinSize");

		forceShader.setUniform("ViewWorld");
		forceShader.setUniform("WinSize");
	}


	WorldAdapter::WorldSettings ws;
	ws.cells_limit = 100000;
	ws.world_size = vec2(std::sqrt(ws.cells_limit));
	// Запуск симуляции в отдельном потоке
	std::thread simulationThread(&WorldAdapter::run, &world, std::ref(ws));
	
	

	// Цикл графики
	glfwSwapInterval(Vsync);

	while (!glfwWindowShouldClose(window)) {

		control();

		sync();

		graphics();

		gui();

		glfwSwapBuffers(window);
	}

	// Остановка симуляции и ожидание завершения потока симуляции
	world.stop();
	simulationThread.join();

	return 0;
}