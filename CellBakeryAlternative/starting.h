#pragma once



int Context::run() {

	// ���� �������
	glfwSwapInterval(Vsync);
	while (!glfwWindowShouldClose(window)) {
		// ������ � glfw3
		{
			if (VsyncNow != Vsync) {
				glfwSwapInterval(Vsync);
				VsyncNow = Vsync;
			}

			// �������� ������
			glfwPollEvents();

			// ����������� ������ ��� ���������� ����
			int new_xsize_display, new_ysize_display;
			glfwGetFramebufferSize(window, &new_xsize_display, &new_ysize_display);
			if (uint32_t(new_xsize_display) != winSize[0] || uint32_t(new_ysize_display) != winSize[1]) {
				winSize = osl::uvec2(new_xsize_display, new_ysize_display);
				glViewport(0, 0, winSize[0], winSize[1]);
			}
		}


		glfwSwapBuffers(window);
	}

	return 1;
}