#include "include.h"

using namespace osl;

class key_trigger {
public:
	void push(bool t) {
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
	Context(GLFWwindow *w) : window(w), frameTime(60), frameTimeLowLatency(3) {};
private:
	int err = 0;
	Randomaizer RAND;
	int Xd = 800, Yd = 600;

	int Vsync = 1, VsyncNow = Vsync;

	GLFWwindow *window;

	shad::VoidMesh voidMesh;
	shad::SimpleMesh fullScreenMesh;

	shad::ComputeShader randomizer1ComputeShader; // шейдер генерации ключей рандомайзеров
	shad::ComputeShader randomizer2ComputeShader; // шейдер генерации ключей рандомайзеров
	shad::ComputeShader resetCellsComputeShader; // стартовая загрузка данных в ssbo_cells
	shad::ComputeShader resetGridComputeShader; // стартовая загрузка данных в ssbo_chunk
	shad::ComputeShader lightComputeShader; // вычисление brightness в ssbo_chunk
	shad::ComputeShader cellsPhysics_listComputeShader; // генерация связного списка
	shad::ComputeShader cellsPhysics_processForcesComputeShader; // нахождение и обработка коллизий (суммирование сил)
	shad::ComputeShader cellsPhysics_processPositionComputeShader; // пересчёт скорости и позиции, а также затирание ID в чанке, пересчёт позиции чанка клетки

	shad::Shader cellsShader;
	shad::Shader petriShader;

	shad::SSBO CellsSSBO;
	shad::SSBO GridSSBO;
	shad::SSBO RandSSBO;

	uint32_t count_of_frames = 0;
	uint32_t count_of_updates = 0;
	std::ostringstream buff; // текст буффер
	ch_tp frame_time_point[2]; // две переменные времени
	fastLinearFilter frameTime;
	fastLinearFilter frameTimeLowLatency;

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

	struct Cell_ssboBlock {
		uvec2 ipos;
		fvec2 pos;
		float radius;
		float angle;
		fvec4 color_rgb;
		fvec4 color_hsv;
		int type_id;
		int linked_list;
		int chunk_id;
		int is_first;
		float weight;
		fvec4 velocity;
		fvec4 force;
	};

	struct Chunk_ssboBlock {
		int32_t first_list_ID;
		int32_t linked_list;
		float brightness;
	};
};

void glGetIntegervCout(int l) {
	int size = 0;
	glGetIntegerv(l, &size);
	std::cout << size << '\n';
}

void glGetIntegeri_vCout(int l) {
	int size = 0;
	glGetIntegeri_v(l, 0, &size);
	std::cout << size << ' ';
	glGetIntegeri_v(l, 1, &size);
	std::cout << size << ' ';
	glGetIntegeri_v(l, 2, &size);
	std::cout << size << '\n';
}

int Context::run() {
	if (err != 0)
		return err;
	frame_time_point[0] = chGetTime();
	frame_time_point[1] = frame_time_point[0];
	RAND.ini();
	//glfwSetKeyCallback(window, )

	{
		std::vector<float> m = { -1., -1., -1., 1., 1., -1., 1., 1. };
		fullScreenMesh.loadFrom(&m[0], m.size());
	}

	// шейдеры симуляции

	{
		randomizer1ComputeShader.name = std::string("randomizer1ComputeShader");
		std::ifstream inpf;
		inpf.open("Shaders/Computing/randomizer1.comp");
		std::string sourseC{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		randomizer1ComputeShader.compile(sourseC.c_str());
	}

	{
		randomizer2ComputeShader.name = std::string("randomizer2ComputeShader");
		std::ifstream inpf;
		inpf.open("Shaders/Computing/randomizer2.comp");
		std::string sourseC{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		randomizer2ComputeShader.compile(sourseC.c_str());
	}

	{
		resetGridComputeShader.name = std::string("resetGridComputeShader");
		std::ifstream inpf;
		inpf.open("Shaders/Computing/resetGrid.comp");
		std::string sourseC{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		resetGridComputeShader.compile(sourseC.c_str());
	}

	{
		resetCellsComputeShader.name = std::string("resetCellsComputeShader");
		std::ifstream inpf;
		inpf.open("Shaders/Computing/resetCells.comp");
		std::string sourseC{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		resetCellsComputeShader.compile(sourseC.c_str());
	}

	{
		lightComputeShader.name = std::string("lightComputeShader");
		std::ifstream inpf;
		inpf.open("Shaders/Computing/light.comp");
		std::string sourseC{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		lightComputeShader.compile(sourseC.c_str());
	}

	{
		cellsPhysics_listComputeShader.name = std::string("cellsPhysics_listComputeShader");
		std::ifstream inpf;
		inpf.open("Shaders/Computing/cellsPhysics_list.comp");
		std::string sourseC{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		cellsPhysics_listComputeShader.compile(sourseC.c_str());
	}

	{
		cellsPhysics_processForcesComputeShader.name = std::string("cellsPhysics_processForceComputeShader");
		std::ifstream inpf;
		inpf.open("Shaders/Computing/cellsPhysics_processForces.comp");
		std::string sourseC{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		cellsPhysics_processForcesComputeShader.compile(sourseC.c_str());
	}

	{
		cellsPhysics_processPositionComputeShader.name = std::string("cellsPhysics_processPositionComputeShader");
		std::ifstream inpf;
		inpf.open("Shaders/Computing/cellsPhysics_processPositions.comp");
		std::string sourseC{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		cellsPhysics_processPositionComputeShader.compile(sourseC.c_str());
	}

	// шейдеры графики

	{
		petriShader.name = std::string("petriShader");
		std::ifstream inpf;
		inpf.open("Shaders/Graphics/petri.vert");
		std::string sourseV{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		inpf.open("Shaders/Graphics/petri.frag", std::ios_base::in);
		std::string sourseF{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		petriShader.compile(sourseV.c_str(), sourseF.c_str());
	}

	{
		cellsShader.name = std::string("cellsShader");
		std::ifstream inpf;
		inpf.open("Shaders/Graphics/cells.vert");
		std::string sourseV{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		inpf.open("Shaders/Graphics/cells.frag", std::ios_base::in);
		std::string sourseF{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		inpf.open("Shaders/Graphics/cells.geom", std::ios_base::in);
		std::string sourseG{ std::istreambuf_iterator<char>(inpf), std::istreambuf_iterator<char>() };
		inpf.close();
		cellsShader.compile(sourseV.c_str(), sourseF.c_str(), sourseG.c_str());
	}

	glGetIntegervCout(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
	glGetIntegeri_vCout(GL_MAX_COMPUTE_WORK_GROUP_COUNT);
	glGetIntegeri_vCout(GL_MAX_COMPUTE_WORK_GROUP_SIZE);
	glGetIntegervCout(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
//	glGetIntegerv();
//	std::cout << size;

	// Создание мира (ВНИМАНИЕ! Данные мира больше не используются, все вычисления переносятся на GPU)
	///==============================
	WorldCS world;
	//world.Dp = 2. / 0.03;
	//world.mec = 1024;
	world.Dp = 2. / 0.03;
	world.mec = 1024;
	std::cout << "enter Dp = ";
	std::cin >> world.Dp;
	world.Dp /= 0.03;
	std::cout << "\nenter max enable cells = ";
	std::cin >> world.mec;
	world.iniWorld();



	glDisable(GL_MULTISAMPLE);

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

	//	glClearColor(0.8f, 0.8f, 0.8f, 1.f);
	//	glClear(GL_COLOR_BUFFER_BIT);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// Управление камерой мышью и клавой
		static key_trigger boost, test, gmesh, msaa;
		vec2 mnPos, mxPos;
		{
			boost.push(glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS);
			test.push(glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS);
			gmesh.push(glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS);
			msaa.push(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS);
			// управление камерой
			{	///==============================
				// движение клавиатурой
				{	///==============================
					if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
						world.view.Pos[0] -= (frameTimeLowLatency.get() / 1000.) * world.view.mst;
					if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
						world.view.Pos[0] += (frameTimeLowLatency.get() / 1000.) * world.view.mst;
					if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
						world.view.Pos[1] -= (frameTimeLowLatency.get() / 1000.) * world.view.mst;
					if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
						world.view.Pos[1] += (frameTimeLowLatency.get() / 1000.) * world.view.mst;
				}	///==============================


				// масштабирование в указатель и движение
				{	///==============================
					// получение эвента колёсика мыши
					//float scroll = ImGui::GetIO().MouseWheel;
					if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
						scroll -= 10. * (frameTimeLowLatency.get() / 1000.);
					if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
						scroll += 10. * (frameTimeLowLatency.get() / 1000.);

					dmPos2 = dmPos1; // движение
					glfwGetCursorPos(window, &dmPos1[0], &dmPos1[1]); dmPos1[0] = -dmPos1[0];
					if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
						world.view.Pos -= mVec_to_wVec(dmPos2 - dmPos1, world.view.mst);
					}
					mPos = mPos_to_wPos(vec2(-dmPos1[0], dmPos1[1]), world.view.mst, world.view.Pos);

					/// компенсация движения
					// dmPos3 *= vec2(std::min(vec2(dmPos2.x - dmPos1.x).length() * 0.05, 2.), std::min(vec2(dmPos2.y - dmPos1.y).length() * 0.05, 2.)) * 0.3;
					const frac pk = 0.2;

					while (scroll > 0.01) { // приближение
						scroll -= 0.1 * pk;
						if (scroll < 0.) scroll = 0.;
						vec2 mnPos = mPos_to_wPos(vec2(0, 0), world.view.mst, world.view.Pos);
						vec2 mxPos = mPos_to_wPos(vec2(Xd, Yd), world.view.mst, world.view.Pos);
						mnPos = mix(mnPos, mPos, +0.012 * pk);
						mxPos = mix(mxPos, mPos, +0.012 * pk);
						world.view.Pos = mix(mnPos, mxPos, 0.5);
						world.view.mst = mnPos[1] - mxPos[1];

						world.view.mst = std::max(world.view.mst, 0.00001 * world.Dp);
					}
					while (scroll < -0.01) { // отдаление 
						scroll += 0.1 * pk;
						vec2 mnPos = mPos_to_wPos(vec2(0, 0), world.view.mst, world.view.Pos);
						vec2 mxPos = mPos_to_wPos(vec2(Xd, Yd), world.view.mst, world.view.Pos);
						mnPos = mix(mnPos, mPos, -0.012 * pk);
						mxPos = mix(mxPos, mPos, -0.012 * pk);
						world.view.Pos = mix(mnPos, mxPos, 0.5);
						world.view.mst = mnPos[1] - mxPos[1];

						world.view.mst = std::min(world.view.mst, world.Dp * 2.);
					}
				}	///==============================
			}	///==============================

			world.view.Pos = clamp(world.view.Pos, -world.Dp * 0.9, world.Dp * 1.9);

			mnPos = mPos_to_wPos(vec2(0, 0), world.view.mst, world.view.Pos);
			mxPos = mPos_to_wPos(vec2(Xd, Yd), world.view.mst, world.view.Pos);
		}

		if (msaa.is_click)
			glEnable(GL_MULTISAMPLE);
		if (msaa.is_release)
			glDisable(GL_MULTISAMPLE);

		// Вычисления
		for (int upd = 0; upd < 1 * test.is_press || count_of_updates == 0; upd++)
		{
			// Инициализация ssbo буферов
			if (count_of_updates == 0) {
				// Рандомайзер
				std::vector<uint32_t> bu(384 * 4 * 10, 0u);
				RandSSBO.loadFrom(4 * 384 * 4 * 10, &(bu[0]));
				
				RandSSBO.bind(randomizer1ComputeShader.glID, "ssbo_rand");
				RandSSBO.bind(randomizer2ComputeShader.glID, "ssbo_rand");
				RandSSBO.bind(cellsPhysics_processPositionComputeShader.glID, "ssbo_rand");

				// Поле мира
				GridSSBO.setSize((world.Dm + 2) * (world.Dm + 2) * (sizeof(Chunk_ssboBlock)));

				GridSSBO.bind(resetGridComputeShader.glID, "ssbo_grid");
				GridSSBO.bind(lightComputeShader.glID, "ssbo_grid");
				GridSSBO.bind(cellsPhysics_listComputeShader.glID, "ssbo_grid");
				GridSSBO.bind(cellsPhysics_processForcesComputeShader.glID, "ssbo_grid");
				GridSSBO.bind(cellsPhysics_processPositionComputeShader.glID, "ssbo_grid");
				GridSSBO.bind(petriShader.glID, "ssbo_grid");

				// Клетки
				CellsSSBO.setSize(world.mec * sizeof(Cell_ssboBlock));

				CellsSSBO.bind(resetCellsComputeShader.glID, "ssbo_cells");
				CellsSSBO.bind(cellsPhysics_listComputeShader.glID, "ssbo_cells");
				CellsSSBO.bind(cellsPhysics_processForcesComputeShader.glID, "ssbo_cells");
				CellsSSBO.bind(cellsPhysics_processPositionComputeShader.glID, "ssbo_cells");
				CellsSSBO.bind(cellsShader.glID, "ssbo_cells");
			}


			// инициализатор поля
			if (count_of_updates == 0) {
				glUseProgram(resetGridComputeShader.glID);
				glUniform1i(glGetUniformLocation(resetGridComputeShader.glID, "Dm"), world.Dm);
				glDispatchCompute(world.Dm, world.Dm, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}


			// инициализатор клеток
			if (count_of_updates == 0) {
				glUseProgram(resetCellsComputeShader.glID);
				static bool first_call = 1;
				if (first_call) {
					first_call = 0;
					glUniform1f(glGetUniformLocation(resetCellsComputeShader.glID, "Dp"), world.Dp);
					glUniform1i(glGetUniformLocation(resetCellsComputeShader.glID, "Dm"), world.Dm);
				}
		
				glDispatchCompute(16, world.mec / 16, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}

			
			// вычисление света
			if (count_of_updates == 0) {
				glUseProgram(lightComputeShader.glID);
				static bool first_call = 1;
				if (first_call) {
					first_call = 0;
					glUniform1i(glGetUniformLocation(lightComputeShader.glID, "Dm"), world.Dm + 2);
				}

				glDispatchCompute(world.Dm + 2, world.Dm + 2, 1);

				//glMemoryBarrier(GL_ALL_BARRIER_BITS);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			}


			// рандомайзер этап 1
			if (0) {
				glUseProgram(randomizer1ComputeShader.glID);
				glUniform1i(glGetUniformLocation(randomizer1ComputeShader.glID, "uFrame"), count_of_updates);
				glDispatchCompute(8, 16, 1);
			}
			

			// физика клеток: этап построения списков
			if (1) {
					glUseProgram(cellsPhysics_listComputeShader.glID);
					static bool first_call = 1;
					if (first_call) {
						first_call = 0;
						glUniform1i(glGetUniformLocation(cellsPhysics_listComputeShader.glID, "Dm"), world.Dm);
						glUniform1f(glGetUniformLocation(cellsPhysics_listComputeShader.glID, "Dp"), world.Dp);
					}

					glDispatchCompute(8, 8, world.mec / 1024);
					//glDispatchCompute(1, 1, 1);

					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}


			// рандомайзер этап 2
			if (0) {
				glUseProgram(randomizer2ComputeShader.glID);
				glUniform1i(glGetUniformLocation(randomizer2ComputeShader.glID, "uFrame"), count_of_updates + 29u);
				glDispatchCompute(16, 16, 1);
			}


			// физика клеток: этап обработки коллизий через списки
			if (1) {
					glUseProgram(cellsPhysics_processForcesComputeShader.glID);
					static bool first_call = 1;
					if (first_call) {
						first_call = 0;
						glUniform1i(glGetUniformLocation(cellsPhysics_processForcesComputeShader.glID, "Dm"), world.Dm);
						glUniform1f(glGetUniformLocation(cellsPhysics_processForcesComputeShader.glID, "Dp"), world.Dp);
					}

					glDispatchCompute(8, 8, world.mec / 1024);

					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}


			// физика клеток: применение прошлого этапа
			if (1) {
					glUseProgram(cellsPhysics_processPositionComputeShader.glID);
					static bool first_call = 1;
					if (first_call) {
						first_call = 0;
						glUniform1i(glGetUniformLocation(cellsPhysics_processPositionComputeShader.glID, "Dm"), world.Dm);
						glUniform1f(glGetUniformLocation(cellsPhysics_processPositionComputeShader.glID, "Dp"), world.Dp);
					}

					glDispatchCompute(8, 8, world.mec / 1024);

					glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
				}

			count_of_updates++;
		}

		// Графика
		{
			// чашка петри
			if (1) {
				glUseProgram(petriShader.glID);
				static bool first_call = 1;
				if (first_call) {
					first_call = 0;
					glUniform1i(glGetUniformLocation(petriShader.glID, "Dm"), world.Dm + 2);
					glUniform1f(glGetUniformLocation(petriShader.glID, "Dp"), world.Dp);
				}
				glUniform4f(glGetUniformLocation(petriShader.glID, "ViewWorld"), mnPos[0], mxPos[1], mxPos[0], mnPos[1]);
				vec2 dPos = mnPos - world.Dp;
				glUniform4f(glGetUniformLocation(petriShader.glID, "deltaViewWorld"), dPos[0], dPos[1], dPos[0], dPos[1]);
				glUniform2i(glGetUniformLocation(petriShader.glID, "WinSize"), Xd, Yd);
				glUniform2f(glGetUniformLocation(petriShader.glID, "cPos"), world.view.Pos[0], world.view.Pos[1]);
				glUniform2f(glGetUniformLocation(petriShader.glID, "mPos"), mPos[0], mPos[1]);
				glUniform1f(glGetUniformLocation(petriShader.glID, "mst"), world.view.mst);


				glBindVertexArray(fullScreenMesh.VAO);
				for (int i = -1; i < 50 * boost.is_press; i++) {
					glDrawArrays(GL_TRIANGLE_STRIP, 0, fullScreenMesh.size);
				}
			}


			// клетки
			if (1) {

				glUseProgram(cellsShader.glID);
				static bool first_call = 1;
				if (first_call) {
					first_call = 0;
					glUniform1i(glGetUniformLocation(cellsShader.glID, "Dm"), world.Dm);
					glUniform1f(glGetUniformLocation(cellsShader.glID, "Dp"), world.Dp);
				}
				glUniform4f(glGetUniformLocation(cellsShader.glID, "ViewWorld"), mnPos[0], mxPos[1], mxPos[0], mnPos[1]);
				glUniform2i(glGetUniformLocation(cellsShader.glID, "WinSize"), Xd, Yd);
		//		glUniform2f(glGetUniformLocation(cellsShader.glID, "cPos"), world.view.Pos[0], world.view.Pos[1]);
		//		glUniform2f(glGetUniformLocation(cellsShader.glID, "mPos"), mPos[0], mPos[1]);
				glUniform1f(glGetUniformLocation(cellsShader.glID, "mst"), world.view.mst);
				glUniform1f(glGetUniformLocation(cellsShader.glID, "fTime"), count_of_frames / 60.);
				glUniform1i(glGetUniformLocation(cellsShader.glID, "GM"), gmesh.is_press);


				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glBindVertexArray(voidMesh.VAO);
				for (int i = -1; i < 50 * boost.is_press; i++) {
					glDrawArraysInstanced(GL_POINTS, 0, voidMesh.size, world.mec); //world.set_max_ec
				}
				glDisable(GL_BLEND);
			}
		}

		// Подсчёт кадров
		{
			count_of_frames++;
			buff.str("");
			bool swapBit = count_of_frames % 2;
			frame_time_point[swapBit] = chGetTime();
			frameTime.push(chDurationMillis(frame_time_point[swapBit], frame_time_point[!swapBit]));
			frameTimeLowLatency.push(chDurationMillis(frame_time_point[swapBit], frame_time_point[!swapBit]));
			if (count_of_frames % 100000 == 0)
				frameTime.resum(), frameTimeLowLatency.resum();

			buff << "fps - " << std::fixed << std::setprecision(1) << 1000. / frameTime.get();
			buff << "  ms - " << std::fixed << std::setprecision(2) << frameTime.get();
			buff << "         " << std::fixed << std::setprecision(1) << count_of_frames / 60.;
			glfwSetWindowTitle(window, buff.str().c_str());
		}

		glfwSwapBuffers(window);
	}

	return 1;
}




int main() {
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_SAMPLES, 64);

	auto window = glfwCreateWindow(800, 600, "CellBakery Alternative", NULL, NULL);
	if (window == NULL) return -1;

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

	Context c(window);
	int r = c.run();

	glfwDestroyWindow(window);
	glfwTerminate();

	//int t;
	//std::cin >> t;
	return r;
}