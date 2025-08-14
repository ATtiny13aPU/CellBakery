#pragma once

class Context {
public:
	inline int run();

	Context(GLFWwindow *w) : window(w), rand("seed", 256), frame_counter(0u) {};

private:
	WorldAdapter world;

	struct View {
		osl::fvec2 pos = osl::fvec2(25.0f, 25.0f); // Позиция центра камеры
		float mst = 60.0f;                         // Масштаб (высота видимой области)
	};
	View view;

	shad::Shader cellsShader, forceShader, petriShader;

	shad::CustomMesh cellsMesh;
	shad::SimpleMesh petriMesh;

	osl::fvec4 viewWorld;
	osl::fvec2 winSize;

	uint64_t frame_counter = 0u, last_update_frame = 0u;
	frac32 time_lerp = 0.;
	frac32 delta_time_lerp = 0.;
	osl::fastMovingAverageW<5> framePerUpdate;
	
	inline void control();
	inline void sync();
	inline void graphics();
	inline void gui();

	osl::Random rand;

	GLint Vsync = 1, VsyncNow = Vsync;

	GLFWwindow *window;
};