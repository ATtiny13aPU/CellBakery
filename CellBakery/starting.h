#pragma once




int Context::run() {

	// шейдеры графики
	{
		petriShader.name = std::string("petriShader");
		loadShaderFromFiles(petriShader, "Shaders/petri.vert", "Shaders/petri.frag");

		cellsShader.name = std::string("cellsShader");
		loadShaderFromFiles(cellsShader, "Shaders/cells.vert", "Shaders/cells.frag", "Shaders/cells.geom");
	}

	World world;
	World::WorldSettings ws;
	ws.world_size = vec2(100.);
	ws.cells_limit = 50000;
	world.run(ws);

	// Цикл графики
	glfwSwapInterval(Vsync);

	while (!glfwWindowShouldClose(window)) {

		control();

		compute();

		graphics();

		gui();

		glfwSwapBuffers(window);
	}

	return 1;
}