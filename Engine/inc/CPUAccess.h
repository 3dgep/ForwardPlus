#pragma once

// CPU Access. Used for textures and Buffers
enum class CPUAccess
{
    None = 0,          // No CPU access to this texture is necessary.
    Read = ( 1 << 0 ), // CPU reads permitted.
    Write = ( 1 << 1 ), // CPU writes permitted.
    ReadWrite = Read | Write
};

