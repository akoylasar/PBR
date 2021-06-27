#version 410 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUv;

out vec4 FragColor;

void main()
{
  FragColor = vec4(vec3(vUv, 0.0), 1.0);
}