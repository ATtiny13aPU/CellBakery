module;
#include "monolith_ogl_imgui_header.h";
module Context;
import osl;
using namespace osl::types;
import shad;

// Общая GUI хост функция
void Context::gui() {
	// Подготовка кадра ImGui
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Фиксированное окно управления
	const ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	bool open = true;

	if (ImGui::Begin(u8"Управление CellBakery"_cpp17, &open, windowFlags)) {

		if (ImGui::BeginTabBar("MainTabs")) {

			if (ImGui::BeginTabItem(u8"Мир"_cpp17)) {
				gui_world_control_m();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"Графика"_cpp17)) {
				gui_graphics_m();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"Инфо"_cpp17)) {
				gui_focus_cell_info_m();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"Создать"_cpp17)) {
				gui_creation_m();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}


		// Отображаем FPS и UPS
		ImGui::Separator();

		// Получаем последние валидные данные бенчмарка
		const auto& bench = world.last_capture()->bench;
		if (!bench.empty()) {
			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
			//const double current_ups = bench.contains("mspu") ? 1000.0 / bench.at("mspu") : 0.0;
			const double current_ups = bench.contains("total_ups") ? bench.at("total_ups") : 0.0;
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
				u8"FPS: %.1f (%.1f ms) | UPS: %.1f / %.f"_cpp17,
				ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate, current_ups, 1000. / bench.at("gap"));
			ImGui::PopFont();
		}
	}
	ImGui::End();

	// Рендер
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Элементы управления графикой
void Context::gui_graphics_m() {
	// Управление VSync
	if (ImGui::Checkbox(u8"Вертикальная синхронизация"_cpp17, (bool*)&gui_s.Vsync)) {
		glfw::swapInterval(gui_s.Vsync);
	}

	ImGui::Text(u8"Сглаживание (MSAA):"_cpp17);
	ImGui::SliderFloat(u8"##msaa_val"_cpp17, &gui_s.MSAA, 1.f, 16.f, "%.0f");
	ImGui::Checkbox(u8"Квази-старт MSAA"_cpp17, &gui_s.MSAA_quasi_start);

	ImGui::Text(u8"Визуализация элементов"_cpp17);
	ImGui::Checkbox(u8"Отрисовка клеток"_cpp17, &gui_s.show_cells);
	ImGui::Checkbox(u8"Отрисовка границ (Boxes)"_cpp17, &gui_s.show_boxes);

	ImGui::Checkbox(u8"Отображать векторы сил"_cpp17, &gui_s.show_forces);
	if (gui_s.show_forces) {
		ImGui::Text(u8"Масштаб сил:"_cpp17);
		ImGui::SliderFloat("##gui_s.scale_force_draw", &gui_s.scale_force_draw, 0.f, 20.f, "%.1f");
		ImGui::SliderFloat("##gui_s.scale_vel_draw", &gui_s.scale_vel_draw, 0.f, 20.f, "%.1f");
	}
}

