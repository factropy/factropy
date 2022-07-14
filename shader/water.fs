#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec4 fragColor;
in float fragShine;
in vec4 fragLightSpace;

uniform sampler2D texture0;
uniform vec3 camera;
uniform vec4 ambient;
uniform vec4 sunColor;
uniform vec3 sunPosition;
uniform float fogDensity;
uniform float fogOffset;
uniform vec4 fogColor;

#define MAX_LIGHTS 4
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT 1

struct Light {
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};

float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.x < 0.0 || projCoords.y < 0.0 || projCoords.z < 0.0)
        return 0.0;

    if(projCoords.x > 1.0 || projCoords.y > 1.0 || projCoords.z > 1.0)
        return 0.0;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(texture0, projCoords.xy).r;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(texture0, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(texture0, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main() {
    Light sun;
    sun.type = LIGHT_DIRECTIONAL;
    sun.position = sunPosition;
    sun.target = vec3(0,0,0);
    sun.color = sunColor;

    vec4 texelColor = vec4(1,1,1,1);
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(camera - fragPosition);
    vec3 specular = vec3(0.0);

            vec3 light = vec3(0.0);

            if (sun.type == LIGHT_DIRECTIONAL)
            {
                light = -normalize(sun.target - sun.position);
            }

            if (sun.type == LIGHT_POINT)
            {
                light = normalize(sun.position - fragPosition);
            }

            float NdotL = max(dot(normal, light), 0.0);
            lightDot += sun.color.rgb*NdotL;

            float specCo = 0.0;
            if (NdotL > 0.0 && fragShine > 0.0) {
                specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), fragShine);
            }
            specular += specCo;

    vec3 lightDir = normalize(sun.position - sun.target);
    float cosTheta = clamp(dot(normal, lightDir), 0, 1);
    float bias = clamp(0.0005*tan(acos(cosTheta)), 0, 0.001);

    float shadow = ShadowCalculation(fragLightSpace, bias);
    if (shadow > 0.5) shadow = 0.5;
    float lit = 1.0 - shadow;

    vec4 finalColor = (texelColor*((lit * (fragColor + vec4(specular, 1.0))) * vec4(lightDot, 1.0)));
    finalColor += texelColor*(fragColor*ambient);

    // Gamma correction
    finalColor = pow(finalColor, vec4(1.0/2.2));
    finalColor.a = 0.9;

    // Fog calculation
    float dist = max(0.0, length(camera - fragPosition) - fogOffset);

    // Exponential fog
    float fogFactor = 1.0/exp((dist*fogDensity)*(dist*fogDensity));

    fogFactor = clamp(fogFactor, 0.0, 1.0);

    finalColor = mix(fogColor, finalColor, fogFactor);

    gl_FragColor = finalColor;
}
