module;
#include "monolith_ogl_imgui_header.h";

export module Context;
import osl;
using namespace osl::types;
import shad;
import vspefs.glfwppm;
import World;

import std;
namespace fs = std::filesystem;

class Context {
public:
	int run();

	explicit Context(glfw::Window& w) : window(w), rand("seed", 256), gui_s{} {};

private:
	WorldAdapter world;
	WorldKeyValueCommands wkv_push_commands;
	WorldKeyValueCommands wkv_pull_commands;

	/*
		Это хранилище для тройной буферизации кадров мира.
		Нужно для того, чтобы избежать аллокаций в самом мире,
		поскольку планируется выделять память сразу на видеокарте.

		Запрещено читать содержимое этого массива напрямую,
		нужно использовать только методы WorldAdapter для доступа к кадрам.
	*/
	std::array<shad::vbo_im, 3> world_snapshots_storage;
	uint32_t current_vbo_gl_id = 0u;
	uint32_t current_vbo_index = 0u;

	shad::base_buffer_t<GL_BUFFER, true> frame_id_immutable_buffer;

	osl::camera_controller_2d camera;

	shad::shader cellsShader;
	shad::shader boxShader;
	shad::shader forceShader;
	shad::shader petriShader;

	shad::simple_mesh cellsMesh;
	shad::simple_mesh screenMesh;

	//std::unique_ptr<shad::texture2d> frame_texture;
	GLuint frame_texture_id = 0;
	struct {
		GLuint id;
		GLsync sync = nullptr;
		bool pending = false;
		bool active = false;
		uvec4 last_sample = uvec4(-1);
	} pbo;

	fvec2 win_size;

	uint64_t frame_counter = 0u;
	uint64_t last_update_frame = 0u;
	uvec4 frame_counter_proxy = uvec4(0);
	uint32_t* frame_counter_mapped_ptr = nullptr;

	frac32 time_lerp = 0.;
	frac32 delta_time_lerp = 0.;
	osl::fastMovingAverageW<5> framePerUpdate;
	
	void control();
	void sync();
	void graphics();
	void gui();

	osl::random rand;

	glfw::Window &window;

	GLint Vsync = 1;
	GLint VsyncNow = Vsync;

	// GUI
	struct GuiState {
		GuiState() {
			init_template();
		}

		// Симуляция
		float ups_world_set = 10.f;
		bool no_update_flag = false;
		bool pause_simulation = false;

		// Графика
		float MSAA = 4.f;
		bool MSAA_quasi_start = false;
		GLint Vsync = 1;

		// Визуализация объектов
		float scale_force_draw = 0.f;
		float scale_vel_draw = 0.f;
		bool show_cells = true;
		bool show_forces = true;
		bool show_boxes = false;

		// Создание клеток
		bool is_placing_cell = false;
		cell_t template_cell;

		// Вспомогательная структура для UI-состояний
		struct {
			int current_type_idx = 0;
			float radius = 0.5f;
			float weight = 1.0f;
			bool is_dragging_speed = false; // Флаг для визуализации "натяжения" скорости 
			// Здесь можно хранить данные, которые нужны только в момент отрисовки
		} ui;

		void init_template() {
			template_cell.radius = 0.5;
			template_cell.color = fvec4(1.0f, 0.5f, 0.2f, 1.0f);
			template_cell.velocity = vec2(0.0);
			template_cell.weight = 1.0;

			// Синхронизируем UI мост
			ui.radius = static_cast<float>(template_cell.radius);
			ui.weight = static_cast<float>(template_cell.weight);
			ui.current_type_idx = static_cast<int>(template_cell.type);
		}
	} gui_s;


	void gui_graphics_m();
	void gui_world_control_m();
	void gui_focus_cell_info_m();
	void gui_creation_m();
};




export int main_too() {
	std::locale::global(std::locale("en_US.UTF-8"));

	auto glfwInit = glfw::init();

	{
		glfw::WindowHints wh{
			.contextVersionMajor = 4, .contextVersionMinor = 6,
			.openglProfile = glfw::OpenGlProfile::Core
		};
		//	wh.samples = 16;
		wh.apply();
		//	glEnable(GL_MULTISAMPLE);
	}
	glfw::Window window{ 640, 480, "CellBakery" };
	glfw::makeContextCurrent(window);

	if (!gladLoadGL())
		return 1;

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