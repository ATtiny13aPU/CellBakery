#pragma once

class Context {
public:
	inline int run();

	Context(GLFWwindow *w) : window(w), rand("seed", 256), frame_counter(0u) {};

private:
	WorldAdapter world;

	CameraController2D camera;

	shad::Shader cellsShader, forceShader, petriShader;

	shad::CustomMesh cellsMesh;
	shad::SimpleMesh petriMesh;

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

	// GUI
	float ups_world_set = 5.f;
	float scale_force_draw = 5.f;
};