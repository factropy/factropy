#include "mesh.h"
#include "shader.h"
#include "common.h"
#include "point.h"
#include "crew.h"
#include <set>
#include <string>
#include <fstream>
#include <algorithm>

#include "par_shapes.h"

namespace {
	// rendering thread caches to reduce mutex contention
	thread_local bool batching = false;
	thread_local std::map<Mesh*,std::map<GLuint,Mesh::renderGroup>> lgroups;
}

void Mesh::resetAll() {
	const std::lock_guard<std::mutex> lock(allMutex);
	current = current ? 0:1; // toggle
	future = current ? 0:1; // opposite
	for (auto mesh: all) {
		for (auto& [_,group]: mesh->groups[future]) {
			group.colors.clear();
			group.shines.clear();
			group.filters.clear();
			group.instances.clear();
			group.shadowInstances.clear();
		}
	}
}

void Mesh::prepareAll() {
}

void Mesh::instance(GLuint group, const glm::mat4& trx, const glm::vec4& color, GLfloat shine, bool shadow, uint filter) {
	if (batching) {
		auto& g = lgroups[this][group];
		g.colors.push_back(color);
		g.shines.push_back(shine);
		g.filters.push_back(filter);
		g.instances.push_back(trx);
		if (shadow) g.shadowInstances.push_back(trx);
		return;
	}
	const std::lock_guard<std::mutex> lock(mutex[current]);
	auto& g = groups[current][group];
	g.colors.push_back(color);
	g.shines.push_back(shine);
	g.filters.push_back(filter);
	g.instances.push_back(trx);
	if (shadow) g.shadowInstances.push_back(trx);
}

void Mesh::instances(GLuint group, const std::vector<glm::mat4>& batch, const glm::vec4& color, GLfloat shine, bool shadow, uint filter) {
	if (batching) {
		auto& g = lgroups[this][group];
		g.colors.resize(g.colors.size() + batch.size(), color);
		g.shines.resize(g.shines.size() + batch.size(), shine);
		g.filters.resize(g.filters.size() + batch.size(), filter);
		g.instances.insert(g.instances.end(), batch.begin(), batch.end());
		if (shadow) g.shadowInstances.insert(g.shadowInstances.end(), batch.begin(), batch.end());
		return;
	}
	const std::lock_guard<std::mutex> lock(mutex[current]);
	auto& g = groups[current][group];
	g.colors.resize(g.colors.size() + batch.size(), color);
	g.shines.resize(g.shines.size() + batch.size(), shine);
	g.filters.resize(g.filters.size() + batch.size(), filter);
	g.instances.insert(g.instances.end(), batch.begin(), batch.end());
	if (shadow) g.shadowInstances.insert(g.shadowInstances.end(), batch.begin(), batch.end());
}

void Mesh::batchInstances() {
	lgroups.clear();
	batching = true;
}

void Mesh::flushInstances() {
	for (auto& [mesh,mgroup]: lgroups) {
		const std::lock_guard<std::mutex> lock(mesh->mutex[future]);
		for (auto& [group,lgroup]: mgroup) {
			auto& g = mesh->groups[future][group];
			g.instances.insert(g.instances.end(), lgroup.instances.begin(), lgroup.instances.end());
			g.shadowInstances.insert(g.shadowInstances.end(), lgroup.shadowInstances.begin(), lgroup.shadowInstances.end());
			g.colors.insert(g.colors.end(), lgroup.colors.begin(), lgroup.colors.end());
			g.shines.insert(g.shines.end(), lgroup.shines.begin(), lgroup.shines.end());
			g.filters.insert(g.filters.end(), lgroup.filters.begin(), lgroup.filters.end());
		}
	}
	lgroups.clear();
	batching = false;
}

void Mesh::init(glm::mat4 srt) {
	ensuref(vertices.size() == normals.size(), "vertex/normal mismatch %llu %llu", vertices.size(), normals.size());

	for (auto& vertex: vertices) {
		vertex = srt * glm::vec4(vertex,1);
		ensuref(Point(vertex).valid(), "init: invalid vertex %s", std::string(Point(vertex)));
	}

	for (auto& normal: normals) {
		normal = srt * glm::vec4(normal,0);
		normal = glm::normalize(normal);
		ensuref(Point(normal).valid(), "init: invalid normal");
	}
}

