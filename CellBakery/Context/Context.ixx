module;
#include "monolith_ogl_imgui_header.h";

export module Context;
import osl;
using namespace osl::types;
import shad.base;
import vspefs.glfwppm;
import World;

import std;
namespace fs = std::filesystem;

class Context {
public:
	int run();

	explicit Context(glfw::Window &w) : window(w), rand("seed", 256) {};

private:
	WorldAdapter world;

	osl::CameraController2D camera;

	shad::Shader cellsShader;
	shad::Shader forceShader;
	shad::Shader petriShader;

	shad::SimpleMesh cellsMesh;
	shad::SimpleMesh petriMesh;

	fvec2 winSize;

	uint64_t frame_counter = 0u;
	uint64_t last_update_frame = 0u;
	frac32 time_lerp = 0.;
	frac32 delta_time_lerp = 0.;
	osl::fastMovingAverageW<5> framePerUpdate;
	
	void control();
	void sync();
	void graphics();
	void gui();

	osl::random rand;

	GLint Vsync = 1;
	GLint VsyncNow = Vsync;

	glfw::Window &window;

	// GUI
	float ups_world_set = 5.f;
	float scale_force_draw = 5.f;

	WorldKeyValueCommands wkv_commands;
};




export int main_too() {
	std::locale::global(std::locale("en_US.UTF-8"));

	auto glfwInit = glfw::init();

	glfw::WindowHints{
		.contextVersionMajor = 4, .contextVersionMinor = 6,
		.openglProfile = glfw::OpenGlProfile::Core
	}.apply();

	glfw::Window window{ 640, 480, "CellBakery" };
	glfw::makeContextCurrent(window);

	if (!gladLoadGL()) {
		return 1;
	}

	// загрузка и настройка imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450 core");

	// поиск шрифта
	try {
		fs::path font;
		auto dir = fs::path("./");
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
			std::cout << u8"Imgui: файл шрифта не найден\n"_cpp17 << std::endl;
		}
	}
	catch (const fs::filesystem_error& e) {
		std::cerr << "filesystem error: " << e.what() << std::endl;
	}

	// не сохранять состояние меню imgui в файл
	ImGui::GetIO().IniFilename = nullptr;
	int r = -1;
	try {
		Context c(window);
		r = c.run();
	}
	catch (const std::system_error& e) {
		std::cerr << "Context error: " << e.what() << std::endl;
	}

	return r;
}