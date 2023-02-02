struct FRAG_INPUT {
	float4 position : SV_POSITION;
	float4 color : TEXCOORD0;
	float3 viewPos : TEXCOORD1;
};

cbuffer matrices : register (b0) {
	matrix <float, 4, 4> modelMat;
	matrix <float, 4, 4> viewMat;
	matrix <float, 4, 4> projectionMat;
}

cbuffer shaderData : register (b0) {
    float worldScale;
	vector <float, 4> color;
}

FRAG_INPUT vertex(float3 position: POSITION, vector <float, 4> vertexColor: COLOR) {
	FRAG_INPUT o;

	float4 posMV = mul(viewMat, mul(modelMat, float4(position, 1.0f)));
	o.position = mul(projectionMat, posMV);
	o.color = vertexColor;
	o.viewPos = posMV;

	return o;
}

float4 fragment(FRAG_INPUT i) : SV_TARGET{
	float falloff = length(i.viewPos) / worldScale;
	falloff = clamp(falloff / 500.0f, 0.0f, 1.0f);
	i.color.w = i.color.w * pow(1.0f - falloff, 2.0f) * 2.0f;
    
	i.color.w *= 0.5f;
	
	if (i.color.w < 0.02f) discard;
    return i.color * color;
}