Mesh::~Mesh() {
	const std::lock_guard<std::mutex> lock(allMutex);
	all.erase(this);
	unload();
}

Mesh::Mesh() {
	const std::lock_guard<std::mutex> lock(allMutex);
	all.insert(this);
}

Mesh::Mesh(const std::string stl) : Mesh() {
	loadSTL(stl);
	// openscad Z-up -> opengl Y-up
	init(glm::rotate(glm::radians(-90.0f), glm::vec3(1,0,0)));
}

Mesh::Mesh(const std::string stl, glm::mat4 m) : Mesh() {
	loadSTL(stl);
	// openscad Z-up -> opengl Y-up
	init(m * glm::rotate(glm::radians(-90.0f), glm::vec3(1,0,0)));
}

void Mesh::load() {
	if (!vao) {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		vbo.vertex = 0;
		vbo.normal = 0;
		vbo.index = 0;

		glGenBuffers(1, &vbo.vertex);
		glBindBuffer(GL_ARRAY_BUFFER, vbo.vertex);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &vbo.normal);
		glBindBuffer(GL_ARRAY_BUFFER, vbo.normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);

		if (indices.size()) {
			glGenBuffers(1, &vbo.index);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.index);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
		}

		glBindVertexArray(0);
	}
}

void Mesh::unload() {
	if (vao) {
		glDeleteBuffers(1, &vbo.vertex);
		glDeleteBuffers(1, &vbo.normal);
		if (vbo.index) glDeleteBuffers(1, &vbo.index);
		glDeleteVertexArrays(1, &vao);
		vao = 0;
		vbo.vertex = 0;
		vbo.normal = 0;
		vbo.index = 0;
	}
}

uint64_t Mesh::memory() {
	return
		(vertices.size() * sizeof(glm::vec3)) +
		(normals.size() * sizeof(glm::vec3)) +
		(indices.size() * sizeof(GLuint));
}

void Mesh::renderAll(GLuint group, GLuint shadowMap) {
	const std::lock_guard<std::mutex> lock(allMutex);
	for (auto mesh: all) {
		auto& g = mesh->groups[current][group];
		mesh->renderMany(shadowMap, g.instances, g.colors, g.shines, g.filters);
	}
}

void Mesh::renderMany(GLuint shadowMap, std::vector<glm::mat4>& instances, std::vector<glm::vec4>& colors, std::vector<GLfloat>& shines, std::vector<GLfloat>& filters) {
	if (!instances.size()) return;

	load();

	glBindVertexArray(vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, shadowMap);

	//layout(location = 0) in vec3 vertex;
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//layout(location = 1) in vec3 normal;
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.normal);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//layout(location = 2) in vec4 color;
	glEnableVertexAttribArray(2);
	GLuint vboColor;
	glGenBuffers(1, &vboColor);
	glBindBuffer(GL_ARRAY_BUFFER, vboColor);
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec4), colors.data(), GL_STREAM_DRAW);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glVertexAttribDivisor(2, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//layout(location = 3) in float shine;
	glEnableVertexAttribArray(3);
	GLuint vboSpecular;
	glGenBuffers(1, &vboSpecular);
	glBindBuffer(GL_ARRAY_BUFFER, vboSpecular);
	glBufferData(GL_ARRAY_BUFFER, shines.size() * sizeof(GLfloat), shines.data(), GL_STREAM_DRAW);
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glVertexAttribDivisor(3, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//layout(location = 4) in float filter;
	glEnableVertexAttribArray(4);
	GLuint vboFilter;
	glGenBuffers(1, &vboFilter);
	glBindBuffer(GL_ARRAY_BUFFER, vboFilter);
	glBufferData(GL_ARRAY_BUFFER, filters.size() * sizeof(uint32_t), filters.data(), GL_STREAM_DRAW);
	glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glVertexAttribDivisor(4, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//layout(location = 12) in mat4 instance;
	GLuint vboInstance;
	glGenBuffers(1, &vboInstance);
	glBindBuffer(GL_ARRAY_BUFFER, vboInstance);
	glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(glm::mat4), instances.data(), GL_STREAM_DRAW);

	// layout instance matrices from location 12 (4*vec4 each)
	for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(12+i);
		glVertexAttribPointer(12+i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(GLfloat) * 4));
		glVertexAttribDivisor(12+i, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (indices.size()) {
		glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instances.size());
	} else {
		glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(), instances.size());
	}

	glDeleteBuffers(1, &vboColor);
	glDeleteBuffers(1, &vboInstance);
	glDeleteBuffers(1, &vboSpecular);
	glDeleteBuffers(1, &vboFilter);
	glBindVertexArray(0);
}