// Элементы состояния мира и управление выполнением
void Context::gui_world_control_m() {
	ImGui::Text(u8"Состояние:"_cpp17);
	ImGui::BeginGroup();
	if (ImGui::Button(gui_s.no_update_flag ? u8"Запустить"_cpp17 : u8"Приостановить"_cpp17, ImVec2(150, 0))) {
		gui_s.no_update_flag = !gui_s.no_update_flag;
		if (gui_s.no_update_flag)
			wkv_push_commands.emplace_back("ups", float(-1.));
		else
			wkv_push_commands.emplace_back("ups", gui_s.ups_world_set);
	}

	if (ImGui::Button(u8"Пересоздать"_cpp17, ImVec2(150, 0))) {
		wkv_push_commands.emplace_back("restart", uint64_t(0));
	}
	ImGui::EndGroup();

	ImGui::SameLine();

	ImGui::BeginGroup();

	if (ImGui::Button(u8"Быстрое сохранение"_cpp17, ImVec2(180, 0))) {
	}

	if (ImGui::Button(u8"Быстрая загрузка"_cpp17, ImVec2(180, 0))) {
	}
	ImGui::EndGroup();

	ImGui::Spacing();
	ImGui::Text(u8"Производительность:"_cpp17);

	const auto& bench = world.last_capture()->bench;

	if (!bench.empty()) {
		// Вывод временных интервалов (gap1 - подготовка, gap2 - вычисления)
		ImGui::Text(u8" Сортировка и поиск\n %.2f + %.2f = %.2f мс"_cpp17,
			bench.at("gap1"), bench.at("gap2"), bench.at("gap"));
		
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(u8"Отдельно время на подготовку перед алгоритмомо поиска коллизий\nи отдельно время на его работу. (сумма времени обработки физики)"_cpp17);

		ImGui::BulletText(u8"Обработка коллизий: %.2f мс"_cpp17, bench.at("collision"));
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(u8"Время вычисления суммы сил и ньютоновской физики."_cpp17);

		ImGui::BulletText(u8"Синхр. графики: %.2f мс"_cpp17, bench.at("sync"));
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(u8"Время на загрузку данных о клетках в видеопамять\nи время ожидания доступа к буферу видеопамяти."_cpp17);

		// Дополнительные счетчики эффективности
		ImGui::BulletText(u8"Проверок/клетка: %.2f"_cpp17, bench.at("gapcc"));
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(u8"Сколько клетка в среднем делает проверок\nколлизии другими сущностями. (проверки ведутся парами)"_cpp17);

		ImGui::BulletText(u8"Коллизий/клетка: %.2f"_cpp17, bench.at("avr_c") * 2.);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(u8"Сколько в среднем столкновений имеют клетки."_cpp17);

		gui_s.max_collision_list = std::max<double>(gui_s.max_collision_list, bench.at("gapcc") * 100000.);
		ImGui::BulletText(u8"Использование ОЗУ миром: %.1f Мб"_cpp17, bench.at("mem"));
	}

	ImGui::Text(u8"Целевой UPS:"_cpp17);
	if (ImGui::SliderFloat("##ups_set", &gui_s.ups_world_set, 
		gui_s.expendet_ups ? 0.5f : 4.f, gui_s.expendet_ups ? 10000.f : 1000.f,
		"%.1f", ImGuiSliderFlags_Logarithmic)) {
		map_command_t c;
		if (!gui_s.no_update_flag)
			wkv_push_commands.emplace_back("ups", gui_s.ups_world_set);
	}
	ImGui::SameLine();
	ImGui::Checkbox("##exp_ups", &gui_s.expendet_ups);
}

