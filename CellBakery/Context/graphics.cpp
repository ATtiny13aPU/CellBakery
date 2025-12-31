module;
#include "monolith_ogl_imgui_header.h"
module Context;
import osl;
using namespace osl::types;
import shad.base;

void Context::graphics() {
	frame_counter++;
	// Очистка экрана
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	fvec4 worldView = camera.getDirectView();
	fvec4 windowView = camera.getInverseView();
	
	// Отрисовка чашки Петри
	{
		petriShader.use();
		petriShader.uniform("ViewWorld", worldView);

		petriMesh.draw(shad::draw_primitive::gl_triangle_strip);
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
	// Отрисовка сил
	if (scale_force_draw > 0.1f) {
		forceShader.use();
		glLineWidth(1.8f);
		forceShader.uniform("ViewWorld", worldView);
		forceShader.uniform("ViewWindow", windowView);
		forceShader.uniform("Scale", static_cast<float>(scale_force_draw / 20.f));
	
		cellsMesh.draw(shad::draw_primitive::gl_points);
	}
}