#version 410 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUv;

out vec4 FragColor;

uniform int uMode;

void main()
{
  vec3 color = uMode == 2 ? vec3(vUv, 0.0) : (uMode == 1 ? vNormal : vPos);
  FragColor = vec4(color, 1.0);
}