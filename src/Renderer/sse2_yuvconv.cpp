/* Videoplayer_Plugin - for licensing and copyright see license.txt */

/*
* Copyright (c) 2011, Nils L. Corneliusen
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
* NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
* OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
* OR OTHER DEALINGS IN THE SOFTWARE.
*
* This work is licensed under a Creative Commons Attribution 3.0 Unported License.
* http://creativecommons.org/licenses/by/3.0/
*
* You are free:
*   to Share — to copy, distribute and transmit the work
*   to Remix — to adapt the work
*   to make commercial use of the work
*
* Under the following conditions:
*   Attribution — You must attribute the work in the manner specified by the author or licensor
*   (but not in any way that suggests that they endorse you or your use of the work)
*/

#ifdef _DEBUG
#define _ITERATOR_DEBUG_LEVEL 0
#endif

#include "stdint.h"
#include "emmintrin.h"
#include "mmintrin.h"

#include <Renderer/CVideoRenderer.h>

namespace VideoplayerPlugin
{
    typedef __m128i& rtd;

    inline void calcRows(
        rtd y00, rtd y01, // all
        rtd rv00, rtd rv01, // r
        rtd gu00, rtd gv00, rtd gu01, rtd gv01, // g
        rtd bu00, rtd bu01, // b
        rtd r00, rtd r01, rtd g00, rtd g01, rtd b00, rtd b01 // result
    )
    {
        r00 = _mm_srai_epi16( _mm_add_epi16( y00, rv00 ), 6 );
        r01 = _mm_srai_epi16( _mm_add_epi16( y01, rv01 ), 6 );
        g00 = _mm_srai_epi16( _mm_sub_epi16( _mm_sub_epi16( y00, gu00 ), gv00 ), 6 );
        g01 = _mm_srai_epi16( _mm_sub_epi16( _mm_sub_epi16( y01, gu01 ), gv01 ), 6 );
        b00 = _mm_srai_epi16( _mm_add_epi16( y00, bu00 ), 6 );
        b01 = _mm_srai_epi16( _mm_add_epi16( y01, bu01 ), 6 );
    }

#undef PARAMS
#define PARAMS \
    rtd r, \
    rtd g, \
    rtd b, \
    rtd a00, \
    SAlphaGenParam& ap

    template<eAlphaMode ALPHAMODE>
    inline void calcAlpha( PARAMS )
    {}

#undef PARAMS
#define PARAMS \
    rtd r, \
    rtd g, \
    rtd b, \
    rtd a, \
    rtd rgba1, rtd rgba2, \
    rtd ggbb, rtd gbgb

    // Saturating and pack the results into chunky pixels efficiently.
    template<eByteOrder COLOR_DST_FMT>
    inline void packResult( PARAMS )
    {}

    template<>
    inline void packResult<VBO_BGRA>( PARAMS )
    {
        // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
        // Interleaves the lower 8 signed or unsigned 8-bit integers in a with the lower 8 signed or unsigned 8-bit integers in b.
        r = _mm_unpacklo_epi8( _mm_packus_epi16( r, a ), a ); // arar.. saturated
        // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
        ggbb = _mm_packus_epi16( g, b );  // ggggggggbbbbbbbb saturated
        // Shifts the 128-bit value in a right by imm bytes while shifting in zeros. imm must be an immediate.
        // Interleaves the lower 8 signed or unsigned 8-bit integers in a with the lower 8 signed or unsigned 8-bit integers in b.
        gbgb = _mm_unpacklo_epi8( _mm_srli_si128( ggbb, 8 ), ggbb ); // gbgbgbgbgbgbgbgb
        // Interleaves the lower 4 signed or unsigned 16-bit integers in a with the lower 4 signed or unsigned 16-bit integers in b.
        rgba1 = _mm_unpacklo_epi16( gbgb, r ); // argbargbargbargb (low)
        // Interleaves the upper 4 signed or unsigned 16-bit integers in a with the upper 4 signed or unsigned 16-bit integers in b.
        rgba2 = _mm_unpackhi_epi16( gbgb, r );  // argbargbargbargb (high)
    }

    template<>
    inline void packResult<VBO_RGBA>( PARAMS )
    {
        // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
        // Interleaves the lower 8 signed or unsigned 8-bit integers in a with the lower 8 signed or unsigned 8-bit integers in b.
        b = _mm_unpacklo_epi8( _mm_packus_epi16( b, a ), a ); // abab.. saturated
        // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
        ggbb = _mm_packus_epi16( g, r );  // ggggggggrrrrrrrr saturated
        // Shifts the 128-bit value in a right by imm bytes while shifting in zeros. imm must be an immediate.
        // Interleaves the lower 8 signed or unsigned 8-bit integers in a with the lower 8 signed or unsigned 8-bit integers in b.
        gbgb = _mm_unpacklo_epi8( _mm_srli_si128( ggbb, 8 ), ggbb ); // grgr ....
        // Interleaves the lower 4 signed or unsigned 16-bit integers in a with the lower 4 signed or unsigned 16-bit integers in b.
        rgba1 = _mm_unpacklo_epi16( gbgb, b ); // abgr ... (low)
        // Interleaves the upper 4 signed or unsigned 16-bit integers in a with the upper 4 signed or unsigned 16-bit integers in b.
        rgba2 = _mm_unpackhi_epi16( gbgb, b );  // abgr ... (high)
    }

