#pragma once


class Context {
public:
	inline int run();

	Context(GLFWwindow *w) : window(w) {};

private:
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