void Mesh::shadowAll(GLuint group) {
	const std::lock_guard<std::mutex> lock(allMutex);
	for (auto mesh: all) {
		auto& g = mesh->groups[current][group];
		mesh->shadowMany(g.shadowInstances);
	}
}

void Mesh::shadowMany(std::vector<glm::mat4>& instances) {
	if (!instances.size()) return;

	load();

	glBindVertexArray(vao);

	//layout(location = 0) in vec3 vertex;
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.vertex);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	//layout(location = 12) in mat4 instance;
	GLuint vboInstance;
	glGenBuffers(1, &vboInstance);
	glBindBuffer(GL_ARRAY_BUFFER, vboInstance);
	glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(glm::mat4), instances.data(), GL_STREAM_DRAW);

	// layout instance matrices from location 12 (4*vec4 each)
	for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(12+i);
		glVertexAttribPointer(12+i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * sizeof(GLfloat) * 4));
		glVertexAttribDivisor(12+i, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (indices.size()) {
		glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instances.size());
	} else {
		glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(), instances.size());
	}

	glDeleteBuffers(1, &vboInstance);
	glBindVertexArray(0);
}

void Mesh::loadSTL(std::string stl) {
	//notef("Mesh: load %s", stl);

	auto ltrim = [](std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](auto ch) { return !std::isspace(ch); }));
	};

	auto rtrim = [](std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](auto ch) { return !std::isspace(ch); }).base(), s.end());
	};

	auto trim = [&](std::string& s) {
		ltrim(s);
		rtrim(s);
	};

	struct Triangle {
		size_t count = 0;
		glm::vec3 vertices[3];
		glm::vec3 normal;
	};

	std::vector<Triangle> triangles;
	Triangle triangle;

	auto peek = std::ifstream(stl, std::ifstream::binary);
	ensuref(peek, "failed to open: %s", stl);

	std::vector<char> tmp(6);
	peek.read(tmp.data(), 5);
	peek.close();

	if (std::string(tmp.data()) == "solid") {
		// https://en.wikipedia.org/wiki/STL_%28file_format%29#ASCII_STL
		auto in = std::ifstream(stl);

		for (std::string line; std::getline(in, line);) {
			trim(line);

			double x, y, z;

			if (3 == std::sscanf(line.c_str(), "facet normal %lf %lf %lf", &x, &y, &z)) {
				triangle.normal = glm::vec3((float)x,(float)y,(float)z);
				continue;
			}

			if (3 == std::sscanf(line.c_str(), "vertex %lf %lf %lf", &x, &y, &z)) {
				triangle.vertices[triangle.count++] = glm::vec3((float)x,(float)y,(float)z);

				if (triangle.count == 3) {
					triangles.push_back(triangle);
					triangle.count = 0;
				}

				continue;
			}
		}

		in.close();

	} else {

		// https://en.wikipedia.org/wiki/STL_%28file_format%29#Binary_STL
		auto in = std::ifstream(stl, std::ifstream::binary);
		in.seekg(80, in.beg);

		uint32_t count = 0;
		in.read((char*)&count, sizeof(uint32_t));

		char buf[50];

		for (uint32_t i = 0; i < count; i++) {
			in.read(buf, 50);

			triangle.count = 3;
			// endianness?
			triangle.normal = *(glm::vec3*)(buf+0);
			triangle.vertices[0] = *(glm::vec3*)(buf+12);
			triangle.vertices[1] = *(glm::vec3*)(buf+24);
			triangle.vertices[2] = *(glm::vec3*)(buf+36);

			triangles.push_back(triangle);
		}

		in.close();
	}

	for (auto& triangle: triangles) {
		for (int v = 0; v < 3; v++) {
			vertices.push_back(triangle.vertices[v]);
		}

		// OpenSCAD isn't that reliable
		if (Point(triangle.normal) == Point::Zero)
			triangle.normal = glm::up;

		for (int v = 0; v < 3; v++) {
			normals.push_back(triangle.normal);
		}
	}
}

