
#include "common.h"
#include "shader.h"
#include <vector>
#include <iostream>
#include <fstream>

GLuint Shader::id() const {
	return my.id;
}

GLuint Shader::uniform(const std::string& name) {
	if (!my.uniform.count(name)) {
		my.uniform[name] = glGetUniformLocation(my.id, name.c_str());
	}
	return my.uniform[name];
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
	auto slurp = [](const char* path) {
		std::ifstream ifs(path);
		ensuref(ifs, "read failed: %s", path);
		return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	};

	auto vsrc = slurp(vertexPath);
	auto fsrc = slurp(fragmentPath);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	ensuref(vs, "vs", glErr);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	ensuref(fs, "%s", glErr);

	GLint status;
	int length;
	const char* src;

	src = vsrc.c_str();
	length = vsrc.size();
	glShaderSource(vs, 1, (const GLchar**)&src, &length);
	glCompileShader(vs);

	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &length);

	if (status == GL_FALSE) {
		std::vector<char> err(length+1);
		glGetShaderInfoLog(vs, length, NULL, &err[0]);
		std::cerr << std::string(err.data()) << std::endl;
		ensuref(0, "vertex shader compilation failed: %s", err.data());
	}

	src = fsrc.c_str();
	length = fsrc.size();
	glShaderSource(fs, 1, (const GLchar**)&src, &length);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);

	if (status == GL_FALSE) {
		std::vector<char> err(length+1);
		glGetShaderInfoLog(fs, length, NULL, &err[0]);
		std::cerr << std::string(err.data()) << std::endl;
		ensuref(0, "fragment shader compilation failed: %s", err.data());
	}

	my.id = glCreateProgram();
	glAttachShader(id(), vs);
	glAttachShader(id(), fs);
	glLinkProgram(id());

	glGetProgramiv(id(), GL_LINK_STATUS, &status);
	glGetProgramiv(id(), GL_INFO_LOG_LENGTH, &length);

	if (status == GL_FALSE) {
		std::vector<char> err(length+1);
		glGetProgramInfoLog(fs, length, NULL, &err[0]);
		std::cerr << std::string(err.data()) << std::endl;
		ensuref(0, "program link compilation failed: %s", err.data());
	}

	glDetachShader(id(), vs);
	glDetachShader(id(), fs);

	glDeleteShader(vs);
	glDeleteShader(fs);
}
