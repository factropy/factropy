#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec4 fragColor;

uniform vec3 camera;
uniform float fogDensity;
uniform float fogOffset;
uniform vec4 fogColor;

void main() {

	vec4 finalColor = fragColor;

    // Gamma correction
    finalColor = pow(finalColor, vec4(1.0/2.2));

    // Transparency
    finalColor.a = 0.6;
    if (fragColor.a < 0.99) finalColor.a = fragColor.a;

    // Fog calculation
    float dist = max(0.0, length(camera - fragPosition) - fogOffset);

    // Exponential fog
    float fogFactor = 1.0/exp((dist*fogDensity)*(dist*fogDensity));

    fogFactor = clamp(fogFactor, 0.0, 1.0);

    finalColor = mix(fogColor, finalColor, fogFactor);

    gl_FragColor = finalColor;
}

