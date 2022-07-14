#version 330

layout(location = 0) in vec3 vertex;
layout(location = 12) in mat4 instance;

uniform mat4 projection;
uniform mat4 view;

void main() {
    gl_Position = projection * view * instance * vec4(vertex, 1.0);
}

