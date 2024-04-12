#pragma once

#include "Object.h"
#include "Rect.h"
#include "Viewport.h"

class RasterizerState : public Object
{
public:
    /**
     * FillMode determines how primitives are rendered.
     * <ul>
     *  <li>FillMode::Wireframe: Primitives are rendered as lines.</li>
     *  <li>FillMode::Solid: Primitives are rendered as solid objects.</li>
     * </ul>
     * DX11 does not distinguish between front-face and back-face fill modes. In this case,
     * only the front-face fill mode is considered.
     * OpenGL allows you to set the front and back face fill modes independently.
     * 
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/ff476131(v=vs.85).aspx
     * @see https://www.opengl.org/sdk/docs/man/html/glPolygonMode.xhtml
     */
    enum class FillMode
    {
        Wireframe,
        Solid
    };

    /**
     * CullMode controls which polygons are rendered based on the direction they are 
     * facing.
     * <ul>
     *  <li>None: Both back and front facing polygons are rendered (no culling is performed).
     *  <li>Front: Front-facing polygons are not rendered.
     *  <li>Back: Back-facing polygons are not rendered.
     *  <li>FrontAndBack: Both front and back-facing polygons are culled (OpenGL only)
     * </ul>
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/ff476108(v=vs.85).aspx
     * @see https://www.opengl.org/sdk/docs/man/html/glCullFace.xhtml
     */
    enum class CullMode
    {
        None,
        Front,
        Back,
        FrontAndBack    // OpenGL only
    };

    /**
     * FrontFace determines what polygons are considered "front facing" when applying clipping.
     * The order refers to the ordering of the vertices of a triangle when looking at the "front" 
     * of the primitive.
     * <ul>
     *  <li>Clockwise: Vertices of a triangle that are arranged in a clockwise winding order are considered "front facing".
     *  This is the default value for DirectX (and in left-handed coordinate systems in general).</li>
     *  <li>CounterClockwise: Vertices of a triangle that are arranged in a counter-clockwise winding order are considered "front facing".
     *  This is the default value for OpenGL (and in right-handed coordinate systems in general. This is also my personal preference).</li>
     * </ul>
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/ff476198(v=vs.85).aspx
     * @see https://www.opengl.org/sdk/docs/man/html/glFrontFace.xhtml
     */
    enum class FrontFace
    {
        Clockwise,
        CounterClockwise
    };

    virtual void SetFillMode( FillMode frontFace = FillMode::Solid, FillMode backFace = FillMode::Solid ) = 0;
    virtual void GetFillMode( FillMode& frontFace, FillMode& backFace ) const = 0;

    virtual void SetCullMode( CullMode cullMode = CullMode::Back ) = 0;
    virtual CullMode GetCullMode() const = 0;

    virtual void SetFrontFacing( FrontFace frontFace = FrontFace::CounterClockwise ) = 0;
    virtual FrontFace GetFrontFacing() const = 0;

    /**
     * The depth bias allows you to offset the computed depth value of the current pixel.
     * For example, you can use a depth bias to mitigate shadow acne when performing projected shadow mapping.
     * Depth bias is also useful when rendering decals as Z-fighting can be reduced.
     * 
     * For DirectX, the bias is computed like this:
     * <code>Bias = depthBias * r + slopeBias * Slope;</code>
     * Where r is the minimum representable value > 0 and Slope is the maximum slope of the 
     * horizontal and vertical slopes of the depth value of the current pixel.
     * OpenGL computes the depth bias like this:
     * <code>Bias = slopeBias * DZ + r * depthBias;</code>
     * Where r is the smallest value > 0 and DZ is the rate of change of the slope
     * at the current pixel.
     * These formulas are virtually identical but may produce slightly different results
     * depending on the format of the depth buffer.
     * The default values are 0, 0 which effectively disables depth bias.
     * 
     * The bias clamp value is applied to the computed depth bias
     * before it is used to offset the depth value of the current pixel.
     * Having a depth bias > 0 can help resolve shadow acne when using projected shadow mapping,
     * but it can also cause another undesirable effect called "peter panning" (when the
     * shadow seems to "jump off" of the shadow caster. The result is that the shadow caster appears to be
     * floating. To resolve "peter panning", the computed depth bias value can be clamped into a reasonable offset.
     * Before the bias is applied to the computed depth value, it is clamped by this value using the following formula:
     * <code>if ( BiasClamp > 0 )
     *          Bias = min( BiasClamp, Bias )
     *       else if ( DepthBiasClamp < 0 )
     *          Bias = max( BiasClamp, Bias )
     * </code>
     * Setting the bias clamp value to 0 effectively disables it.
     * Note: This function is supported in OpenGL as an extension EXT_polygon_offset_clamp.
     * 
     * The computed bias value is added to the depth value of the current pixel 
     * before depth testing is performed and before the depth value is written to the 
     * depth buffer.
     * The final depth value is calculated:
     * <code>z = z + Bias;</code>
     * So the final depth value will be increased or decreased in final screen space
     * based on the depthBias and the slopeBias.
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/cc308048(v=vs.85).aspx
     * @see https://www.opengl.org/sdk/docs/man/html/glPolygonOffset.xhtml
     * @see https://www.opengl.org/registry/specs/EXT/polygon_offset_clamp.txt
     */
    virtual void SetDepthBias( float depthBias = 0.0f, float slopeBias = 0.0f, float biasClamp = 0.0f ) = 0;
    virtual void GetDepthBias( float& depthBias, float& slopeBias, float& biasClamp ) const = 0;

