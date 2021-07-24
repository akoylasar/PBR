#version 410 core

#define PI 3.1415926535

in vec3 vPos;
in vec3 vNormal;

out vec4 FragColor;

uniform vec3 uAlbedo;
uniform float uMetallic;
uniform float uRoughness;
uniform float uAo;

uniform vec3 uCameraPos;

#define NUM_LIGHTS 4
uniform vec3 uLightPositions[NUM_LIGHTS];
uniform vec3 uLightColor;

uniform samplerCube sIrradianceMap;

// The next four functions are described in great details at:
// https://learnopengl.com/PBR/Lighting
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float num = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float num = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

void main()
{
  vec3 F0 = vec3(0.04); // Good approx for dielectrics.
  F0 = mix(F0, uAlbedo, uMetallic);
  vec3 N = vNormal;
  vec3 V = normalize(uCameraPos - vPos);

  vec3 Lo = vec3(0.0);
  for (int i = 0; i < NUM_LIGHTS; i++)
  {
    vec3 lightPos = uLightPositions[i];

    vec3 L = normalize(lightPos - vPos);
    vec3 H = normalize(L + V);

    float d = length(lightPos - vPos);
    vec3 radiance = uLightColor / (d * d);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(vNormal, H, uRoughness);        
    float G = GeometrySmith(N, V, L, uRoughness);      
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0); 

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - uMetallic;

    float NdotL = max(dot(N, L), 0.0);  
    float NdotV = max(dot(N, V), 0.0);             

    vec3 specular = (NDF * G * F) / max(4.0 * NdotV * NdotL, 0.001); 
    // Lo = radiance;
    Lo += (kD * (uAlbedo / PI) + specular) * radiance * NdotL;
  }

  // Diffuse light from the environment as the ambient term.
  vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
  vec3 kD = 1.0 - kS;

  vec3 irradiance = texture(sIrradianceMap, N).rgb;
  vec3 diffuse = irradiance * uAlbedo;
  vec3 ambient = kD * diffuse * uAo;

  vec3 color = ambient + Lo;

  // Tone-mapping
  color = color / (vec3(1.0) + color);
  // Gamma correction
  color = pow(color, vec3(1.0 / 2.2));

  FragColor = vec4(color, 1.0);
}