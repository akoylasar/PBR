#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUv;

out vec3 vPos;
out vec3 vNormal;
out vec2 vUv;

layout (std140) uniform ubMatrices
{
  mat4 uProjection;
  mat4 uView;
};

void main()
{
  // Just to have the backfaces rendered most conveniently.
  vPos = aPos.xyz;
  vNormal = aNormal;
  vUv = aUv;
  vec4 clip = uProjection * mat4(mat3(uView)) * vec4(-vPos, 1.0);
  gl_Position = clip.xyww;
}