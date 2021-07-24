#version 410 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUv;

out vec3 vPos;
out vec3 vNormal;
out vec2 vUv;

void main()
{
  vUv = aUv;
  gl_Position = vec4(aPos, 1.0);
}