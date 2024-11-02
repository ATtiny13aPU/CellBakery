#include "include.h"

class Context {
public:
	int run();
	Context(GLFWwindow *w) : window(w) {};
private:
	int err = 0;
	osl::Randomaizer<8192> RAND;
	osl::uvec2 winSize = osl::uvec2(0u);

	size_t Vsync = 1, VsyncNow = Vsync;

	GLFWwindow *window;
};


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




int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	auto window = glfwCreateWindow(800, 600, "CellBakery Alternative", NULL, NULL);
	if (window == NULL) return -1;

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

	Context c(window);
	int r = c.run();

	glfwDestroyWindow(window);
	glfwTerminate();

	return r;
}