void Mesh::smooth() {
	ensuref(!indices.size(), "mesh already indexed");
	unload();

	struct Unique {
		glm::vec3 vertex = Point::Zero;
		glm::vec3 normal = Point::Zero;
		bool done = false;
		uint index = 0;
	};

	std::unordered_map<Point,Unique> lookup;
	std::vector<Unique*> uniques;
	uniques.reserve(vertices.size());

	for (int i = 0, l = vertices.size(); i < l; i++) {
		glm::vec3 v = vertices[i];
		glm::vec3 n = normals[i];
		ensuref(Point(v).valid(), "invalid vertex %s", std::string(Point(v)));
		ensuref(Point(n).valid(), "invalid normal %s", std::string(Point(n)));
		auto& unique = lookup[v];
		unique.vertex = v;
		unique.normal += n;
		uniques.push_back(&unique);
	}

	vertices.clear();
	vertices.shrink_to_fit();
	normals.clear();
	normals.shrink_to_fit();

	for (auto& unique: uniques) {
		if (!unique->done) {
			unique->done = true;
			unique->index = vertices.size();
			vertices.push_back(unique->vertex);
			normals.push_back(glm::normalize(unique->normal));
		}
		indices.push_back(unique->index);
	}
}

MeshMulti::MeshMulti(Mesh* original, const std::vector<glm::vec3>& bumps) : Mesh() {
	for (auto& bump: bumps) {
		for (auto& i: original->indices) indices.push_back(i + vertices.size());
		for (auto& v: original->vertices) vertices.push_back(v + bump);
		for (auto& n: original->normals) normals.push_back(n);
	}
	init(glm::mat4(1.0f));
}

MeshPlane::MeshPlane(float size) : Mesh() {
	auto half = 0.5f;

	vertices.push_back(glm::vec3(-half,0,-half));
	vertices.push_back(glm::vec3(-half,0, half));
	vertices.push_back(glm::vec3( half,0,-half));
	vertices.push_back(glm::vec3( half,0, half));
	vertices.push_back(glm::vec3( half,0,-half));
	vertices.push_back(glm::vec3(-half,0, half));

	normals.push_back(glm::up);
	normals.push_back(glm::up);
	normals.push_back(glm::up);
	normals.push_back(glm::up);
	normals.push_back(glm::up);
	normals.push_back(glm::up);

	init(glm::scale(glm::vec3(size,1.0f,size)));
}

MeshWater::MeshWater(int size) : Mesh() {
	auto half = 0.5f;

	for (int x = -size/2; x < size/2; x++) {
		for (int y = -size/2; y < size/2; y++) {
			float fx = x;
			float fy = y;
			vertices.push_back(glm::vec3(fx+-half,0,fy+-half));
			vertices.push_back(glm::vec3(fx+-half,0,fy+ half));
			vertices.push_back(glm::vec3(fx+ half,0,fy+-half));
			vertices.push_back(glm::vec3(fx+ half,0,fy+ half));
			vertices.push_back(glm::vec3(fx+ half,0,fy+-half));
			vertices.push_back(glm::vec3(fx+-half,0,fy+ half));

			normals.push_back(glm::up);
			normals.push_back(glm::up);
			normals.push_back(glm::up);
			normals.push_back(glm::up);
			normals.push_back(glm::up);
			normals.push_back(glm::up);
		}
	}
	init(glm::mat4(1.0f));
}

