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
			camera.ratio(winSize[0] / winSize[1]);
		}
	}

	if (!ImGui::GetIO().WantCaptureMouse) {
		const auto &mp = ImGui::GetIO().MousePos;
		const auto &dmp = ImGui::GetIO().MouseDelta;
		vec2 nmp = vec2(mp[0], mp[1]) / vec2(winSize);
		nmp[1] = 1. - nmp[1];
		vec2 ndmp = vec2(dmp[0], -dmp[1]) / vec2(winSize);
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS)
			ndmp = vec2(0.);

		frac scroll = ImGui::GetIO().MouseWheel;

		vec2 scroll_vec = vec2(scroll) * 0.1;
		camera.pushMouse(nmp, -ndmp, scroll_vec);
	}
}