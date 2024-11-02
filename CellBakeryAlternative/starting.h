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
			glfwGetFramebufferSize(window, reinterpret_cast<int*>(&winSize[0]), reinterpret_cast<int*>(&winSize[1]));
			glViewport(0, 0, winSize[0], winSize[1]);
		}


		glfwSwapBuffers(window);
	}

	return 1;
}