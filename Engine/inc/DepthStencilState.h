#pragma once

#include "Object.h"

class DepthStencilState : public Object
{
public:

    /**
     * Used to enable or disable depth buffer writes.
     */
    enum class DepthWrite
    {
        Enable,
        Disable
    };

    /**
     * Compare function determines if a pixel is written to the back buffer.
     * This enumerator is used for both depth and stencil operations but
     * depth and stencil compare functions are set independently.
     * @see DepthStencilState::DepthMode::DepthFunction
     * @see DepthStencilState::StencilMode::StencilFunction
     */
    enum class CompareFunction
    {
        Never,          // Never pass the comparison operation.
        Less,           // Pass if the source pixel's depth or stencil reference value is less than the value stored in the depth/stencil buffer ( DSs < DSd ).
        Equal,          // Pass if the source pixel's depth or stencil reference value is equal to the value stored in the depth/stencil buffer ( DSs == DSd ).
        LessOrEqual,    // Pass if the source pixel's depth or stencil reference value is less than or equal to the value stored in the depth/stencil buffer ( DSs <= DSd ).
        Greater,        // Pass if the source pixel's depth or stencil reference value is greater than the value stored in the depth/stencil buffer ( DSs > DSd ).
        NotEqual,       // Pass if the source pixel's depth or stencil reference value is not equal to teh value stored in the depth/stencil buffer ( DSs != DSd ).
        GreaterOrEqual, // Pass if the source pixel's depth or stencil reference value is greater than or equal to the value stored in the depth/stencil buffer ( DSs >= DSd ).
        Always          // Always pass the comparison operation.
    };

    /**
     * The stencil operation determines what happens to the stencil buffer if
     * either a comparison function passes, or fails. You can specify the operation
     * depending on the result of the comparison function in the StencilMode structure.
     * @see DepthStencilState::StencilMode.
     */
    enum class StencilOperation
    {
        Keep,           // Keep the existing value in the stencil buffer unmodified.
        Zero,           // Set the value in the stencil buffer to 0.
        Reference,      // Set the value in the stencil buffer to the reference value. @see DepthStencilState::StencilMode::StencilReference.
        IncrementClamp, // Increment the value in the stencil buffer by 1 and clamp the result if it is out of range.
        DecrementClamp, // Decrement the value in the stencil buffer by 1 and clamp the result if it is out of range.
        Invert,         // Invert the value in the stencil value.
        IncrementWrap,  // Increment the value in the stencil buffer by 1 and wrap the result if it is out of range.
        DecrementWrap,  // Decrement the value in the stencil buffer by 1 and wrap the result if it is out of range.
    };

    struct DepthMode
    {
        /**
         * Set to true to enable depth testing.
         * If enabled, the DepthFunction will be used to check if a pixel should
         * be written to the back buffer or discarded.
         * Default: True
         * @see DepthStencilState::DepthMode::DepthFunction
         */
        bool DepthEnable;

        /**
         * Enable or disable writing to the depth buffer.
         * This should be enabled for opaque geometry and disabled
         * for partially transparent objects.
         * Default: Enabled.
         */
        DepthWrite DepthWriteMask;

        /**
         * If the depth comparison function evaluates to TRUE then the pixel
         * is rasterized. If the depth comparison function evaluates to FALSE then
         * the pixel is discarded and not written to the back buffer.
         * The default value is CompareFunc::Less which means that source pixels
         * that are closer to the camera will pass the compare function and source
         * pixels that appear further away from the camera are discarded.
         */
        CompareFunction DepthFunction;

        explicit DepthMode( bool depthEnable = true,
                            DepthWrite depthWrite = DepthWrite::Enable,
                            CompareFunction depthFunction = CompareFunction::Less )
            : DepthEnable( depthEnable )
            , DepthWriteMask( depthWrite )
            , DepthFunction( depthFunction )
        {}
    };

    /**
     * The operation to perform on the stencil buffer can be specified
     * depending on whether the primitive that is being tested is front-facing
     * or back facing (according to the value of RasterizerState::FrontFacing).
     */
    struct FaceOperation
    {
        /**
         * The operation to perform on the value in the stencil buffer if the 
         * stencil comparision function (specified by FaceOperation::StencilFunction) 
         * returns FALSE.
         * Default: StencilOperation::Keep
         */
        StencilOperation StencilFail;

        /**
         * The operation to perform on the value in the stencil buffer if the
         * stencil comparision function (specified by FaceOperation::StencilFunction)
         * returns TRUE and depth comparision function (determined by DepthMode::DepthFunction)
         * returns FALSE.
         * Default: StencilOperation::Keep
         */
        StencilOperation StencilPassDepthFail;

        /**
         * The operation to perform if both depth comparison function 
         * (determined by DepthMode::DepthFunction) and stencil comparision 
         * function (specified by FaceOperation::StencilFunction) returns TRUE.
         * Default: StencilOperation::Keep
         */
        StencilOperation StencilDepthPass;

        /**
         * The the comparison method to use for stencil operations.
         * Default: CompareFunction::Always (Tests always passes).
         * @see DepthStencilState::CompareFunction
         */
        CompareFunction StencilFunction;

        explicit FaceOperation( StencilOperation stencilFail = StencilOperation::Keep,
                                StencilOperation stencilPassDepthFail = StencilOperation::Keep,
                                StencilOperation stencilDepthPass = StencilOperation::Keep,
                                CompareFunction stencilFunction = CompareFunction::Always )
            : StencilFail( stencilFail )
            , StencilPassDepthFail( stencilPassDepthFail )
            , StencilDepthPass( stencilDepthPass )
            , StencilFunction( stencilFunction )
        {}
    };

    struct StencilMode
    {
        /**
         * Set to true to enable stencil testing.
         * Default: false.
         */
        bool StencilEnabled;

        /**
         * A mask that is AND'd to the value in the stencil buffer before it is read.
         * Default: 0xff
         */
        uint8_t ReadMask;

        /**
         * A mask that is AND'd to the value in the stencil buffer before it is written.
         * Default: 0xff
         */
        uint8_t WriteMask;

        /**
         * The value to set the stencil buffer to if any of the StencilOperation 
         * members of the FaceOperation struct is set to StencilOperation::Reference.
         * Default: 0
         */
        uint32_t StencilReference;

        /**
         * The compare function and pass/fail operations to perform on the stencil
         * buffer for front-facing polygons.
         */
        FaceOperation FrontFace;

        /**
         * The compare function and pass/fail operations to perform on the stencil 
         * buffer for back-facing polygons.
         */
        FaceOperation BackFace;

        StencilMode( bool stencilEnabled = false,
                     uint8_t readMask = 0xff,
                     uint8_t writeMask = 0xff, 
                     uint32_t stencilReference = 0,
                     FaceOperation frontFace = FaceOperation(),
                     FaceOperation backFace = FaceOperation() )
            : StencilEnabled( stencilEnabled )
            , ReadMask( readMask )
            , WriteMask( writeMask )
            , StencilReference( stencilReference )
            , FrontFace( frontFace )
            , BackFace( backFace )
        {}
    };

    virtual void SetDepthMode( const DepthMode& depthMode ) = 0;
    virtual const DepthMode& GetDepthMode() const = 0;

    virtual void SetStencilMode( const StencilMode& stencilMode ) = 0;
    virtual const StencilMode& GetStencilMode() const = 0;
};