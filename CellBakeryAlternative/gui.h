#pragma once



inline void Context::gui() {

	// Работа с ImGui
	{
		// Что-то нужное для ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Запускаем рендер меню
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}