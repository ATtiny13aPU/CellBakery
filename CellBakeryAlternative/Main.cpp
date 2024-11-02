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

	// загрузка и настройка imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430 core");

	// поиск шрифта
	try {
		fs::path font;
		auto dir = fs::path("");
		// Проходим по всем файлам в директории
		for (const auto& entry : fs::directory_iterator(dir)) {
			if (entry.is_regular_file() && entry.path().extension() == ".ttf") {
				font = entry.path();
				break; // Прекращает поиск после нахождения первого подходящего файла
			}
		}

		if (!font.empty()) {
			ImGui::GetIO().Fonts->AddFontFromFileTTF(font.generic_string().c_str(), 18.f, nullptr, ImGui::GetIO().Fonts->GetGlyphRangesCyrillic());
		}
		else {
			std::cout << "Imgui: файл шрифта не найден\n" << std::endl;
		}
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "filesystem error: " << e.what() << std::endl;
	}

	// не сохранять состояние меню imgui в файл
	ImGui::GetIO().IniFilename = nullptr;

	Context c(window);
	int r = c.run();

	glfwDestroyWindow(window);
	glfwTerminate();

	return r;
}