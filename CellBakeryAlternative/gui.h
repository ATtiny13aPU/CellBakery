#pragma once



inline void Context::gui() {

	// ������ � ImGui
	{
		// ���-�� ������ ��� ImGui
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ��������� ������ ����
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}