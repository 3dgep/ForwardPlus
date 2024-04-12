// Default shader for materials.
// Assumes only a single texture coordinate set
// and a diffuse texture.

struct AppData
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

cbuffer PerObject : register( b0 )
{
    float4x4 ModelViewProjection;
}

Texture2D DiffuseTexture : register( t2 );
sampler DiffuseSampler : register( s0 );

struct VertexShaderOutput
{
    float2 texCoord : TEXCOORD0;
    float4 position : SV_POSITION;
};

VertexShaderOutput VS_main( AppData IN )
{
    VertexShaderOutput OUT;

    OUT.position = mul( ModelViewProjection, float4( IN.position, 1.0f ) );
    OUT.texCoord = IN.texCoord;

    return OUT;
}

float4 PS_main( VertexShaderOutput IN ) : SV_TARGET
{
    return DiffuseTexture.Sample( DiffuseSampler, IN.texCoord );
}