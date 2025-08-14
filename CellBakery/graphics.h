#pragma once


inline void Context::graphics() {
	frame_counter++;
	// Очистка экрана
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);

	// Отрисовка чашки Петри
	{
		glUseProgram(petriShader.glID);
		glUniform4fv(petriShader.getUniform("ViewWorld"), 1, &viewWorld[0]);

		glBindVertexArray(petriMesh.VAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, petriMesh.size);
	}
	// Отрисовка клеток
	{
		glUseProgram(cellsShader.glID);
		glUniform1f(cellsShader.getUniform("TimeLerp"), time_lerp - 1.);
		glUniform4fv(cellsShader.getUniform("ViewWorld"), 1, &viewWorld[0]);
		glUniform2fv(cellsShader.getUniform("WinSize"), 1, &winSize[0]);

		glBindVertexArray(cellsMesh.VAO);
		glDrawArrays(GL_POINTS, 0, cellsMesh.size);
	}
	// Отрисовка сил
	{
		glLineWidth(2.5f);
		glUseProgram(forceShader.glID);
		glUniform4fv(forceShader.getUniform("ViewWorld"), 1, &viewWorld[0]);
		glUniform2fv(forceShader.getUniform("WinSize"), 1, &winSize[0]);

		glDrawArrays(GL_POINTS, 0, cellsMesh.size);
	}
}