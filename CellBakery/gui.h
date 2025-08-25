#pragma once



inline void Context::gui() {

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
			//	| ImGuiWindowFlags_NoBackground;
			//ImGui::SetNextWindowSize(ImVec2(150, 20));
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			bool open = true;
			ImGui::Begin(u8"##settings_window", &open, windowFlags);

			
			ImGui::PushItemWidth(500);

			if (ImGui::SliderFloat("##ups_world_set", &ups_world_set, 1.f, 1000.f, "%.1f")) {
				WorldKeyValueCommand c;
				c["ups"] = ups_world_set;

				wkv_commands.push_back(c);

			}
			ImGui::SliderFloat("##scale_force_draw", &scale_force_draw, 0.f, 20.f, "%.1f");
			

			ImGui::PopItemWidth();
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