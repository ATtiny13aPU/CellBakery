module;
#include "monolith_ogl_imgui_header.h";
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
		glUniform4fv(petriShader.getUniform("ViewWorld"), 1, &worldView[0]);

		petriMesh.draw(shad::draw_primitive::gl_triangle_strip);
	}

	// Отрисовка клеток
	{
		cellsShader.use();
		glUniform1f(cellsShader.getUniform("TimeLerp"), time_lerp - 1.);
		glUniform4fv(cellsShader.getUniform("ViewWorld"), 1, &worldView[0]);
		glUniform4fv(cellsShader.getUniform("ViewWindow"), 1, &windowView[0]);
		glUniform2fv(cellsShader.getUniform("WinSize"), 1, &winSize[0]);

		cellsMesh.draw(shad::draw_primitive::gl_points);
	}
	// Отрисовка сил
	if (scale_force_draw > 0.1f) {
		forceShader.use();
		glLineWidth(1.8f);
		glUniform4fv(forceShader.getUniform("ViewWorld"), 1, &worldView[0]);
		glUniform4fv(forceShader.getUniform("ViewWindow"), 1, &windowView[0]);
		glUniform1f(forceShader.getUniform("Scale"), float(scale_force_draw / 20.f));
	
		cellsMesh.draw(shad::draw_primitive::gl_points);
	}
}