// Информация о выделенной клетке
void Context::gui_focus_cell_info_m() {
	pbo.active = true;
	ImGui::TextDisabled(u8"Выберите клетку в мире (ЛКМ)"_cpp17);
	const uint32_t c_id = pbo.last_sample[0];
	if (c_id == uint32_t(-1))
		return;

	const auto& cells_graphic = world.last_capture()->cells;
	if (c_id >= cells_graphic.size())
		return;

	const auto& cell_graphic = cells_graphic[c_id];

	ImGui::Separator();

	// Создаем таблицу: 1 столбец для названия, 4 для компонентов вектора
	// Используем флаг SizingFixedFit, чтобы колонки не "гуляли"
	if (ImGui::BeginTable("CellData", 5, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {

		// Заголовок таблицы
		ImGui::TableSetupColumn(u8"Параметр"_cpp17, ImGuiTableColumnFlags_WidthFixed, 100.f);
		ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed, 100.f);
		ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed, 100.f);
		ImGui::TableHeadersRow();

		// Строка 1: Позиция (используем position[0] и [1])
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted(u8"Позиция"_cpp17);
		ImGui::TableNextColumn(); ImGui::Text("%.1f", cell_graphic.position[0]);
		ImGui::TableNextColumn(); ImGui::Text("%.1f", cell_graphic.position[1]);

		// Строка 2: Скорость (используем position[2] и [3] согласно вашему соглашению)
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted(u8"Скорость"_cpp17);
		ImGui::TableNextColumn(); ImGui::Text("%.1f", cell_graphic.position[2]);
		ImGui::TableNextColumn(); ImGui::Text("%.1f", cell_graphic.position[3]);

		// Строка 3: Сила (используем position[2] и [3] согласно вашему соглашению)
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted(u8"Сила"_cpp17);
		ImGui::TableNextColumn(); ImGui::Text("%.1f", cell_graphic.meta[0]);
		ImGui::TableNextColumn(); ImGui::Text("%.1f", cell_graphic.meta[1]);

		// Строка 4: Анимация и Радиус (используем position[2] и [3] согласно вашему соглашению)
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); ImGui::TextUnformatted(u8"Сила"_cpp17);
		ImGui::TableNextColumn(); ImGui::Text("%.1f", cell_graphic.meta[0]);
		ImGui::TableNextColumn(); ImGui::Text("%.1f", cell_graphic.meta[1]);

		// Строка 3: Цвет
		ImGui::TableNextRow();
		ImGui::TableNextColumn(); 
		ImGui::TextUnformatted(u8"Цвет"_cpp17);
		ImGui::SameLine();
		const ImVec4 col = ImVec4(cell_graphic.color[0], cell_graphic.color[1], cell_graphic.color[2], cell_graphic.color[3]);
		ImGui::ColorButton("##cell_color", col, ImGuiColorEditFlags_NoTooltip, ImVec2(18, 18));
		for (int i = 0; i < 4; ++i) {
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", cell_graphic.color[i]);
		}

		ImGui::EndTable();
	}


	// TODO: Вывод данных из world_state для конкретного ID
}

