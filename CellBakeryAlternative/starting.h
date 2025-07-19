#pragma once

// функция загружает файлы с указанных путей с проверкой на BOM и пытается скомпилировать шейдер
bool loadShaderFromFiles(shad::Shader &shader, std::string vert_path, std::string frag_path, std::string geom_path = "") {
	std::string sourseV = osl::loadFileToStringWithoutBOM(vert_path);
	std::string sourseF = osl::loadFileToStringWithoutBOM(frag_path);
	std::string sourseG;
	if (!geom_path.empty()) // геометрический шейдер не обязателен, игнорируем если путь к нему не указан
		sourseG = osl::loadFileToStringWithoutBOM(geom_path);
	if (!sourseV.empty() && !sourseF.empty() && (!sourseG.empty() || geom_path.empty())) {
		if (geom_path.empty()) {
			if (shader.compile(sourseV.c_str(), sourseF.c_str()))
				return false;
		}
		else {
			if (shader.compile(sourseV.c_str(), sourseF.c_str(), sourseG.c_str()))
				return false;
		}
	}
	else {
		std::cout << u8"Графический шейдер \"" << shader.name << u8"\" не был загружен\n";
		return false;
	}
	return true;
}

bool loadShaderFile(fs::path path) {

	std::regex include_regex(R"(#include\s+\"([^\"]+)\")");
}

int Context::run() {

	// шейдеры графики
	{
		petriShader.name = std::string("petriShader");
		loadShaderFromFiles(petriShader, "Shaders/Graphics/petri.vert", "Shaders/Graphics/petri.frag");

		cellsShader.name = std::string("cellsShader");
		loadShaderFromFiles(cellsShader, "Shaders/Graphics/cells.vert", "Shaders/Graphics/cells.frag", "Shaders/Graphics/cells.geom");
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