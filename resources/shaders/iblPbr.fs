#version 410 core

#define PI 3.1415926535
#define MAX_PREFILTER_MIP 4

in vec3 vPos;
in vec3 vNormal;

out vec4 FragColor;

uniform vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAo;

uniform vec3 uCameraPos;

uniform samplerCube sIrradianceMap;
uniform samplerCube sPrefilterMap;
uniform sampler2D sBrdf;

vec3 fresnelSchlick(float cosTheta, vec3 F0, float roughness)
{
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

void main()
{
  vec3 F0 = vec3(0.04); // Good approx for dielectrics.
  F0 = mix(F0, uAlbedo, uMetallic);
  vec3 N = vNormal;
  vec3 V = normalize(uCameraPos - vPos);
  vec3 R = 2 * dot(N, V) * N - V;

  vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0, uRoughness);
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - uMetallic;

  // Diffuse term.
  vec3 irradiance = texture(sIrradianceMap, N).rgb;
  vec3 diffuse = kD * uAlbedo * irradiance;

  // Specular term.
  vec3 prefilter = textureLod(sPrefilterMap, R, uRoughness * MAX_PREFILTER_MIP).rgb;
  vec2 brdf = texture(sBrdf, vec2(max(dot(N, V), 0.0), uRoughness)).rg;
  vec3 specular = prefilter * (kS * brdf.x + brdf.y);

  vec3 color = (diffuse + specular) * uAo;

  // Tone-mapping
  color = color / (vec3(1.0) + color);
  // Gamma correction
  color = pow(color, vec3(1.0 / 2.2));

  FragColor = vec4(color, 1.0);
}