// Создание новой клетки
void Context::gui_creation_m() {
	auto& t_cell = gui_s.template_cell;
	auto& ui = gui_s.ui;
	ImDrawList* draw_list = ImGui::GetForegroundDrawList(); // Для оверлеев

	ImGui::PushItemWidth(200.0f);

	// 1. Выбор типа
	if (ImGui::Combo(u8"Тип клетки"_cpp17, &ui.current_type_idx, cell_type_names, IM_ARRAYSIZE(cell_type_names))) {
		t_cell.type = static_cast<cell_t::type_t>(ui.current_type_idx);
	}

	// 2. Радиус и Масса
	// Используем ui.radius напрямую. При изменении — пересчитываем всё остальное.
	if (ImGui::DragFloat(u8"Радиус (м)"_cpp17, &ui.radius, 0.005f, 0.1f, 0.5f, "%.3f")) {
		t_cell.radius = ui.radius;
		t_cell.weight = t_cell.radius * t_cell.radius * 4.0;
		ui.weight = static_cast<float>(t_cell.weight);
	}

	if (ImGui::DragFloat(u8"Масса (кг)"_cpp17, &ui.weight, 0.01f, 0.04f, 1.0f, "%.3f")) {
		t_cell.weight = ui.weight;
		t_cell.radius = std::sqrt(t_cell.weight / 4.0);
		ui.radius = static_cast<float>(t_cell.radius);
	}

	// 3. Цвет — передаем указатель на внутренние данные fvec4
	ImGui::ColorEdit4(u8"Цвет"_cpp17, t_cell.color.data(), ImGuiColorEditFlags_NoInputs);

	ImGui::PopItemWidth();

	// 4. Вектор скорости (Drag Button)
	ImGui::Text(u8"Начальная скорость: %.2f, %.2f"_cpp17, t_cell.velocity[0], t_cell.velocity[1]);
	ImGui::SameLine();

	// Кнопка настройки скорости
	ImGui::Button(u8"..."_cpp17);
	ImVec2 btn_pos = ImGui::GetItemRectMin();
	ImVec2 btn_size = ImGui::GetItemRectSize();
	ImVec2 btn_center = ImVec2(btn_pos.x + btn_size.x * 0.5f, btn_pos.y + btn_size.y * 0.5f);

	if (ImGui::IsItemActive()) {
		ui.is_dragging_speed = true;
		ImVec2 m_delta_px = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);

		// Сброс скорости по ПКМ во время зажатия ЛКМ на кнопке
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			t_cell.velocity = vec2(0.0);
		}
		else {
			// Рассчитываем новую скорость на основе смещения мыши
			vec2 m_delta_norm = vec2(m_delta_px.x, m_delta_px.y) / win_size;
			vec2 world_scale = camera.screen_to_world(vec2(1.0, 1.0)) - camera.screen_to_world(vec2(0.0, 0.0));

			// Устанавливаем скорость пропорционально "вытягиванию" кнопки (множитель 10.0 для чувствительности)
			t_cell.velocity = (m_delta_norm * world_scale) * 10.0;
		}

		// Визуализация "натяжения" прямо от кнопки меню
		draw_list->AddLine(btn_center, ImGui::GetIO().MousePos, 0xFF00FFFF, 2.0f);
		ImGui::SetTooltip(u8"Скорость: %.2f\nПКМ для сброса"_cpp17, osl::length(t_cell.velocity));
	}
	else {
		ui.is_dragging_speed = false;
	}

	// Сброс по двойному клику (альтернативный вариант)
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		t_cell.velocity = vec2(0.0);
	}

	ImGui::Separator();

	// 5. Размещение клетки в мире
	if (!gui_s.is_placing_cell) {
		if (ImGui::Button(u8"СОЗДАТЬ (Разместить)"_cpp17, ImVec2(-1, 0))) {
			gui_s.is_placing_cell = true;
		}
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
		if (ImGui::Button(u8"ОТМЕНА (ПКМ)"_cpp17, ImVec2(-1, 0))) {
			gui_s.is_placing_cell = false;
		}
		ImGui::PopStyleColor();

		ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) gui_s.is_placing_cell = false;

		if (!io.WantCaptureMouse) {
			vec2 m_norm = vec2(io.MousePos.x, io.MousePos.y) / win_size;
			vec2 world_pos = camera.screen_to_world(m_norm);
			vec2 screen_center_v = camera.world_to_screen(world_pos) * win_size;
			ImVec2 screen_center = ImVec2(screen_center_v[0], screen_center_v[1]);

			// Отрисовка превью клетки
			vec2 screen_edge = camera.world_to_screen(world_pos + vec2(t_cell.radius, 0)) * win_size;
			float radius_px = osl::length(screen_edge - screen_center_v);
			ImU32 cell_col = ImGui::ColorConvertFloat4ToU32(*(ImVec4*)t_cell.color.data());

			draw_list->AddCircleFilled(screen_center, radius_px, (cell_col & 0x00FFFFFF) | 0x60000000);
			draw_list->AddCircle(screen_center, radius_px, 0xFFFFFFFF, 0, 1.0f);

			// Отрисовка вектора скорости (от центра будущей клетки)
			if (osl::length(t_cell.velocity) > 0.001) {
				// Конец вектора в мировых координатах
				vec2 world_vel_end = world_pos + t_cell.velocity;
				vec2 screen_vel_end_v = camera.world_to_screen(world_vel_end) * win_size;
				ImVec2 screen_vel_end = ImVec2(screen_vel_end_v[0], screen_vel_end_v[1]);

				// Рисуем линию и стрелочку
				draw_list->AddLine(screen_center, screen_vel_end, 0xFF0000FF, 2.5f);
				draw_list->AddCircleFilled(screen_vel_end, 3.0f, 0xFF0000FF); // Наконечник
			}

			// Размещение (клик или шифт+клик) 
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || (ImGui::IsMouseDown(ImGuiMouseButton_Left) && io.KeyShift)) {
				t_cell.pos = world_pos;
				wkv_push_commands.emplace_back("create cell", t_cell); // [cite: 1, 26, 29]
				if (!io.KeyShift) gui_s.is_placing_cell = false;
			}
		}
	}
}