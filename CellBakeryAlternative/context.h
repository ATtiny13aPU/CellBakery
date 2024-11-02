#pragma once


class Context {
public:
	int run();
	Context(GLFWwindow *w) : window(w) {};
private:
	int err = 0;
	osl::Randomaizer<8192> RAND;
	osl::uvec2 winSize = osl::uvec2(0u);

	size_t Vsync = 1, VsyncNow = Vsync;

	GLFWwindow *window;
};