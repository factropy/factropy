#pragma once
#include "gl-ex.h"
#include <map>
#include <string>

class Shader {
	struct {
		GLuint id = 0;
		std::map<std::string,GLuint> uniform;
	} my;
public:
	GLuint id() const;
	Shader() = default;
	Shader(const char* vertexPath, const char* fragmentPath);
	GLuint uniform(const std::string& name);
};

