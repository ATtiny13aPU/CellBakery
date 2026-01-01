module;
#include "monolith_ogl_imgui_header.h";
module Context;
import osl;
using namespace osl::types;
import shad;

void Context::gui() {

	// Работа с ImGui
	{
		// Что-то нужное для ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Небольшое контекстное меню управления миром
		{
			const ImGuiWindowFlags windowFlags =
				ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			ImGui::SetNextWindowPos(ImVec2(0, 0));
			bool open = true;
			ImGui::Begin("##settings_window", &open, windowFlags);


			ImGui::PushItemWidth(400);

			if (ImGui::SliderFloat("##ups_world_set", &ups_world_set, 4.f, 1000.f, "%.1f")) {
				WorldKeyValueCommand c;
				c["ups"] = ups_world_set;

				wkv_commands.push_back(c);

			}
			ImGui::SliderFloat("##scale_force_draw", &scale_force_draw, 0.f, 20.f, "%.1f");

			ImGui::PopItemWidth();
			bool check = Vsync;
			if (ImGui::Checkbox("vsync", &check)) {
				Vsync = check;
				glfw::swapInterval(Vsync);
			}
			ImGui::SameLine();
			ImGui::Text(" %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			ImGui::End();
		}

		// Запускаем рендер меню
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	if (!wkv_commands.empty()) {
		world.pushWKVCommands(wkv_commands);
	}
}