void MeshCube::cube() {
	vertices.clear();
	normals.clear();

	auto half = 0.5f;

	auto a = glm::vec3( half, half, half);
	auto b = glm::vec3( half, half,-half);
	auto c = glm::vec3(-half, half,-half);
	auto d = glm::vec3(-half, half, half);
	auto e = glm::vec3( half,-half, half);
	auto f = glm::vec3( half,-half,-half);
	auto g = glm::vec3(-half,-half,-half);
	auto h = glm::vec3(-half,-half, half);

	vertices = {
		// up
		a,b,d,
		c,d,b,
		// down
		e,h,f,
		g,f,h,
		// east
		e,f,a,
		b,a,f,
		// west
		h,d,g,
		c,g,d,
		// north
		c,b,g,
		f,g,b,
		// south
		a,d,e,
		h,e,d,
	};

	normals = {
		glm::up,glm::up,glm::up,
		glm::up,glm::up,glm::up,
		glm::down,glm::down,glm::down,
		glm::down,glm::down,glm::down,
		glm::east,glm::east,glm::east,
		glm::east,glm::east,glm::east,
		glm::west,glm::west,glm::west,
		glm::west,glm::west,glm::west,
		glm::north,glm::north,glm::north,
		glm::north,glm::north,glm::north,
		glm::south,glm::south,glm::south,
		glm::south,glm::south,glm::south,
	};
}

MeshCube::MeshCube() : Mesh() {
}

MeshCube::MeshCube(float size) : MeshCube() {
	cube();
	init(glm::scale(glm::vec3(size,size,size)));
}

MeshLine::MeshLine(float length, float thick) : MeshCube() {
	cube();
	init(glm::scale(glm::vec3(thick,thick,length)));
}

MeshSphere::MeshSphere() : Mesh() {
}

MeshSphere::MeshSphere(float radius, int ndiv) : MeshSphere() {
	par_shapes_mesh *sphere = par_shapes_create_subdivided_sphere(ndiv);
	par_shapes_scale(sphere, radius, radius, radius);

	vertices.resize(sphere->npoints);
	normals.resize(sphere->npoints);
	indices.resize(sphere->ntriangles*3);

	ensure(sphere->points);
	ensure(sphere->normals);
	ensure(sphere->triangles);

	for (int i = 0, l = sphere->npoints*3; i < l; i+=3) {
		vertices[i/3] = {sphere->points[i+0], sphere->points[i+1], sphere->points[i+2]};
		normals[i/3] = {sphere->normals[i+0], sphere->normals[i+1], sphere->normals[i+2]};
	}

	for (int i = 0, l = sphere->ntriangles*3; i < l; i++) {
		indices.push_back(sphere->triangles[i]);
	}

	par_shapes_free_mesh(sphere);

	init(glm::mat4(1.0f));
}

MeshHeightMap::MeshHeightMap(int edge, float* heightmap, float base) : Mesh() {
	int mapX = edge;
	int mapZ = edge;

	int triangles = (mapX-1)*(mapZ-1)*2;
	vertices.resize(triangles*3);
	normals.resize(triangles*3);

	int vi = 0;
	int ni = 0;

	glm::vec3 vA;
	glm::vec3 vB;
	glm::vec3 vC;
	glm::vec3 vX;
	glm::vec3 vN;

	for (int z = 0, zo = 0; z < mapZ-1; z++, zo = z*mapX) {
		for (int x = 0; x < mapX-1; x++) {
			vertices[vi] = {(float)x, (float)heightmap[x + zo] + base, (float)z};
			vertices[vi+1] = {(float)x, (float)heightmap[x + zo + mapX] + base, (float)(z + 1)};
			vertices[vi+2] = {(float)(x + 1), (float)heightmap[(x + 1) + zo] + base, (float)z};
			vertices[vi+3] = vertices[vi+2];
			vertices[vi+4] = vertices[vi+1];
			vertices[vi+5] = {(float)(x + 1), (float)heightmap[(x + 1) + zo + mapX] + base, (float)(z + 1)};
			vi += 6;
			for (int i = 0; i < 6; i += 3) {
				vA = vertices[ni+i+0];
				vB = vertices[ni+i+1];
				vC = vertices[ni+i+2];
				vX = glm::cross(vB-vA, vC-vA);
				vN = glm::normalize(vX);
				if (!Point(vN).valid()) {
					notef("invalid normal!");
					notef("vA %s", std::string(Point(vA)));
					notef("vB %s", std::string(Point(vB)));
					notef("vC %s", std::string(Point(vC)));
					notef("vX %s", std::string(Point(vX)));
					notef("vN %s", std::string(Point(vN)));
					vN = glm::up;
				}
				normals[ni+i+0] = vN;
				normals[ni+i+1] = vN;
				normals[ni+i+2] = vN;
			}
			ni += 6;
		}
	}

	init(glm::mat4(1.0f));
}