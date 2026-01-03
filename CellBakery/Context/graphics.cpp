module;
#include "monolith_ogl_imgui_header.h"
module Context;
import osl;
using namespace osl::types;
import shad;

void Context::graphics() {
	frame_counter++;
	// Очистка экрана
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	fvec4 worldView = camera.getDirectView();
	fvec4 windowView = camera.getInverseView();

	
	// Привязка и отчистка текстуры
	{
		glBindImageTexture(0, frame_texture_id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32UI);
		static std::array<uint32_t, 4> color_reset = { -1, -1, -1, -1 };
		glClearTexImage(
			frame_texture_id,       // ID текстуры (в вашем случае 17)
			0,                // level (мип-уровень)
			GL_RGBA_INTEGER,  // format (ОБЯЗАТЕЛЬНО с суффиксом _INTEGER)
			GL_UNSIGNED_INT,  // type (тип данных в массиве clear_color)
			color_reset.data()
		);
		//glBindImageTexture(0, frame_texture->id(), 0, GL_FALSE, 0, static_cast<GLenum>(shad::access_order::gl_read_write), GL_RGBA32UI);
		////frame_texture->bind_as_image(0, 0, shad::access_order::gl_read_write);
		//static std::array<uint32_t, 4> color_reset = { -1, -1, -1, -1 };
		//frame_texture->clear_color(color_reset.data(), shad::pixel_format::rgba, shad::pixel_type::ubyte_t);
	}

	// Отрисовка клеток
	{
		cellsShader.use();
		cellsShader.uniform("TimeLerp", time_lerp - 1.f);
		cellsShader.uniform("ViewWorld", worldView);
		cellsShader.uniform("ViewWindow", windowView);
		cellsShader.uniform("WinSize", winSize);

		cellsMesh.draw(shad::draw_primitive::gl_points);
	}

	// Отрисовка чашки Петри (Пост-эффект отрисовка)
	{
		petriShader.use(); 
		petriShader.uniform("TimeLerp", time_lerp - 1.f);
		petriShader.uniform("ViewWorld", worldView);
		petriShader.uniform("WinSize", winSize);
		petriShader.uniform("MSAA", MSAA);
		petriShader.uniform("MSAA_quasi_start", float(MSAA_quasi_start));

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		screenMesh.draw(shad::draw_primitive::gl_triangle_strip);
	}

	// Отрисовка коробок
	if (0) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		boxShader.use();
		boxShader.uniform("TimeLerp", time_lerp - 1.f);
		boxShader.uniform("ViewWorld", worldView);
		boxShader.uniform("ViewWindow", windowView);
		boxShader.uniform("WinSize", winSize);

		cellsMesh.draw(shad::draw_primitive::gl_points);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Отрисовка сил
	if (1) // Шейдер не работает более, потому что поле meta занят
	if (scale_force_draw > 0.1f) {
		forceShader.use();
		glLineWidth(1.8f);
		forceShader.uniform("ViewWorld", worldView);
		forceShader.uniform("ViewWindow", windowView);
		forceShader.uniform("Scale", static_cast<float>(scale_force_draw / 20.f));

		cellsMesh.draw(shad::draw_primitive::gl_points);
	}
}