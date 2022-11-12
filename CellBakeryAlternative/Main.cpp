#include "include.h"

using namespace osl;

class key_trigger {
public:
	void push (bool t) {
		if (t) {
			is_click = !is_press;
			is_press = true;
		}
		else {
			is_release = is_press;
			is_click = false, is_press = false;
		}
	}
	bool is_press = 0, is_click = 0, is_release = 0;
private:
};

class Context {
public:
	int run();
	Context(GLFWwindow *w) : window(w), frameTime(60) {};
private:
	int err = 0;
	Randomaizer RAND;
	int Xd = 800, Yd = 600;

	int Vsync = 0, VsyncNow = Vsync;

	GLFWwindow *window;
	shad::VoidMesh voidMesh;
	shad::Shader cellsShader;

	shad::SimpleMesh fullScreenMesh;
	shad::Shader petriShader;

	uint32_t count_of_frames = -1;
	std::ostringstream buff; // текст буффер
	ch_tp frame_time_point[2]; // две переменные времени
	fastLinearFilter frameTime;

	vec2 mPos, dmPos1, dmPos2;
	float scroll = 0.;

	vec2 mPos_to_wPos(vec2 mPos, frac mst, vec2 wPos) {
		mPos = (vec2(mPos[0] - Xd / 2., Yd - mPos[1] - Yd / 2.)) / frac(Yd) * mst;
		mPos = mPos + wPos;
		return mPos;
	}

	vec2 mVec_to_wVec(vec2 mPos, frac mst) {
		mPos = (vec2(mPos[0], mPos[1])) / frac(Yd) * mst;
		return mPos;
	}
};

