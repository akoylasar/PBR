#version 410 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUv;

out vec4 FragColor;

uniform sampler2D sEnvironment;

vec2 cartesianToSpherical(vec3 p)
{
  return vec2(atan(p.z, p.x), asin(p.y));
}

vec2 shpericalToUvSpace(vec2 p)
{
  // spherical to -1, 1 range.
  const vec2 s = vec2(0.3183, 0.6366); // 1 / (pi), 1 / (pi / 2)
  vec2 uv = 0.5 * p * s;

  // to UV
  uv += 0.5;
  return uv;
}

void main()
{
  vec2 uv = shpericalToUvSpace(cartesianToSpherical(normalize(vPos)));

  vec3 color = texture(sEnvironment, uv).rgb;
  
  // Tone-mapping
  color = color / (vec3(1.0) + color);
  // Gamma correction
  color = pow(color, vec3(1.0 / 2.2));

  FragColor = vec4(color, 1.0);
}