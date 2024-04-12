#pragma once

#include <cstdint>

/**
 * Flags to specify which value should be cleared.
 */
enum class ClearFlags : uint8_t
{
    Color = 1 << 0,
    Depth = 1 << 1,
    Stencil = 1 << 2,
    DepthStencil = Depth | Stencil,
    All = Color | Depth | Stencil,
};
