#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUv;

out vec3 vPos;
out vec3 vNormal;

layout (std140) uniform ubMatrices
{
  mat4 uProjection;
  mat4 uView;
};

void main()
{
  vPos = aPos.xyz;
  vNormal = aNormal;
  gl_Position =  uProjection * uView * vec4(aPos, 1.0f);
}