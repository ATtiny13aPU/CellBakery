#pragma once


// функция загружает файлы с указанных путей с проверкой на BOM и пытается скомпилировать шейдер
bool loadShaderFromFiles(shad::Shader &shader, std::string vert_path, std::string frag_path, std::string geom_path = "") {
	std::string sourseV, sourseF, sourseG;
	if (!osl::loadShaderFile(vert_path, sourseV)) {
		std::cout << u8"Файл вершинного шейдера \"" << sourseV << u8"\" не был загружен\n";
		std::cout << u8"Графический шейдер \"" << shader.name << u8"\" не был загружен\n";
		return false;
	}
	if (!osl::loadShaderFile(frag_path, sourseF)) {
		std::cout << u8"Файл фрагментного шейдера \"" << sourseF << u8"\" не был загружен\n";
		std::cout << u8"Графический шейдер \"" << shader.name << u8"\" не был загружен\n";
		return false;
	}

	if (!geom_path.empty()) // геометрический шейдер не обязателен, игнорируем если путь к нему не указан
		if (!osl::loadShaderFile(geom_path, sourseG)) {
			std::cout << u8"Файл геометрического шейдера \"" << sourseG << u8"\" не был загружен\n";
			std::cout << u8"Графический шейдер \"" << shader.name << u8"\" не был загружен\n";
			return false;
		}

	if (!sourseV.empty() && !sourseF.empty() && (!sourseG.empty() || geom_path.empty())) {
		if (geom_path.empty()) {
			if (shader.compile(sourseV.c_str(), sourseF.c_str())) {
				std::cout << u8"Графический шейдер \"" << shader.name << u8"\" не скомпилирован\n";
				return false;
			}
		}
		else {
			if (shader.compile(sourseV.c_str(), sourseF.c_str(), sourseG.c_str())) {
				std::cout << u8"Графический шейдер \"" << shader.name << u8"\" не скомпилирован\n";
				return false;
			}
		}
	}
	else {
		std::cout << u8"Графический шейдер \"" << shader.name << u8"\" не был загружен\n";
		return false;
	}
	return true;
}

int Context::run() {

	// шейдеры графики
	{
		petriShader.name = std::string("petriShader");
		loadShaderFromFiles(petriShader, "Shaders/petri.vert", "Shaders/petri.frag");

		cellsShader.name = std::string("cellsShader");
		loadShaderFromFiles(cellsShader, "Shaders/cells.vert", "Shaders/cells.frag", "Shaders/cells.geom");
	}


	// Цикл графики
	glfwSwapInterval(Vsync);

	while (!glfwWindowShouldClose(window)) {

		control();

		compute();

		graphics();

		gui();

		glfwSwapBuffers(window);
	}

	return 1;
}