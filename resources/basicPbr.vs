#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 vPos;
out vec3 vNormal;

uniform mat4 uProjection;
uniform mat4 uView;

void main()
{
    vPos = aPos.xyz;
    vNormal = aNormal;
    gl_Position =  uProjection * uView * vec4(aPos, 1.0f);
}