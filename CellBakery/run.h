#pragma once




int Context::run() {

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

	World::WorldSettings ws;
	ws.cells_limit = 100000;
	ws.world_size = vec2(std::sqrt(ws.cells_limit));
	World world;
	// Запуск симуляции в отдельном потоке
	std::thread simulationThread(&World::run, &world, ws);

	// Цикл графики
	glfwSwapInterval(Vsync);

	while (!glfwWindowShouldClose(window)) {

		control();

		compute();

		graphics();

		gui();

		glfwSwapBuffers(window);
	}

	// Остановка симуляции и ожидание завершения потока
	world.stop();
	simulationThread.join();

	return 0;
}