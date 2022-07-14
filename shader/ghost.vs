#version 330

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 12) in mat4 instance;

uniform mat4 projection;
uniform mat4 view;

out vec3 fragPosition;
out vec3 fragNormal;
out vec4 fragColor;

void main() {
    fragPosition = vec3(instance * vec4(vertex, 1.0));

    fragNormal = normalize(transpose(inverse(mat3(instance))) * normal);

    fragColor = color;

    gl_Position = projection * view * instance * vec4(vertex, 1.0);
}

