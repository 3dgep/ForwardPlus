#include "CommonInclude.hlsl"

#ifndef BLOCK_SIZE
#pragma message( "BLOCK_SIZE undefined. Default to 16.")
#define BLOCK_SIZE 16 // should be defined by the application.
#endif

struct ComputeShaderInput
{
    uint3 groupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 groupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 dispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint  groupIndex        : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

// Global variables
cbuffer DispatchParams : register( b4 )
{
    // Number of groups dispatched. (This parameter is not available as an HLSL system value!)
    uint3   numThreadGroups;
    // uint padding // implicit padding to 16 bytes.
    // Total number of threads dispatched. (Also not available as an HLSL system value!)
    // Note: This value may be less than the actual number of threads executed 
    // if the screen size is not evenly divisible by the block size.
    uint3   numThreads;
    // uint padding // implicit padding to 16 bytes.
}

// The depth from the screen space texture.
Texture2D DepthTextureVS : register( t3 );
// Precomputed frustums for the grid.
StructuredBuffer<Frustum> in_Frustums : register( t9 );

// Debug texture for debugging purposes.
Texture2D LightCountHeatMap : register( t10 );
RWTexture2D<float4> DebugTexture : register( u0 );

// Global counter for current index into the light index list.
// "o_" prefix indicates light lists for opaque geometry while 
// "t_" prefix indicates light lists for transparent geometry.
globallycoherent RWStructuredBuffer<uint> o_LightIndexCounter : register( u1 );
globallycoherent RWStructuredBuffer<uint> t_LightIndexCounter : register( u2 );

// Light index lists and light grids.
RWStructuredBuffer<uint> o_LightIndexList : register( u3 );
RWStructuredBuffer<uint> t_LightIndexList : register( u4 );
RWTexture2D<uint2> o_LightGrid : register( u5 );
RWTexture2D<uint2> t_LightGrid : register( u6 );

// Group shared variables.
groupshared uint uMinDepth;
groupshared uint uMaxDepth;
groupshared Frustum GroupFrustum;

// Opaque geometry light lists.
groupshared uint o_LightCount;
groupshared uint o_LightIndexStartOffset;
groupshared uint o_LightList[1024];

// Transparent geometry light lists.
groupshared uint t_LightCount;
groupshared uint t_LightIndexStartOffset;
groupshared uint t_LightList[1024];

// Add the light to the visible light list for opaque geometry.
void o_AppendLight( uint lightIndex )
{
    uint index; // Index into the visible lights array.
    InterlockedAdd( o_LightCount, 1, index );
    if ( index < 1024 )
    {
        o_LightList[index] = lightIndex;
    }
}

// Add the light to the visible light list for transparent geometry.
void t_AppendLight( uint lightIndex )
{
    uint index; // Index into the visible lights array.
    InterlockedAdd( t_LightCount, 1, index );
    if ( index < 1024 )
    {
        t_LightList[index] = lightIndex;
    }
}

// Implementation of light culling compute shader is based on the presentation
// "DirectX 11 Rendering in Battlefield 3" (2011) by Johan Andersson, DICE.
// Retrieved from: http://www.slideshare.net/DICEStudio/directx-11-rendering-in-battlefield-3
// Retrieved: July 13, 2015
// And "Forward+: A Step Toward Film-Style Shading in Real Time", Takahiro Harada (2012)
// published in "GPU Pro 4", Chapter 5 (2013) Taylor & Francis Group, LLC.
[numthreads( BLOCK_SIZE, BLOCK_SIZE, 1 )]
void CS_main( ComputeShaderInput IN )
{
    // Calculate min & max depth in threadgroup / tile.
    int2 texCoord = IN.dispatchThreadID.xy;
    float fDepth = DepthTextureVS.Load( int3( texCoord, 0 ) ).r;

    uint uDepth = asuint( fDepth );

    if ( IN.groupIndex == 0 ) // Avoid contention by other threads in the group.
    {
        uMinDepth = 0xffffffff;
        uMaxDepth = 0;
        o_LightCount = 0;
        t_LightCount = 0;
        GroupFrustum = in_Frustums[IN.groupID.x + ( IN.groupID.y * numThreadGroups.x )];
    }

    GroupMemoryBarrierWithGroupSync();

    InterlockedMin( uMinDepth, uDepth );
    InterlockedMax( uMaxDepth, uDepth );

    GroupMemoryBarrierWithGroupSync();

    float fMinDepth = asfloat( uMinDepth );
    float fMaxDepth = asfloat( uMaxDepth );

    // Convert depth values to view space.
    float minDepthVS = ScreenToView( float4( 0, 0, fMinDepth, 1 ) ).z;
    float maxDepthVS = ScreenToView( float4( 0, 0, fMaxDepth, 1 ) ).z;
    float nearClipVS = ScreenToView( float4( 0, 0, 0, 1 ) ).z;

    // Clipping plane for minimum depth value 
    // (used for testing lights within the bounds of opaque geometry).
    Plane minPlane = { float3( 0, 0, -1 ), -minDepthVS };

    // Cull lights
    // Each thread in a group will cull 1 light until all lights have been culled.
    for ( uint i = IN.groupIndex; i < NUM_LIGHTS; i += BLOCK_SIZE * BLOCK_SIZE )
    {
        if ( Lights[i].Enabled )
        {
            Light light = Lights[i];

            switch ( light.Type )
            {
            case POINT_LIGHT:
            {
                Sphere sphere = { light.PositionVS.xyz, light.Range };
                if ( SphereInsideFrustum( sphere, GroupFrustum, nearClipVS, maxDepthVS ) )
                {
                    // Add light to light list for transparent geometry.
                    t_AppendLight( i );

                    if ( !SphereInsidePlane( sphere, minPlane ) )
                    {
                        // Add light to light list for opaque geometry.
                        o_AppendLight( i );
                    }
                }
            }
            break;
            case SPOT_LIGHT:
            {
                float coneRadius = tan( radians( light.SpotlightAngle ) ) * light.Range;
                Cone cone = { light.PositionVS.xyz, light.Range, light.DirectionVS.xyz, coneRadius };
                if ( ConeInsideFrustum( cone, GroupFrustum, nearClipVS, maxDepthVS ) )
                {
                    // Add light to light list for transparent geometry.
                    t_AppendLight( i );

                    if ( !ConeInsidePlane( cone, minPlane ) )
                    {
                        // Add light to light list for opaque geometry.
                        o_AppendLight( i );
                    }
                }
            }
            break;
            case DIRECTIONAL_LIGHT:
            {
                // Directional lights always get added to our light list.
                // (Hopefully there are not too many directional lights!)
                t_AppendLight( i );
                o_AppendLight( i );
            }
            break;
            }
        }
    }

    // Wait till all threads in group have caught up.
    GroupMemoryBarrierWithGroupSync();

    // Update global memory with visible light buffer.
    // First update the light grid (only thread 0 in group needs to do this)
    if ( IN.groupIndex == 0 )
    {
        // Update light grid for opaque geometry.
        InterlockedAdd( o_LightIndexCounter[0], o_LightCount, o_LightIndexStartOffset );
        o_LightGrid[IN.groupID.xy] = uint2( o_LightIndexStartOffset, o_LightCount );

        // Update light grid for transparent geometry.
        InterlockedAdd( t_LightIndexCounter[0], t_LightCount, t_LightIndexStartOffset );
        t_LightGrid[IN.groupID.xy] = uint2( t_LightIndexStartOffset, t_LightCount );
    }

    GroupMemoryBarrierWithGroupSync();

    // Now update the light index list (all threads).
    // For opaque goemetry.
    for ( i = IN.groupIndex; i < o_LightCount; i += BLOCK_SIZE * BLOCK_SIZE )
    {
        o_LightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
    }
    // For transparent geometry.
    for ( i = IN.groupIndex; i < t_LightCount; i += BLOCK_SIZE * BLOCK_SIZE )
    {
        t_LightIndexList[t_LightIndexStartOffset + i] = t_LightList[i];
    }

    // Update the debug texture output.
    if ( IN.groupThreadID.x == 0 || IN.groupThreadID.y == 0 )
    {
        DebugTexture[texCoord] = float4( 0, 0, 0, 0.9f );
    }
    else if ( IN.groupThreadID.x == 1 || IN.groupThreadID.y == 1 )
    {
        DebugTexture[texCoord] = float4( 1, 1, 1, 0.5f );
    }
    else if ( o_LightCount > 0 )
    {
        float normalizedLightCount = o_LightCount / 50.0f;
        float4 lightCountHeatMapColor = LightCountHeatMap.SampleLevel( LinearClampSampler, float2( normalizedLightCount, 0 ), 0);
        DebugTexture[texCoord] = lightCountHeatMapColor;
    }
    else
    {
        DebugTexture[texCoord] = float4( 0, 0, 0, 1);
    }
}

// View space frustums for the grid cells.
RWStructuredBuffer<Frustum> out_Frustums : register( u0 );

// A kernel to compute frustums for the grid
// This kernel is executed once per grid cell. Each thread
// computes a frustum for a grid cell.
[numthreads( BLOCK_SIZE, BLOCK_SIZE, 1 )]
void CS_ComputeFrustums( ComputeShaderInput IN )
{
    // View space eye position is always at the origin.
    const float3 eyePos = float3( 0, 0, 0 );

    // Compute 4 points on the far clipping plane to use as the 
    // frustum vertices.
    float4 screenSpace[4];
    // Top left point
    screenSpace[0] = float4( IN.dispatchThreadID.xy * BLOCK_SIZE, -1.0f, 1.0f );
    // Top right point
    screenSpace[1] = float4( float2( IN.dispatchThreadID.x + 1, IN.dispatchThreadID.y ) * BLOCK_SIZE, -1.0f, 1.0f );
    // Bottom left point
    screenSpace[2] = float4( float2( IN.dispatchThreadID.x, IN.dispatchThreadID.y + 1 ) * BLOCK_SIZE, -1.0f, 1.0f );
    // Bottom right point
    screenSpace[3] = float4( float2( IN.dispatchThreadID.x + 1, IN.dispatchThreadID.y + 1 ) * BLOCK_SIZE, -1.0f, 1.0f );

    float3 viewSpace[4];
    // Now convert the screen space points to view space
    for ( int i = 0; i < 4; i++ )
    {
        viewSpace[i] = ScreenToView( screenSpace[i] ).xyz;
    }

    // Now build the frustum planes from the view space points
    Frustum frustum;

    // Left plane
    frustum.planes[0] = ComputePlane( eyePos, viewSpace[2], viewSpace[0] );
    // Right plane
    frustum.planes[1] = ComputePlane( eyePos, viewSpace[1], viewSpace[3] );
    // Top plane
    frustum.planes[2] = ComputePlane( eyePos, viewSpace[0], viewSpace[1] );
    // Bottom plane
    frustum.planes[3] = ComputePlane( eyePos, viewSpace[3], viewSpace[2] );

    // Store the computed frustum in global memory (if our thread ID is in bounds of the grid).
    if ( IN.dispatchThreadID.x < numThreads.x && IN.dispatchThreadID.y < numThreads.y )
    {
        uint index = IN.dispatchThreadID.x + ( IN.dispatchThreadID.y * numThreads.x );
        out_Frustums[index] = frustum;
    }
}

StructuredBuffer<uint> LightIndexList : register( t9 );
Texture2D<uint2> LightGrid : register( t10 );

[earlydepthstencil]
float4 PS_main( VertexShaderOutput IN ) : SV_TARGET
{
    // Everything is in view space.
    const float4 eyePos = { 0, 0, 0, 1 };
    Material mat = Mat;

    float4 diffuse = mat.DiffuseColor;
    if ( mat.HasDiffuseTexture )
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
    if ( mat.HasOpacityTexture )
    {
        // If the material has an opacity texture, use that to override the diffuse alpha.
        alpha = OpacityTexture.Sample( LinearRepeatSampler, IN.texCoord ).r;
    }

    float4 ambient = mat.AmbientColor;
    if ( mat.HasAmbientTexture )
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
    ambient *= mat.GlobalAmbient;

    float4 emissive = mat.EmissiveColor;
    if ( mat.HasEmissiveTexture )
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

    if ( mat.HasSpecularPowerTexture )
    {
        mat.SpecularPower = SpecularPowerTexture.Sample( LinearRepeatSampler, IN.texCoord ).r * mat.SpecularScale;
    }

    float4 N;

    // Normal mapping
    if ( mat.HasNormalTexture )
    {
        // For scense with normal mapping, I don't have to invert the binormal.
        float3x3 TBN = float3x3( normalize( IN.tangentVS ),
                                 normalize( IN.binormalVS ),
                                 normalize( IN.normalVS ) );

        N = DoNormalMapping( TBN, NormalTexture, LinearRepeatSampler, IN.texCoord );
        //return N;
    }
    // Bump mapping
    else if ( mat.HasBumpTexture )
    {
        // For most scenes using bump mapping, I have to invert the binormal.
        float3x3 TBN = float3x3( normalize( IN.tangentVS ),
                                 normalize( -IN.binormalVS ),
                                 normalize( IN.normalVS ) );

        N = DoBumpMapping( TBN, BumpTexture, LinearRepeatSampler, IN.texCoord, mat.BumpIntensity );
        //return N;
    }
    // Just use the normal from the model.
    else
    {
        N = normalize( float4( IN.normalVS, 0 ) );
        //return N;
    }

    float4 P = float4( IN.positionVS, 1 );
    float4 V = normalize( eyePos - P );

    // Get the index of the current pixel in the light grid.
    uint2 tileIndex = uint2( floor(IN.position.xy / BLOCK_SIZE) );

    // Get the start position and offset of the light in the light index list.
    uint startOffset = LightGrid[tileIndex].x;
    uint lightCount = LightGrid[tileIndex].y;

    LightingResult lit = (LightingResult)0; // DoLighting( Lights, mat, eyePos, P, N );

    for ( uint i = 0; i < lightCount; i++ )
    {
        uint lightIndex = LightIndexList[startOffset + i];
        Light light = Lights[lightIndex];

        LightingResult result = (LightingResult)0;

        // Skip lights that are not enabled.
        if ( !light.Enabled ) continue;
        // Skip point and spot lights that are out of range of the point being shaded.
        if ( light.Type != DIRECTIONAL_LIGHT && length( light.PositionVS - P ) > light.Range ) continue;

        switch ( light.Type )
        {
        case DIRECTIONAL_LIGHT:
        {
            result = DoDirectionalLight( light, mat, V, P, N );
        }
        break;
        case POINT_LIGHT:
        {
            result = DoPointLight( light, mat, V, P, N );
        }
        break;
        case SPOT_LIGHT:
        {
            result = DoSpotLight( light, mat, V, P, N );
        }
        break;
        }
        lit.Diffuse += result.Diffuse;
        lit.Specular += result.Specular;
    }
    
    diffuse *= float4( lit.Diffuse.rgb, 1.0f ); // Discard the alpha value from the lighting calculations.

    float4 specular = 0;
    if ( mat.SpecularPower > 1.0f ) // If specular power is too low, don't use it.
    {
        specular = mat.SpecularColor;
        if ( mat.HasSpecularTexture )
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
        specular *= lit.Specular;
    }

    return float4( ( ambient + emissive + diffuse + specular ).rgb, alpha * mat.Opacity );

}
