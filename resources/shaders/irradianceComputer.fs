#version 410 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUv;

out vec4 FragColor;

#define PI 3.141592653589793
#define HALF_PI PI * 0.5

uniform sampler2D sBackground;

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
  vec3 N = normalize(vPos);
  vec3 Lo = vec3(0.0);   

  vec3 up = vec3(0.0, 1.0, 0.0);
  vec3 right = normalize(cross(up, N));
  up = normalize(cross(N, right));
     
  float delta = 0.025;
  float numSamples = 0.0;
  for(float phi = 0.0; phi < 2.0 * PI; phi += delta)
  {
      for(float theta = 0.0; theta < HALF_PI; theta += delta)
      {
        vec3 sampl = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
        vec3 wi = sampl.x * right + sampl.y * up + sampl.z * N; 
        vec2 wiPolar = shpericalToUvSpace(cartesianToSpherical(normalize(wi)));
        Lo += texture(sBackground, wiPolar).rgb * cos(theta) * sin(theta);
        numSamples++;
      }
  }
  Lo = (PI * Lo) / float(numSamples);

  FragColor = vec4(Lo, 1.0);
}