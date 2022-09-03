#pragma once

#include "gl-ex.h"
#include "glm-ex.h"
#include "shader.h"
#include "common.h"
#include "miniset.h"
#include <string>
#include <span>
#include <map>
#include <set>
#include <mutex>
#include <thread>

class Mesh {
public:
	static inline std::mutex allMutex;
	static inline miniset<Mesh*> all;
	static void reset();

	GLuint vao = 0;

	struct {
		GLuint vertex = 0;
		GLuint normal = 0;
		GLuint index = 0;
	} vbo;

	minivec<glm::vec3> vertices;
	minivec<glm::vec3> normals;
	minivec<GLuint> indices;

	void loadSTL(std::string stl);
	void init(glm::mat4 srt);

	struct renderGroup {
		minivec<glm::mat4> instances;
		minivec<glm::mat4> shadowInstances;
		minivec<glm::vec4> colors;
		minivec<GLfloat> shines;
		minivec<GLfloat> filters;
	};

	struct {
		std::atomic<uint> instances = {0};
	} stats;

	static inline uint8_t current = 0;
	static inline uint8_t future = 1;
	std::map<GLuint,renderGroup> groups[2];
	std::mutex mutex[2];

	static void batchInstances();
	static void flushInstances();

	Mesh();
	Mesh(const std::string stl);
	Mesh(const std::string stl, glm::mat4 m);
	virtual ~Mesh();

	void load();
	void unload();
	uint64_t memory();

	void renderMany(GLuint shadowMap, std::span<const glm::mat4> instances, std::span<const glm::vec4> colors, std::span<const GLfloat> shines, std::span<const GLfloat> filters);
	void shadowMany(std::span<const glm::mat4> instances);

	void instance(GLuint group, const glm::mat4& trx, const glm::vec4& color, GLfloat shine = 0.0f, bool shadow = false, uint filter = 0);
	void instances(GLuint group, std::span<const glm::mat4> trx, const glm::vec4& color, GLfloat shine = 0.0f, bool shadow = false, uint filter = 0);
	void smooth();

	static void resetAll();
	static void prepareAll();
	static void renderAll(GLuint group, GLuint shadowMap);
	static void shadowAll(GLuint group);
};

class MeshMulti : public Mesh {
public:
	MeshMulti(Mesh* original, std::span<const glm::vec3> bumps);
};

class MeshPlane : public Mesh {
public:
	MeshPlane(float size);
};

class MeshWater : public Mesh {
public:
	MeshWater(int size);
};

class MeshCube : public Mesh {
protected:
	void cube();
public:
	MeshCube();
	MeshCube(float size);
};

class MeshLine : public MeshCube {
public:
	MeshLine(float length, float thick);
};

class MeshSphere : public Mesh {
public:
	MeshSphere();
	MeshSphere(float radius, int ndiv = 1);
};

class MeshHeightMap : public Mesh {
public:
	MeshHeightMap(int edge, float* heightmap, float base);
};
