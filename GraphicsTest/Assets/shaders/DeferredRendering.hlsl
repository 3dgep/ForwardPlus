#include "CommonInclude.hlsl"

struct PixelShaderOutput
{
    float4 LightAccumulation    : SV_Target0;   // Ambient + emissive (R8G8B8_????) Unused (A8_UNORM)
    float4 Diffuse              : SV_Target1;   // Diffuse Albedo (R8G8B8_UNORM) Unused (A8_UNORM)
    float4 Specular             : SV_Target2;   // Specular Color (R8G8B8_UNROM) Specular Power(A8_UNORM)
    float4 NormalVS             : SV_Target3;   // View space normal (R32G32B32_FLOAT) Unused (A32_FLOAT)
};

// Pixel shader that generates the G-buffer
[earlydepthstencil]
PixelShaderOutput PS_Geometry( VertexShaderOutput IN )
{
    PixelShaderOutput OUT;

    float4 diffuse = Mat.DiffuseColor;
    if ( Mat.HasDiffuseTexture )
    {
        float4 diffuseTex = DiffuseTexture.Sample( LinearRepeatSampler, IN.texCoord );
        if ( any( diffuse.rgb ) )
        {
            diffuse *= diffuseTex;
        }
        else
        {
            diffuse = diffuseTex;
        }
    }

    // By default, use the alpha from the diffuse component.
    float alpha = diffuse.a;
    if ( Mat.HasOpacityTexture )
    {
        // If the material has an opacity texture, use that to override the diffuse alpha.
        alpha = OpacityTexture.Sample( LinearRepeatSampler, IN.texCoord ).r;
    }

    if ( alpha * Mat.Opacity < Mat.AlphaThreshold )
    {
        discard;
    }

    OUT.Diffuse = diffuse;

    float4 ambient = Mat.AmbientColor;
    if ( Mat.HasAmbientTexture )
    {
        float4 ambientTex = AmbientTexture.Sample( LinearRepeatSampler, IN.texCoord );
        if ( any( ambient.rgb ) )
        {
            ambient *= ambientTex;
        }
        else
        {
            ambient = ambientTex;
        }
    }

    // Combine the global ambient term.
    ambient *= Mat.GlobalAmbient;

    float4 emissive = Mat.EmissiveColor;
    if ( Mat.HasEmissiveTexture )
    {
        float4 emissiveTex = EmissiveTexture.Sample( LinearRepeatSampler, IN.texCoord );
        if ( any( emissive.rgb ) )
        {
            emissive *= emissiveTex;
        }
        else
        {
            emissive = emissiveTex;
        }
    }

    // TODO: Also compute directional lighting in the LightAccumulation buffer.
    OUT.LightAccumulation = ( ambient + emissive );

    float4 N;

    // Normal mapping
    if ( Mat.HasNormalTexture )
    {
        // For scense with normal mapping, I don't have to invert the binormal.
        float3x3 TBN = float3x3( normalize( IN.tangentVS ),
                                 normalize( IN.binormalVS ),
                                 normalize( IN.normalVS ) );

        N = DoNormalMapping( TBN, NormalTexture, LinearRepeatSampler, IN.texCoord );
        //return N;
    }
    // Bump mapping
    else if ( Mat.HasBumpTexture )
    {
        // For most scenes using bump mapping, I have to invert the binormal.
        float3x3 TBN = float3x3( normalize( IN.tangentVS ),
                                 normalize( -IN.binormalVS ), 
                                 normalize( IN.normalVS ) );

        N = DoBumpMapping( TBN, BumpTexture, LinearRepeatSampler, IN.texCoord, Mat.BumpIntensity );
        //return N;
    }
    // Just use the normal from the model.
    else
    {
        N = normalize( float4( IN.normalVS, 0 ) );
        //return N;
    }

    OUT.NormalVS = N;

    float specularPower = Mat.SpecularPower;
    if ( Mat.HasSpecularPowerTexture )
    {
        specularPower = SpecularPowerTexture.Sample( LinearRepeatSampler, IN.texCoord ).r * Mat.SpecularScale;
    }

    float4 specular = 0;
    if ( specularPower > 1.0f ) // If specular power is too low, don't use it.
    {
        specular = Mat.SpecularColor;
        if ( Mat.HasSpecularTexture )
        {
            float4 specularTex = SpecularTexture.Sample( LinearRepeatSampler, IN.texCoord );
            if ( any( specular.rgb ) )
            {
                specular *= specularTex;
            }
            else
            {
                specular = specularTex;
            }
        }
    }

    // Method of packing specular power from "Deferred Rendering in Killzone 2" presentation 
    // from Michiel van der Leeuw, Guerrilla (2007)
    OUT.Specular = float4( specular.rgb, log2( specularPower ) / 10.5f ); 

    return OUT;
}