    /**
     * Enable clipping based on Z-distance.
     * Default value is true.
     */
    virtual void SetDepthClipEnabled( bool depthClipEnabled = true ) = 0;
    virtual bool GetDepthClipEnabled() const = 0;

    /**
    * Specify an array of viewports to bind to the rasterizer stage.
    * The helper function SetViewport can be used to set the viewport at index 0.
    */
    virtual void SetViewport( const Viewport& viewport ) = 0;
    virtual void SetViewports( const std::vector<Viewport>& viewports ) = 0;
    virtual const std::vector<Viewport>& GetViewports() = 0;

    /**
    * Enable scissor-rectangle culling.
    * Default value is false.
    */
    virtual void SetScissorEnabled( bool scissorEnable = false ) = 0;
    virtual bool GetScissorEnabled() const = 0;

    /**
     * If scissor rectangle culling is enabled, then use the rectangles passed
     * in the rects array to perform scissor culling. One scissor rectangle 
     * is specified per viewport.
     * If the user sends more than GL_MAX_VIEWPORTS (for OpenGL) or ?? for DirectX
     * scissor rects then only the first GL_MAX_VIEWPORTS or ?? will be set.
     * Rectangle coordinates are in window coordinates.
     */
    virtual void SetScissorRect( const Rect& rect ) = 0;
    virtual void SetScissorRects( const std::vector<Rect>& rects ) = 0;
    virtual const std::vector<Rect>& GetScissorRects() const = 0;

    /**
     * Specifies whether to use the quadrilateral or alpha line anti-aliasing 
     * algorithm on multisample antialiasing (MSAA) render targets. Set to TRUE 
     * to use the quadrilateral line anti-aliasing algorithm and to FALSE to use 
     * the alpha line anti-aliasing algorithm.
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/hh404489(v=vs.85).aspx
     * @see https://www.opengl.org/sdk/docs/man/html/glEnable.xhtml at GL_MULTISAMPLE
     */
    virtual void SetMultisampleEnabled( bool multisampleEnabled = false ) = 0;
    virtual bool GetMultisampleEnabled() const = 0;

    /**
     * Specifies whether to enable line antialiasing; only applies if doing line 
     * drawing and MultisampleEnable is FALSE.
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/hh404489(v=vs.85).aspx
     * @see https://www.opengl.org/sdk/docs/man/html/glEnable.xhtml GL_LINE_SMOOTH
     */
    virtual void SetAntialiasedLineEnable( bool antialiasedLineEnable ) = 0;
    virtual bool GetAntialiasedLineEnable() const = 0;

    /**
     * The sample count that is forced while UAV rendering or rasterizing. 
     * Valid values are 0, 1, 2, 4, 8, and optionally 16. 0 indicates that the 
     * sample count is not forced.
     * At the time of this writing, I could not find an OpenGL equivalent.
     * @see https://msdn.microsoft.com/en-us/library/hh404489(v=vs.85).aspx
     */
    virtual void SetForcedSampleCount( uint8_t forcedSampleCount = 0 ) = 0;
    virtual uint8_t GetForcedSampleCount() = 0;

    /**
     * Enable conservative rasterization mode.
     * Currently only supported in DirectX 11.3 and 12.
     * @see https://msdn.microsoft.com/en-us/library/windows/desktop/dn903791(v=vs.85).aspx
     */
    virtual void SetConservativeRasterizationEnabled( bool conservativeRasterizationEnabled = false ) = 0;
    virtual bool GetConservativeRasterizationEnabled() const = 0;

};