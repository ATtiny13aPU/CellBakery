#pragma once


class Context {
public:
	inline int run();

	Context(GLFWwindow *w) : window(w), CellsSSBO(0), GridSSBO(1), RandSSBO(2), cellsPostProcessingSSBO(3) {};

private:

	shad::Shader cellsShader;
	shad::Shader petriShader;

	// буфер хранения информации о клетках
	shad::SSBO CellsSSBO;
	// буфер хранения информации о игровом поле
	shad::SSBO GridSSBO;
	// буфер хранения внутреннего стейта ГПСЧ
	shad::SSBO RandSSBO;
	// буфер хранения ближайших к фрагментам id клеток
	shad::SSBO cellsPostProcessingSSBO;
	
	inline void control();
	inline void compute();
	inline void graphics();
	inline void gui();

	int err = 0;
	osl::Randomaizer<8192> RAND;
	osl::uvec2 winSize = osl::uvec2(0u);

	GLint Vsync = 1, VsyncNow = Vsync;

	GLFWwindow *window;
};