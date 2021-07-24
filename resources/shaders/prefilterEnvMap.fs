#version 410 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUv;

out vec4 FragColor;

#define PI 3.141592653589793
#define HALF_PI PI * 0.5

uniform sampler2D sBackground;
uniform float uRoughness;

vec2 cartesianToSpherical(vec3 p)
{
  return vec2(atan(p.z, p.x), asin(p.y));
}

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
  return vec2(float(i) / float(N), RadicalInverse_VdC(i));
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
  
  vec3 R = N;
  vec3 V = R;

  const uint NumSamples = 2048;
  vec3 prefilteredColor = vec3(0.0);
  float totalWeight = 0.0;
  
  for(uint i = 0; i < NumSamples; ++i)
  {
    vec2 Xi = Hammersley(i, NumSamples);
    vec3 H = ImportanceSampleGGX(Xi, N, uRoughness);
    vec3 L = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(dot(N, L), 0.0);
    if(NdotL > 0.0)
    {
      vec2 LPolar = shpericalToUvSpace(cartesianToSpherical(L));
      prefilteredColor += texture(sBackground, LPolar).rgb * NdotL;
      totalWeight += NdotL;
    }
  }

  prefilteredColor = prefilteredColor / totalWeight;

  FragColor = vec4(prefilteredColor, 1.0);
}