    // orginal from yuv420_to_rgba8888 : http://www.ignorantus.com/yuv2rgb_sse2/index.html
#undef PARAMS
#define PARAMS uint8_t *yp, uint8_t *up, uint8_t *vp, \
    uint32_t sy, uint32_t suv, \
    int width, int height, \
    uint32_t *rgb, uint32_t srgb, SAlphaGenParam& ap

    template<eByteOrder COLOR_DST_FMT, eAlphaMode ALPHAMODE>
    inline void SSE2_YUV420_2_( PARAMS )
    {
        __m128i y0r0, y0r1, u0, v0;
        __m128i y00r0, y01r0, y00r1, y01r1;
        __m128i u00, u01, v00, v01;
        __m128i rv00, rv01, gu00, gu01, gv00, gv01, bu00, bu01;
        __m128i r00, r01, g00, g01, b00, b01, a00, a01;
        __m128i rgb0123, rgb4567, rgb89ab, rgbcdef;
        __m128i ggbb, gbgb;
        __m128i ysub, uvsub;
        __m128i setall, zero, facy, facrv, facgu, facgv, facbu;
        __m128i* srcy128r0, *srcy128r1;
        __m128i* dstrgb128r0, *dstrgb128r1;
        __m64* srcu64, *srcv64;
        int x, y;

        // Some necessary constants first
        ysub = _mm_set1_epi32( 0x00100010 );
        uvsub = _mm_set1_epi32( 0x00800080 );

        facy  = _mm_set1_epi32( 0x004a004a );
        facrv = _mm_set1_epi32( 0x00660066 );
        facgu = _mm_set1_epi32( 0x00190019 );
        facgv = _mm_set1_epi32( 0x00340034 );
        facbu = _mm_set1_epi32( 0x00810081 );

        zero  = _mm_set1_epi32( 0x00000000 );
        setall = _mm_set1_epi32( 0xFFFFFFFF );
        a00 = setall; // standard fill mode
        a01 = setall;

        for ( y = 0; y < height; y += 2 )
        {
            srcy128r0 = ( __m128i* )( yp + sy * y );
            srcy128r1 = ( __m128i* )( yp + sy * y + sy );
            srcu64 = ( __m64* )( up + suv * ( y / 2 ) );
            srcv64 = ( __m64* )( vp + suv * ( y / 2 ) );

            dstrgb128r0 = ( __m128i* )( rgb + srgb * y );
            dstrgb128r1 = ( __m128i* )( rgb + srgb * y + srgb );

            for ( x = 0; x < width; x += 16 )
            {
                // We start off by loading 8 bytes of data from u and v, and 16 from the two y rows. So we're processing 32 pixels at a time.
                u0 = _mm_loadl_epi64( ( __m128i* )srcu64 );
                srcu64++;
                v0 = _mm_loadl_epi64( ( __m128i* )srcv64 );
                srcv64++;

                y0r0 = _mm_load_si128( srcy128r0++ );
                y0r1 = _mm_load_si128( srcy128r1++ );

                // Next, we expand to 16 bit, calculate the constant y factors, and subtract 16:

                // constant y factors
                y00r0 = _mm_mullo_epi16( _mm_sub_epi16( _mm_unpacklo_epi8( y0r0, zero ), ysub ), facy );
                y01r0 = _mm_mullo_epi16( _mm_sub_epi16( _mm_unpackhi_epi8( y0r0, zero ), ysub ), facy );
                y00r1 = _mm_mullo_epi16( _mm_sub_epi16( _mm_unpacklo_epi8( y0r1, zero ), ysub ), facy );
                y01r1 = _mm_mullo_epi16( _mm_sub_epi16( _mm_unpackhi_epi8( y0r1, zero ), ysub ), facy );

                // Then it's time to prepare the uv factors that are common on both rows.
                // Since using SSE2 multipliers is cheap, I expand the u and v data first and use 8 mullo instead of the obvious 4.
                // Expanding afterwards is costly. I tried it.

                // yuv420: expand u and v so they're aligned with y values
                u0  = _mm_unpacklo_epi8( u0, zero );
                u00 = _mm_sub_epi16( _mm_unpacklo_epi16( u0, u0 ), uvsub );
                u01 = _mm_sub_epi16( _mm_unpackhi_epi16( u0, u0 ), uvsub );

                v0  = _mm_unpacklo_epi8( v0, zero );
                v00 = _mm_sub_epi16( _mm_unpacklo_epi16( v0, v0 ), uvsub );
                v01 = _mm_sub_epi16( _mm_unpackhi_epi16( v0, v0 ), uvsub );

                // common factors on both rows.
                rv00 = _mm_mullo_epi16( facrv, v00 );
                rv01 = _mm_mullo_epi16( facrv, v01 );
                gu00 = _mm_mullo_epi16( facgu, u00 );
                gu01 = _mm_mullo_epi16( facgu, u01 );
                gv00 = _mm_mullo_epi16( facgv, v00 );
                gv01 = _mm_mullo_epi16( facgv, v01 );
                bu00 = _mm_mullo_epi16( facbu, u00 );
                bu01 = _mm_mullo_epi16( facbu, u01 );

                // row 0

                // Now it's trivial to calculate the r/g/b planar values by summing things together as specified and shifting down by 6,
                // the multiplier we used on the factors.
                calcRows(
                    y00r0, y01r0, //all
                    rv00,   rv01, // r
                    gu00, gv00, gu01, gv01, // g
                    bu00, bu01, // b
                    r00, r01, g00, g01, b00, b01 // result
                );

                // calculate alpha
                calcAlpha<ALPHAMODE>(
                    r00, g00, b00,
                    a00,
                    ap
                );

                calcAlpha<ALPHAMODE>(
                    r01, g01, b01,
                    a01,
                    ap
                );

                // The remaining challenge is saturating and packing the results into chunky pixels efficiently.
                packResult<COLOR_DST_FMT>(
                    r00, // r
                    g00, // g
                    b00, // b
                    a00, // alpha
                    rgb0123, rgb4567, // result
                    ggbb, gbgb // temp
                );

                packResult<COLOR_DST_FMT>(
                    r01, // r
                    g01, // g
                    b01, // b
                    a01, // alpha
                    rgb89ab, rgbcdef, // result
                    ggbb, gbgb // temp
                );

                // Store the finished 16 pixels
                _mm_store_si128( dstrgb128r0++, rgb0123 );
                _mm_store_si128( dstrgb128r0++, rgb4567 );
                _mm_store_si128( dstrgb128r0++, rgb89ab );
                _mm_store_si128( dstrgb128r0++, rgbcdef );
                // That concludes the work necessary for row 0.

                // Repeat the last steps for row 1, replacing the y values and target pointer, and we're done.
                // row 1

                // Now it's trivial to calculate the r/g/b planar values by summing things together as specified and shifting down by 6,
                // the multiplier we used on the factors.
                calcRows(
                    y00r1, y01r1, //all
                    rv00,   rv01, // r
                    gu00, gv00, gu01, gv01, // g
                    bu00, bu01, // b
                    r00, r01, g00, g01, b00, b01 // result
                );

                // calculate alpha
                calcAlpha<ALPHAMODE>(
                    r00, g00, b00,
                    a00,
                    ap
                );

                calcAlpha<ALPHAMODE>(
                    r01, g01, b01,
                    a01,
                    ap
                );

                // The remaining challenge is saturating and packing the results into chunky pixels efficiently.
                packResult<COLOR_DST_FMT>(
                    r00, // r
                    g00, // g
                    b00, // b
                    a00, // alpha
                    rgb0123, rgb4567, // result
                    ggbb, gbgb // temp
                );

                packResult<COLOR_DST_FMT>(
                    r01, // r
                    g01, // g
                    b01, // b
                    a01, // alpha
                    rgb89ab, rgbcdef, // result
                    ggbb, gbgb // temp
                );

                // Store the finished 16 pixels for row 1
                _mm_store_si128( dstrgb128r1++, rgb0123 );
                _mm_store_si128( dstrgb128r1++, rgb4567 );
                _mm_store_si128( dstrgb128r1++, rgb89ab );
                _mm_store_si128( dstrgb128r1++, rgbcdef );
                // That concludes the work necessary for row 1.
            }
        }
    }

