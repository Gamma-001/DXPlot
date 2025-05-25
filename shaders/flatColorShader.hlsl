struct FRAG_INPUT {
    float4 position : SV_POSITION;
    float4 color : TEXCOOD0;
};

cbuffer matrices : register(b0) {
    matrix<float, 4, 4> modelMat;
    matrix<float, 4, 4> viewMat;
    matrix<float, 4, 4> projectionMat;
}

cbuffer shaderData : register(b0) {
    float worldScale;
    float4 albedoColor;
}

FRAG_INPUT vertex(float3 position : POSITION, float4 vertexColor : COLOR) {
    FRAG_INPUT o;

    float4 posMV = mul(viewMat, mul(modelMat, float4(position, 1.0f)));
    o.position = mul(projectionMat, posMV);
    o.color = vertexColor;

    return o;
}

float4 fragment(FRAG_INPUT i) : SV_TARGET {
    return i.color * albedoColor;
}