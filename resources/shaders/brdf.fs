#version 410 core

#define PI 3.1415926535

in vec3 vPos;
in vec3 vNormal;
in vec2 vUv;

out vec4 FragColor;

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits) 
{
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
  return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
  float a = roughness * roughness;
  
  float phi = 2.0 * PI * Xi.x;
  float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
  
  vec3 H;
  H.x = cos(phi) * sinTheta;
  H.y = sin(phi) * sinTheta;
  H.z = cosTheta;
  
  vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 tangent = normalize(cross(up, N));
  vec3 bitangent = cross(N, tangent);
  
  // Tangent to world space.
  vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
  return normalize(sampleVec);
}

float G_Smith(float NoV, float NoL, float roughness)
{
  float k = (roughness * roughness) / 2.0;
  float GL = NoV / (NoV * (1.0 - k) + k);
  float GV = NoL / (NoL * (1.0 - k) + k);
  return GL * GV;
}

vec2 IntegrateBRDF(float NoV, float roughness)
{
  vec3 V;
  V.x = sqrt(1.0 - NoV * NoV); // sin
  V.y = 0.0;
  V.z = NoV; // cos

  float A = 0.0;
  float B = 0.0; 

  vec3 N = vec3(0.0, 0.0, 1.0);
  
  const uint NumSamples = 2048;
  for(uint i = 0; i < NumSamples; ++i)
  {
    vec2 Xi = Hammersley(i, NumSamples);
    vec3 H = ImportanceSampleGGX(Xi, N, roughness);
    vec3 L = normalize(2.0 * dot(V, H) * H - V);

    float NoL = max(L.z, 0.0);
    float NoH = max(H.z, 0.0);
    float VoH = max(dot(V, H), 0.0);

    if(NoL > 0.0)
    {
      float G = G_Smith(NoV, NoL, roughness);
      float G_Vis = (G * VoH) / (NoH * NoV);
      float Fc = pow(1.0 - VoH, 5.0);
      A += (1.0 - Fc) * G_Vis;
      B += Fc * G_Vis;
    }
  }
  A /= float(NumSamples);
  B /= float(NumSamples);
  return vec2(A, B);
}

void main()
{
  vec2 brdf = IntegrateBRDF(vUv.x, vUv.y);
  FragColor = vec4(brdf.x, brdf.y, 0.0, 1.0);
}