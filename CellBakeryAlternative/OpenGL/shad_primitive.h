#pragma once

namespace shad {
	const bool using_external_code_of_shaders = 0;

	class Shader;
	class SimpleMesh;
	class EBOMesh;

	class VoidMesh;
};

class shad::SimpleMesh {
public:
	SimpleMesh() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
	}
	~SimpleMesh() {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
	// Загружает в VBO данные из data (x и y координаты в формате 0.f - 1.f)
	virtual void loadFrom(float* data, int sz) {
		size = sz / 2;
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(data) * sz, data, GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	uint32_t VAO, VBO;
	int size;
private:
};

class shad::EBOMesh {
public:
	EBOMesh() {
		glGenBuffers(1, &EBO);
	}
	~EBOMesh() {
		glDeleteBuffers(1, &EBO);
	}
	// Загружает в EBO данные из data (id из будера вершин)
	virtual void loadFrom(uint32_t* data, int sz) {
		size = sz;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data) * sz, data, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	uint32_t EBO;
	int size;
private:
};

class shad::Shader {
public:
	std::string name;
	Shader(const char *n) : name(n) {
		glID = NULL;
	}
	Shader() {
		glID = NULL;
	}
	Shader(const char *sourseVert, const char *sourseFrag) {
		compile(sourseVert, sourseFrag);
	}

	bool compile(const char *sourseVert, const char *sourseFrag, const char *sourseGeom) {
		int success;
		bool err = 0;
		char infoLog[512];

		// вершинный шейдер
		int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		err |= interimСomp(vertexShader, sourseVert, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n");


		// фрагментный шейдер
		int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		err |= interimСomp(fragmentShader, sourseFrag, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n");

		// геометрический шейдер
		int geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
		err |= interimСomp(geometryShader, sourseGeom, "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n");

		// связывание шейдеров
		glID = glCreateProgram();
		glAttachShader(glID, vertexShader);
		glAttachShader(glID, fragmentShader);
		glAttachShader(glID, geometryShader);
		err |= interimLink();
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		glDeleteShader(geometryShader);
		return err;
	}

	bool compile(const char *sourseVert, const char *sourseFrag) {
		int success;
		bool err = 0;
		char infoLog[512];

		// вершинный шейдер
		int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		err |= interimСomp(vertexShader, sourseVert, "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n");


		// фрагментный шейдер
		int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		err |= interimСomp(fragmentShader, sourseFrag, "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n");

		// связывание шейдеров
		glID = glCreateProgram();
		glAttachShader(glID, vertexShader);
		glAttachShader(glID, fragmentShader);
		err |= interimLink();
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		return err;
	}

	uint32_t glID;
private:
	inline bool interimСomp(int Shader, const char *sourse, const char *errlog) {
		int success;
		bool err = 0;
		char infoLog[512];
		glShaderSource(Shader, 1, &sourse, NULL);
		glCompileShader(Shader);
		glGetShaderiv(Shader, GL_COMPILE_STATUS, &success); // проверка на наличие ошибок компилирования шейдера
		if (!success) {
			err = 1;
			glGetShaderInfoLog(Shader, 512, NULL, infoLog);
			std::cout << name.c_str() << ":\n";
			std::cout << errlog << infoLog << std::endl;
		}
		return err;
	}
	inline bool interimLink() {
		int success;
		bool err = 0;
		char infoLog[512];
		glLinkProgram(glID);
		glGetProgramiv(glID, GL_LINK_STATUS, &success); // проверка на наличие ошибок связывания шейдеров
		if (!success) {
			err = 1;
			glGetProgramInfoLog(glID, 512, NULL, infoLog);
			std::cout << name.c_str() << ":\n";
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}
		return err;
	}
};


class shad::VoidMesh {
public:
	VoidMesh() : size(1) {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, 1, NULL, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	~VoidMesh() {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
	uint32_t VAO, VBO;
	int size;
private:
};