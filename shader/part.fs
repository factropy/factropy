#version 330

in vec3 fragPosition;
in vec3 fragVertex;
in vec3 fragNormal;
in vec4 fragColor;
in float fragShine;
in float fragFilter;
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

//
// Description : Array and textureless GLSL 2D/3D/4D simplex
//               noise functions.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20201014 (stegu)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
//

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
     return mod289(((x*34.0)+10.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
  {
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i);
  vec4 p = permute( permute( permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.5 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 105.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                dot(p2,x2), dot(p3,x3) ) );
  }

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
    vec4 fragAColor = fragColor;

    if (fragFilter > 0) {
        float near = 25;
        float dist = length(camera - fragPosition);
        float base = 0.85;

        // powdercoat
        if (fragFilter > 0.99 && fragFilter < 1.01) {
            if (dist < near) {
                float scale = 75;
                float powder = snoise(vec3(fragVertex.x*scale, fragVertex.y*scale, fragVertex.z*scale)) * (1.0-base) + base;
                fragAColor = vec4(fragAColor.x*powder,fragAColor.y*powder,fragAColor.z*powder,1);
            }
            else {
                float powder = base;// + ((1.0 - base) * 0.5);
                fragAColor = vec4(fragAColor.x*powder,fragAColor.y*powder,fragAColor.z*powder,1);
            }
        }
    }

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

    vec4 finalColor = (texelColor*((lit * (fragAColor + vec4(specular, 1.0))) * vec4(lightDot, 1.0)));
    finalColor += texelColor*(fragAColor*ambient);

    // Gamma correction
    finalColor = pow(finalColor, vec4(1.0/2.2));

    // Fog calculation
    float dist = max(0.0, length(camera - fragPosition) - fogOffset);

    // Exponential fog
    float fogFactor = 1.0/exp((dist*fogDensity)*(dist*fogDensity));

    fogFactor = clamp(fogFactor, 0.0, 1.0);

    finalColor = mix(fogColor, finalColor, fogFactor);

    gl_FragColor = finalColor;
}
