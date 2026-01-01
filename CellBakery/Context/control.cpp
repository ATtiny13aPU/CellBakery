module;
#include "monolith_ogl_imgui_header.h";

module Context;
import osl;
using namespace osl::types;
import shad;

void Context::control() {
	// Работа с glfw3
	{
		if (VsyncNow != Vsync) {
			glfw::swapInterval(Vsync);
			VsyncNow = Vsync;
		}

		// Получаем эвенты
		glfw::pollEvents();

		// Настраиваем камеру под разрешение окна
		auto[new_xsize_display, new_ysize_display] = window.getFramebufferSize();

		if (uint32_t(new_xsize_display) != winSize[0] || uint32_t(new_ysize_display) != winSize[1]) {
			winSize = osl::fvec2(new_xsize_display, new_ysize_display);
			glViewport(0, 0, winSize[0], winSize[1]);
			camera.ratio(winSize[0] / winSize[1]);

			// Фреймбуфер
			{
				if (frame_texture_id)
					glDeleteTextures(1, &frame_texture_id);
				glCreateTextures(GL_TEXTURE_2D, 1, &frame_texture_id);
				glTextureStorage2D(frame_texture_id, 1, GL_RGBA32UI, winSize[0], winSize[1]);
				//frame_texture = std::make_unique<shad::texture2d>(shad::texture2d());
				//frame_texture->create(winSize[0], winSize[1], shad::texture_format::rgba32ui);
			}
		}
	}

	if (!ImGui::GetIO().WantCaptureMouse) {
		const auto& mp = ImGui::GetIO().MousePos;
		const auto& dmp = ImGui::GetIO().MouseDelta;
		vec2 nmp = vec2(mp[0], mp[1]) / vec2(winSize);
		nmp[1] = 1. - nmp[1];
		vec2 ndmp = vec2(dmp[0], -dmp[1]) / vec2(winSize);
		
		if (!window.getMouseButton(glfw::MouseButton::Left))
			ndmp = vec2(0.);

		frac scroll = ImGui::GetIO().MouseWheel;

		vec2 scroll_vec = vec2(scroll) * 0.1;
		camera.pushMouse(nmp, -ndmp, scroll_vec);
	}
}