#include "include.h"


int main() {
	std::locale::global(std::locale("en_US.UTF-8"));
	
	if (!glfwInit()) {
		std::cerr << "Ошибка инициализации GLFW" << std::endl;
		return 1;
	}
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	auto window = glfwCreateWindow(800, 600, "CellBakeryRecovery", nullptr, nullptr);
	if (window == nullptr) {
		std::cerr << "Ошибка создания окна" << std::endl;
		return 2;
	}
	
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Ошибка загрузки функций glad" << std::endl;
		return 3;
	}

	Context c(window);
	int r = c.run();

	glfwDestroyWindow(window);
	glfwTerminate();

	return r;
}