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
uniform int treeBreeze;

uniform float tick;

out vec3 fragPosition;
out vec3 fragVertex;
out vec3 fragNormal;
out vec4 fragColor;
out float fragShine;
out float fragFilter;
out vec4 fragLightSpace;

float hash(vec3 p)
{
    p  = fract( p*0.3183099+.1 );
    p *= 17.0;
    return fract( p.x*p.y*p.z*(p.x+p.y+p.z) );
}

float noise( in vec3 x )
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f*f*(3.0-2.0*f);

    return mix(mix(mix( hash(i+vec3(0,0,0)),
                        hash(i+vec3(1,0,0)),f.x),
                   mix( hash(i+vec3(0,1,0)),
                        hash(i+vec3(1,1,0)),f.x),f.y),
               mix(mix( hash(i+vec3(0,0,1)),
                        hash(i+vec3(1,0,1)),f.x),
                   mix( hash(i+vec3(0,1,1)),
                        hash(i+vec3(1,1,1)),f.x),f.y),f.z);
}

const mat3 m = mat3( 0.00,  0.80,  0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

// https://www.shadertoy.com/view/4sfGzS
float fnoise(in vec3 pos)
{
    vec3 q = 8.0*pos;
    float f = 0.5000*noise( q ); q = m*q*2.01;
    f += 0.2500*noise( q ); q = m*q*2.02;
    f += 0.1250*noise( q ); q = m*q*2.03;
    f += 0.0625*noise( q ); q = m*q*2.01;
    return f;
}

void main() {
    fragVertex = vertex;

    vec3 vertexBumped = vertex;
    vec3 normalBumped = normal;

    if (treeBreeze == 1) {
//        float t = 0; //(tick+(ffilter*100))/375.0;
//        float bumpX = 0; //(fnoise(vertex + vec3(t,0,0))-0.5)*0.5;
//        float bumpY = 0; //(fnoise(vertex + vec3(0,t,0))-0.5)*0.5;
//        float bumpZ = 0; //(fnoise(vertex + vec3(0,0,t))-0.5)*0.5;
//        normalBumped = normalize(normal);// + vec3(bumpX, bumpY, bumpZ));

        float l = length(vec3(vertex.x, 0, vertex.z));
        if (l > 0.25) {
            float t = (tick+(ffilter*100))/750.0;
            float bumpX = 0.0;
            float bumpY = (fnoise(vertex + vec3(0,t,0))-0.5)*0.5;
            float bumpZ = 0.0;
            vertexBumped = vertex + vec3(bumpX, bumpY, bumpZ);
        }
    }

    fragPosition = vec3(instance * vec4(vertexBumped, 1.0));

    mat3 normalMatrix = transpose(inverse(mat3(instance)));
    fragNormal = normalize(normalMatrix * normalBumped);

    fragLightSpace = lightSpace * vec4(fragPosition + vec3(0,0.5,0), 1.0);

    fragColor = color;

    fragShine = shine;

    fragFilter = ffilter;

    gl_Position = projection * view * instance * vec4(vertexBumped, 1.0);
}

