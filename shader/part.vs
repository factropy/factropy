#version 330

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in float shine;
layout(location = 4) in float ffilter;
layout(location = 12) in mat4 instance;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 lightSpace;
uniform float fogDensity;
uniform float fogOffset;
uniform vec4 fogColor;

out vec3 fragPosition;
out vec3 fragVertex;
out vec3 fragNormal;
out vec4 fragColor;
out float fragShine;
out float fragFilter;
out vec4 fragLightSpace;

void main() {
    fragVertex = vertex;

    fragPosition = vec3(instance * vec4(vertex, 1.0));

    fragNormal = normalize(transpose(inverse(mat3(instance))) * normal);

    fragLightSpace = lightSpace * vec4(fragPosition, 1.0);

    fragColor = color;

    fragShine = shine;

    fragFilter = ffilter;

    gl_Position = projection * view * instance * vec4(vertex, 1.0);
}
