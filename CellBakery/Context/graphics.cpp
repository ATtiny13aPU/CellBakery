module;

#include "monolith_std_osl_header.h";
#include "monolith_ogl_imgui_header.h";

module Context;


inline void Context::graphics() {
	frame_counter++;
	// Очистка экрана
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	fvec4 worldView = camera.getDirectView();
	fvec4 windowView = camera.getInverseView();

	// Отрисовка чашки Петри
	{
		glUseProgram(petriShader.glID);
		glUniform4fv(petriShader.getUniform("ViewWorld"), 1, &worldView[0]);

		glBindVertexArray(petriMesh.VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, petriMesh.size);
	}
	// Отрисовка клеток
	{
		glUseProgram(cellsShader.glID);
		glUniform1f(cellsShader.getUniform("TimeLerp"), time_lerp - 1.);
		glUniform4fv(cellsShader.getUniform("ViewWorld"), 1, &worldView[0]);
		glUniform4fv(cellsShader.getUniform("ViewWindow"), 1, &windowView[0]);
		glUniform2fv(cellsShader.getUniform("WinSize"), 1, &winSize[0]);

		glBindVertexArray(cellsMesh.VAO);
		glDrawArrays(GL_POINTS, 0, cellsMesh.size);
	}
	// Отрисовка сил
	if (scale_force_draw > 0.1f) {
		glLineWidth(1.8f);
		glUseProgram(forceShader.glID);
		glUniform4fv(forceShader.getUniform("ViewWorld"), 1, &worldView[0]);
		glUniform4fv(forceShader.getUniform("ViewWindow"), 1, &windowView[0]);
		glUniform1f(forceShader.getUniform("Scale"), float(scale_force_draw / 20.f));

		glDrawArrays(GL_POINTS, 0, cellsMesh.size);
	}
}