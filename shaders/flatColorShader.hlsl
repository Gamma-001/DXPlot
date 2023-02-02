struct FRAG_INPUT {
    float4 position : SV_POSITION;
};

cbuffer matrices : register(b0) {
    matrix<float, 4, 4> modelMat;
    matrix<float, 4, 4> viewMat;
    matrix<float, 4, 4> projectionMat;
}

cbuffer shaderData : register(b0) {
    float worldScale;
    float3 albedoColor;
}

FRAG_INPUT vertex(float3 position : POSITION) {
    FRAG_INPUT o;

    float4 posMV = mul(viewMat, mul(modelMat, float4(position, 1.0f)));
    o.position = mul(projectionMat, posMV);

    return o;
}

float4 fragment(FRAG_INPUT i) : SV_TARGET {
    return float4(albedoColor, 1.0f);
}