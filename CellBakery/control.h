#pragma once


inline void Context::control() {
	// Работа с glfw3
	{
		if (VsyncNow != Vsync) {
			glfwSwapInterval(Vsync);
			VsyncNow = Vsync;
		}

		// Получаем эвенты
		glfwPollEvents();

		// Настраиваем камеру под разрешение окна
		int new_xsize_display, new_ysize_display;
		glfwGetFramebufferSize(window, &new_xsize_display, &new_ysize_display);
		if (uint32_t(new_xsize_display) != winSize[0] || uint32_t(new_ysize_display) != winSize[1]) {
			winSize = osl::fvec2(new_xsize_display, new_ysize_display);
			glViewport(0, 0, winSize[0], winSize[1]);
		}
	}





	// Вычисление области видимости
	osl::fvec2 mnPos = view.pos - osl::fvec2(view.mst * winSize[0] / winSize[1], view.mst) * 0.5f;
	osl::fvec2 mxPos = view.pos + osl::fvec2(view.mst * winSize[0] / winSize[1], view.mst) * 0.5f;
	viewWorld = osl::fvec4(mnPos[0], mnPos[1], mxPos[0], mxPos[1]);
}