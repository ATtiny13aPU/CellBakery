#pragma once



int Context::run() {

	// Цикл графики
	glfwSwapInterval(Vsync);
	while (!glfwWindowShouldClose(window)) {
		// Работа с glfw3
		{
			if (VsyncNow != Vsync) {
				glfwSwapInterval(Vsync);
				VsyncNow = Vsync;
			}

			// Получаем эвенты
			glfwPollEvents();

			// Настраиваем камеру под разрешение окна
			glfwGetFramebufferSize(window, reinterpret_cast<int*>(&winSize[0]), reinterpret_cast<int*>(&winSize[1]));
			glViewport(0, 0, winSize[0], winSize[1]);
		}


		glfwSwapBuffers(window);
	}

	return 1;
}