    void SSE2_YUV420_2_dummy()
    {
        // force the compiler to include these template variations
        SAlphaGenParam d;
        SSE2_YUV420_2_<VBO_RGBA, VAM_FILL>( 0, 0, 0, 0, 0, 0, 0, 0, 0, d );
        SSE2_YUV420_2_<VBO_RGBA, VAM_PASSTROUGH>( 0, 0, 0,   0, 0, 0, 0, 0, 0, d );
        SSE2_YUV420_2_<VBO_RGBA, VAM_FALLOF>( 0, 0, 0,   0, 0, 0, 0, 0, 0, d );
        SSE2_YUV420_2_<VBO_RGBA, VAM_COLORMASK>( 0, 0, 0,   0, 0, 0, 0, 0, 0, d );

        SSE2_YUV420_2_<VBO_BGRA, VAM_FILL>( 0, 0, 0, 0, 0, 0, 0, 0, 0, d );
        SSE2_YUV420_2_<VBO_BGRA, VAM_PASSTROUGH>( 0, 0, 0,   0, 0, 0, 0, 0, 0, d );
        SSE2_YUV420_2_<VBO_BGRA, VAM_FALLOF>( 0, 0, 0,   0, 0, 0, 0, 0, 0, d );
        SSE2_YUV420_2_<VBO_BGRA, VAM_COLORMASK>( 0, 0, 0,   0, 0, 0, 0, 0, 0, d );
    }

}