int Context::run() {
	if (err != 0)
		return err;
	frame_time_point[0] = chGetTime();
	frame_time_point[1] = frame_time_point[0];
	RAND.ini();
	//glfwSetKeyCallback(window, )

	{
		std::vector<float> m = {-1., -1., -1., 1., 1., -1., 1., 1.};
		fullScreenMesh.loadFrom(&m[0], m.size());
	}

	{
		petriShader.name = std::string("petriShader");
		std::ifstream inpf;
		inpf.open("Shaders/petri.vert");
		std::string sourseV {std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>()};
		inpf.close();
		inpf.open("Shaders/petri.frag", std::ios_base::in);
		std::string sourseF {std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>()};
		inpf.close();
		petriShader.compile(sourseV.c_str(), sourseF.c_str());
	}

	{
		cellsShader.name = std::string("cellsShader");
		std::ifstream inpf;
		inpf.open("Shaders/cells.vert");
		std::string sourseV {std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>()};
		inpf.close();
		inpf.open("Shaders/cells.frag", std::ios_base::in);
		std::string sourseF {std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>()};
		inpf.close();
		inpf.open("Shaders/cells.geom", std::ios_base::in);
		std::string sourseG {std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>()};
		inpf.close();
		cellsShader.compile(sourseV.c_str(), sourseF.c_str(), sourseG.c_str());
	}

	// Создание мира
	///==============================
	WorldCS world;
	world.setup.Dp = 5.;
	int Counter1 = 0;

	world.setup.name = "Test world";
	std::thread TWT(&WorldCS::RenderWorld, &world);
	TWT.detach();
	while (!world.control.out_ready) std::this_thread::sleep_for(std::chrono::milliseconds(1));


	GLuint light_map = 0; // текстура карты мира для яркости света
	{
		glGenTextures(1, &light_map);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, light_map);

		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // без интерполяции
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // выдаёт цвет ближайшего пикселя в пределах текстуры при вылете за границы

		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, world.setup.out_mapSize, world.setup.out_mapSize, 0, GL_RED, GL_UNSIGNED_BYTE, world.light_map);
	}
	GLuint collision_map = 0; // текстура карты индексов коллизионных областей мира
	{
		glGenTextures(1, &collision_map);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, collision_map);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // без интерполяции

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // выдаёт цвет ближайшего пикселя в пределах текстуры при вылете за границы

		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, world.setup.out_mapSize, world.setup.out_mapSize, 0, GL_RED_INTEGER, GL_INT, world.collision_map);
	}
	GLuint collision_data = 0; // информация о коллизиях
	{
		glGenTextures(1, &collision_data);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, collision_data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // без интерполяции

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // выдаёт цвет ближайшего пикселя в пределах текстуры при вылете за границы

		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, 4096, world.setup.out_maxСollisionData / 4096, 0, GL_RED_INTEGER, GL_INT, &world.collision_data[0]);
	}
	GLuint cells_pos = 0; // позиции клеток
	{
		glGenTextures(1, &cells_pos);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, cells_pos);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		float clr[4] = {0, 0, 0, 0};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clr); // свойство текстуры за пределами выдавать чёрный прозрачный цвет

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4096, world.setup.set_max_ec / 4096, 0, GL_RGBA, GL_FLOAT, world.buffFloatPosCells);
	}
	GLuint cells_meta = 0; // метадата клеток
	{
		glGenTextures(1, &cells_meta);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, cells_meta);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		float clr[4] = {0, 0, 0, 0};
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clr); // свойство текстуры за пределами выдавать чёрный прозрачный цвет

		glBindTexture(GL_TEXTURE_2D, cells_meta); // также заполняем стартовыми данными для цикла
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4096, world.setup.set_max_ec / 4096, 0, GL_RGBA, GL_UNSIGNED_BYTE, world.buffByteMetaCells);
	}

	
	glEnable(GL_MULTISAMPLE);

	// Цикл графики
	///\/\/\/\/\/\/\/\///
	glfwSwapInterval(Vsync);
	while (!glfwWindowShouldClose(window)) {
		// Работа с glfw3
		{	///============================================================///
			if (VsyncNow != Vsync) {
				glfwSwapInterval(Vsync);
				VsyncNow = Vsync;
			}

			// Получаем эвенты
			glfwPollEvents();

			// Настраиваем камеру под разрешение окна
			glfwGetFramebufferSize(window, &Xd, &Yd);
			glViewport(0, 0, Xd, Yd);
		}	///============================================================///

		glClearColor(0.8f, 0.8f, 0.8f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


		int boost = (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS);
		int test = (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS);
		int gmesh = (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS);
		{
			// управление камерой
			{	///==============================
				// движение клавиатурой
				{	///==============================
					if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
						world.view.Pos[0] -= (frameTime.get() / 1000.) * world.view.mst;
					if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
						world.view.Pos[0] += (frameTime.get() / 1000.) * world.view.mst;
					if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
						world.view.Pos[1] -= (frameTime.get() / 1000.) * world.view.mst;
					if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
						world.view.Pos[1] += (frameTime.get() / 1000.) * world.view.mst;
				}	///==============================

				
				// масштабирование в указатель и движение
				{	///==============================
					// получение эвента колёсика мыши
					//float scroll = ImGui::GetIO().MouseWheel;
					if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
						scroll -= 0.05;
					if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
						scroll += 0.03;

					dmPos2 = dmPos1; // движение
					glfwGetCursorPos(window, &dmPos1[0], &dmPos1[1]); dmPos1[0] = -dmPos1[0];
					if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
						world.view.Pos -= mVec_to_wVec(dmPos2 - dmPos1, world.view.mst);
					}
					mPos = mPos_to_wPos(vec2(-dmPos1[0], dmPos1[1]), world.view.mst, world.view.Pos);

					/// компенсация движения
					// dmPos3 *= vec2(std::min(vec2(dmPos2.x - dmPos1.x).length() * 0.05, 2.), std::min(vec2(dmPos2.y - dmPos1.y).length() * 0.05, 2.)) * 0.3;

					while (scroll > 0.01) { // приближение
						scroll -= 0.1;
						if (scroll < 0.) scroll = 0.;
						vec2 mnPos = mPos_to_wPos(vec2(0, 0), world.view.mst, world.view.Pos);
						vec2 mxPos = mPos_to_wPos(vec2(Xd, Yd), world.view.mst, world.view.Pos);
						mnPos = mix(mnPos, mPos, +0.012);
						mxPos = mix(mxPos, mPos, +0.012);
						world.view.Pos = mix(mnPos, mxPos, 0.5);
						world.view.mst = mnPos[1] - mxPos[1];

						world.view.mst = std::max(world.view.mst, 0.00001 * world.setup.Dp);
					}
					while (scroll < -0.01) { // отдаление 
						scroll += 0.1;
						vec2 mnPos = mPos_to_wPos(vec2(0, 0), world.view.mst, world.view.Pos);
						vec2 mxPos = mPos_to_wPos(vec2(Xd, Yd), world.view.mst, world.view.Pos);
						mnPos = mix(mnPos, mPos, -0.012);
						mxPos = mix(mxPos, mPos, -0.012);
						world.view.Pos = mix(mnPos, mxPos, 0.5);
						world.view.mst = mnPos[1] - mxPos[1];

						world.view.mst = std::min(world.view.mst, world.setup.Dp * 2.);
					}
				}	///==============================
				
			}	///==============================

			vec2 mnPos = mPos_to_wPos(vec2(0, 0), world.view.mst, world.view.Pos);
			vec2 mxPos = mPos_to_wPos(vec2(Xd, Yd), world.view.mst, world.view.Pos);

			// чашка петри
			if (1) {
				glUseProgram(petriShader.glID);
				glUniform4f(glGetUniformLocation(petriShader.glID, "ViewWorld"), mnPos[0], mxPos[1], mxPos[0], mnPos[1]);
				glUniform2i(glGetUniformLocation(petriShader.glID, "WinSize"), Xd, Yd);
				glUniform1i(glGetUniformLocation(petriShader.glID, "mapSize"), world.setup.out_mapSize);
				glUniform2f(glGetUniformLocation(petriShader.glID, "cPos"), world.view.Pos[0], world.view.Pos[1]);
				glUniform2f(glGetUniformLocation(petriShader.glID, "mPos"), mPos[0], mPos[1]);
				glUniform1f(glGetUniformLocation(petriShader.glID, "mst"), world.view.mst);
				glUniform1f(glGetUniformLocation(petriShader.glID, "Dp"), world.setup.Dp);
				glUniform1f(glGetUniformLocation(petriShader.glID, "Ac"), world.setup.Ac);
				//glUniform1i(glGetUniformLocation(petriShader.glID, "MSAA"), MSAAuniform[MSAA]);

				glBindTexture(GL_TEXTURE_2D, light_map); // обновление данных карта света
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, world.setup.out_mapSize, world.setup.out_mapSize, GL_RED, GL_UNSIGNED_BYTE, world.light_map);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, light_map);
				glUniform1i(glGetUniformLocation(petriShader.glID, "Light"), 0);

				glBindVertexArray(fullScreenMesh.VAO);
				for (int i = -1; i < 50 * boost; i++) {
					glDrawArrays(GL_TRIANGLE_STRIP, 0, fullScreenMesh.size);
				}
			}
			// клетки
			if (1) {
				
				glUseProgram(cellsShader.glID);
				glUniform4f(glGetUniformLocation(cellsShader.glID, "ViewWorld"), mnPos[0], mxPos[1], mxPos[0], mnPos[1]);
				glUniform2i(glGetUniformLocation(cellsShader.glID, "WinSize"), Xd, Yd);
				glUniform1i(glGetUniformLocation(cellsShader.glID, "mapSize"), world.setup.out_mapSize);
				glUniform2f(glGetUniformLocation(cellsShader.glID, "cPos"), world.view.Pos[0], world.view.Pos[1]);
				glUniform2f(glGetUniformLocation(cellsShader.glID, "mPos"), mPos[0], mPos[1]);
				glUniform1f(glGetUniformLocation(cellsShader.glID, "mst"), world.view.mst);
				glUniform1f(glGetUniformLocation(cellsShader.glID, "Dp"), world.setup.Dp);
				glUniform1f(glGetUniformLocation(cellsShader.glID, "Ac"), world.setup.Ac);
				glUniform1f(glGetUniformLocation(cellsShader.glID, "fTime"), count_of_frames / 60.);
				glUniform1i(glGetUniformLocation(cellsShader.glID, "GM"), gmesh);
				
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, collision_map);
				glUniform1i(glGetUniformLocation(cellsShader.glID, "Collision"), 1);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, collision_data);
				glUniform1i(glGetUniformLocation(cellsShader.glID, "CollisionData"), 2);

				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, cells_pos);
				glUniform1i(glGetUniformLocation(cellsShader.glID, "CellsPos"), 3);

				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, cells_meta);
				glUniform1i(glGetUniformLocation(cellsShader.glID, "CellsMeta"), 4);
				

				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBindVertexArray(voidMesh.VAO);
				for (int i = -1; i < 50 * boost; i++) {
					glDrawArraysInstanced(GL_POINTS, 0, voidMesh.size, world.setup.set_max_ec); //world.setup.set_max_ec
				}
				glDisable(GL_BLEND);
			}
		}
		
		count_of_frames++;
		buff.str("");
		bool swapBit = count_of_frames % 2;
		frame_time_point[swapBit] = chGetTime();
		frameTime.push(chDurationMillis(frame_time_point[swapBit], frame_time_point[!swapBit]));
		if (count_of_frames % 100000 == 0)
			frameTime.resum();

		buff << "fps - " << std::fixed << std::setprecision(1) << 1000. / frameTime.get();
		buff << "  ms - " << std::fixed << std::setprecision(2) << frameTime.get();
		buff << "         " << std::fixed << std::setprecision(1) << count_of_frames / 60.;
		glfwSetWindowTitle(window, buff.str().c_str());

		glfwSwapBuffers(window);
	}

	world.control.shutdown();
	while (!world.control.out_safe_shutdown)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	return 1;
}




int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_SAMPLES, 16);

	auto window = glfwCreateWindow(800, 600, "CellBakery Alternative", NULL, NULL);
	if (window == NULL) return -1;

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

	Context c(window);
	int r = c.run();

	glfwDestroyWindow(window);
	glfwTerminate();
	return r;
}



