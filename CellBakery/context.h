#pragma once

class Context {
public:
	inline int run();

	Context(GLFWwindow *w) : window(w), rand("seed", 256) {};

private:
	World world;

	shad::Shader cellsShader;
	shad::Shader forceShader;
	shad::Shader petriShader;

	
	inline void control();
	inline void compute();
	inline void graphics();
	inline void gui();

	osl::Random rand;
	osl::uvec2 winSize = osl::uvec2(0u);

	GLint Vsync = 1, VsyncNow = Vsync;

	GLFWwindow *window;
};