#pragma once

#include <cstdint>

// Pixel formats for plotting and fetching pixels from a texture.

// 4-component pixels
template<typename T>
struct Pixel4
{
    T R;
    T G;
    T B;
    T A;
};

// 32-bit types
typedef Pixel4<float> Pixel4f32;
typedef Pixel4<uint32_t> Pixel4u32;
typedef Pixel4<int32_t> Pixel4i32;

// 16-bit types
// TODO: 16-bit floating point types?
typedef Pixel4<uint16_t> Pixel4u16;
typedef Pixel4<int16_t> Pixel4i16;

// 8-bit types
typedef Pixel4<uint8_t> Pixel4u8;
typedef Pixel4<int8_t> Pixel4i8;

// 3-component pixels
template<typename T>
struct Pixel3
{
    T R;
    T G;
    T B;
};

// 32-bit types
typedef Pixel3<float> Pixel3f32;
typedef Pixel3<uint32_t> Pixel3u32;
typedef Pixel3<int32_t> Pixel3i32;

// 16-bit types
typedef Pixel3<uint16_t> Pixel3u16;
typedef Pixel3<int16_t> Pixel3i16;

// 8-bit types
typedef Pixel3<uint8_t> Pixel3u8;
typedef Pixel3<int8_t> Pixel3i8;

// 2-component pixels
template< typename T >
struct Pixel2
{
    T R;
    T G;
};

// 32-bit types
typedef Pixel2<float> Pixel2f32;
typedef Pixel2<uint32_t> Pixel2u32;
typedef Pixel2<int32_t> Pixel2i32;

// 16-bit types
typedef Pixel2<uint16_t> Pixel2u16;
typedef Pixel2<int16_t> Pixel2i16;

// 8-bit types
typedef Pixel2<uint8_t> Pixel2u8;
typedef Pixel2<int8_t> Pixel2i8;

// 1-component pixels
template< typename T >
struct Pixel1
{
    T R;
};

// 32-bit types
typedef Pixel1<float> Pixel1f32;
typedef Pixel1<uint32_t> Pixel1u32;
typedef Pixel1<int32_t> Pixel1i32;

// 16-bit types
typedef Pixel1<uint16_t> Pixel1u16;
typedef Pixel1<int16_t> Pixel1i16;

// 8-bit types
typedef Pixel1<uint8_t> Pixel1u8;
typedef Pixel1<int8_t> Pixel1i8;
