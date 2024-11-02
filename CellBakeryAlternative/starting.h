#pragma once



int Context::run() {
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