cbuffer LightIndexBuffer : register( b4 )
{
    // The index of the light in the Lights array.
    uint LightIndex;
}

// The diffuse color from the view space texture.
Texture2D DiffuseTextureVS : register( t0 );
// The specular color from the screen space texture.
Texture2D SpecularTextureVS : register( t1 );
// The normal from the screen space texture.
Texture2D NormalTextureVS : register( t2 );
// The depth from the screen space texture.
Texture2D DepthTextureVS : register( t3 );

// Deferred lighting pixel shader.
[earlydepthstencil]
float4 PS_DeferredLighting( VertexShaderOutput IN ) : SV_Target
{
    // Everything is in view space.
    float4 eyePos = { 0, 0, 0, 1 };

    int2 texCoord = IN.position.xy;
    float depth = DepthTextureVS.Load( int3( texCoord, 0 ) ).r;

    float4 P = ScreenToView( float4( texCoord, depth, 1.0f ) );

    // View vector
    float4 V = normalize( eyePos - P );

    float4 diffuse = DiffuseTextureVS.Load( int3( texCoord, 0 ) );
    float4 specular = SpecularTextureVS.Load( int3( texCoord, 0 ) );
    float4 N = NormalTextureVS.Load( int3( texCoord, 0 ) );

    // Unpack the specular power from the alpha component of the specular color.
    float specularPower = exp2( specular.a * 10.5f );

    Light light = Lights[LightIndex];
    
    Material mat = (Material)0;
    mat.DiffuseColor = diffuse;
    mat.SpecularColor = specular;
    mat.SpecularPower = specularPower;

    LightingResult lit = (LightingResult)0;

    switch ( light.Type )
    {
    case DIRECTIONAL_LIGHT:
        lit = DoDirectionalLight( light, mat, V, P, N );
        break;
    case POINT_LIGHT:
        lit = DoPointLight( light, mat, V, P, N );
        break;
    case SPOT_LIGHT:
        lit = DoSpotLight( light, mat, V, P, N );
        break;
    }

    return ( diffuse * lit.Diffuse ) + ( specular * lit.Specular );
}

Texture2D DebugTexture : register( t0 );

// Pixel shader to render a texture to the screen.
float4 PS_DebugTexture( VertexShaderOutput IN ) : SV_Target
{
    //return float4( IN.texCoord, 0, 1 );
    //return DebugTexture.SampleLevel( LinearRepeatSampler, IN.texCoord, 0 );
    return DebugTexture.Sample( LinearRepeatSampler, IN.texCoord );
}

// Pixel shader to render a depth buffer to the screen.
float4 PS_DebugDepthTexture( VertexShaderOutput IN ) : SV_Target
{
    //return float4( IN.texCoord, 0, 1 );
    //return DebugTexture.SampleLevel( LinearRepeatSampler, IN.texCoord, 0 );
    return DebugTexture.Sample( LinearRepeatSampler, IN.texCoord ).rrrr;
}

// Pixel shader to render a stencil buffer to the screen.
float4 PS_DebugStencilTexture( VertexShaderOutput IN ) : SV_Target
{
    //return float4( IN.texCoord, 0, 1 );
    //return DebugTexture.SampleLevel( LinearRepeatSampler, IN.texCoord, 0 );
    return float4( 0, 1, 0, 1 );
}

uint PS_LightPicking( VertexShaderOutput IN ) : SV_Target
{
    // Only output the index of the light.
    // The index is in the range 1...maxlights because 0 is the clear color
    // and can't be used as a light index.
    return LightIndex + 1;
}