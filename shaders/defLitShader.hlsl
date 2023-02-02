#define PI 3.141592653f

struct FRAG_INPUT {
	float4 position : SV_POSITION;
	float3 normal : TEXCOORD0;
	float2 uv : TEXCOORD1;

	float3 lightPos : TEXCOORD2;
	float3 fragPos  : TEXCOORD3;
};

// vertex shader cbuffers
cbuffer matrices : register(b0) {
	matrix <float, 4, 4> modelMat;
	matrix <float, 4, 4> viewMat;
	matrix <float, 4, 4> projectionMat;
	matrix <float, 4, 4> normalMat;
};

// pixel shader cbuffers
cbuffer shaderData : register(b0) {
    float worldScale;
	vector <float, 4> albedoColor;
	float roughness;
	float metallic;
}

Texture2D albedo : t0;
SamplerState albedoSampler : s0;

// helper functions

float DistributionTRGGX(float3 n, float3 h, float a);
float GeometryShlickGGX(float3 n, float3 v, float k);
float3 FresnelShlick(float3 F0, float cos_x);

// shader stages

FRAG_INPUT vertex(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD) {
	FRAG_INPUT o;

	float3 lightPos = float3(3.0f, -3.0f, -5.0f);
	
	float4 posMV = mul(viewMat, mul(modelMat, float4(position, 1.0f)));
	o.position = mul(projectionMat, posMV);
	o.normal = mul((float3x3)normalMat, normal);
	o.fragPos = posMV;
	o.uv = uv;
	
	o.lightPos = mul(viewMat, float4(lightPos, 1.0f));

	return o;
}

float4 fragment(FRAG_INPUT i) : SV_TARGET{
	i.normal = normalize(i.normal);

	float3 lightColor = 200.0f * float3(1.0f, 0.95f, 0.88f);

	float3	lightDir = normalize(i.lightPos - i.fragPos),
			viewDir = normalize(-i.fragPos),
			halfVec = normalize(viewDir + lightDir);
	float N_Wi = max(dot(i.normal, lightDir), 0.0f);

	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = (1.0f - metallic) * F0 + metallic * albedoColor;
	float k = (roughness + 1) * (roughness + 1) / 8; // for IBL this would be a * a / 2

	float D = DistributionTRGGX(i.normal, halfVec, roughness);
	float G = GeometryShlickGGX(i.normal, viewDir, k) * GeometryShlickGGX(i.normal, lightDir, k);
	float3 F = FresnelShlick(F0, max(dot(i.normal, halfVec), 0.0f));
	float3 kd = float3(1.0f - F.x, 1.0f - F.y, 1.0f - F.z);
	kd *= 1.0f - metallic;

	float dist = length(i.lightPos - i.fragPos) / worldScale;
	float attenuation = 1.0f / (dist * dist + 0.0001f);
	float3 radiance = attenuation * lightColor;

	float3 BRDF =
		kd * albedoColor / PI +
		D * G * F / (4 * max(dot(viewDir, i.normal), 0.0f) * max(dot(lightDir, i.normal), 0.0f) + 0.0001f);

	float3 Lo = BRDF * radiance * N_Wi;
	float3 color = 0.02f * albedoColor + Lo;
	
	color = color / (color + float3(1.0f, 1.0f, 1.0f));
	color = pow(color, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

	float4 texColor = albedo.Sample(albedoSampler, i.uv);
	return float4(color, 1.0f) * texColor;
}

// helper function definitions

float DistributionTRGGX(float3 n, float3 h, float a) {
	float a2 = a * a;
	float n_h = max(dot(n, h), 0.0f);
	float n_h2 = n_h * n_h;

	float denom = (n_h2 * (a2 - 1.0f) + 1.0f);
	denom = denom * denom * PI;

	return a2 / denom;
}

float GeometryShlickGGX(float3 n, float3 v, float k) {
	float n_v = max(dot(n, v), 0.0f);
	float denom = n_v * (1.0f - k) + k;

	return n_v / denom;
}

float3 FresnelShlick(float3 F0, float cos_x) {
	return F0 + float3(1.0f - F0) * pow(clamp(1.0f - cos_x, 0.0f, 1.0f), 5.0f);
}