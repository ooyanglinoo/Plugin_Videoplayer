/* Videoplayer_Plugin - for licensing and copyright see license.txt */

// This code used with permission from: http://www.ignorantus.com/yuv2rgb_sse2/
// The following things changed:
// - Don't repeat yourself
// - Calculation of loop offsets
// - Byte reorder support
// - Alpha channel support
// - Inline function templates with partial specializations to
//   avoid functions calls and runtime conditionals and making a macro out of the whole thing.

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
    typedef __m128i*& rtdp;

    inline void calcRows(
        rtd y00, rtd y01, // all
        rtd rv00, rtd rv01, // r
        rtd gu00, rtd gv00, rtd gu01, rtd gv01, // g
        rtd bu00, rtd bu01, // b
        rtd r00, rtd r01, rtd g00, rtd g01, rtd b00, rtd b01 // result
    )
    {
        // Shifts the 8 signed 16-bit integers in a right by count bits while shifting in the sign bit.
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
    rtd a, \
    rtd oa, \
    SAlphaGenParam& ap

    template<eAlphaMode ALPHAMODE>
    inline void calcAlpha( PARAMS )
    {}

    template<>
    inline void calcAlpha<VAM_PASSTROUGH>( PARAMS )
    {
        //8px
        //a = oa;
        a = _mm_srai_epi16( oa, 6 );//_mm_sub_epi16( _mm_set1_epi32( 0x77777777 ), oa );
        //a = _mm_sub_epi8( _mm_set1_epi32( 0xFFFFFFFF ), oa );
    }

    enum eInterleaveMode
    {
        VIM_HIGH, //!< High
        VIM_LOW, //!< Low
    };

#undef PARAMS
#define PARAMS \
    rtd v0, \
    rtd v1, \
    rtd v2, \
    rtd v3, \
    rtd temp0, \
    rtd temp1, \
    rtd rgba0, rtd rgba1 \
     
    template<eInterleaveMode INTERLEAVE_MODE>
    inline void interleaveResult( PARAMS )
    {}

    template<>
    inline void interleaveResult<VIM_LOW>( PARAMS )
    {
        // Interleaves the lower 8 signed or unsigned 8-bit integers in a with the lower 8 signed or unsigned 8-bit integers in b.
        temp0 = _mm_unpacklo_epi8( v0, v3 ); // 3030..
        temp1 = _mm_unpacklo_epi8( v2, v1 ); // 1212

        // Interleaves the lower 4 signed or unsigned 16-bit integers in a with the lower 4 signed or unsigned 16-bit integers in b.
        rgba0 = _mm_unpacklo_epi16( temp0, temp1 );  // argbargb..
        // Interleaves the upper 4 signed or unsigned 16-bit integers in a with the upper 4 signed or unsigned 16-bit integers in b.
        rgba1 = _mm_unpackhi_epi16( temp0, temp1 );  // argbargb..
    }

    template<>
    inline void interleaveResult<VIM_HIGH>( PARAMS )
    {
        // Interleaves the upper 8 signed or unsigned 8-bit integers in a with the upper 8 signed or unsigned 8-bit integers in b.
        temp0 = _mm_unpackhi_epi8( v0, v3 ); // 3030
        temp1 = _mm_unpackhi_epi8( v2, v1 ); // 1212

        // Interleaves the lower 4 signed or unsigned 16-bit integers in a with the lower 4 signed or unsigned 16-bit integers in b.
        rgba0 = _mm_unpacklo_epi16( temp0, temp1 ); // argbargb..
        // Interleaves the upper 4 signed or unsigned 16-bit integers in a with the upper 4 signed or unsigned 16-bit integers in b.
        rgba1 = _mm_unpackhi_epi16( temp0, temp1 ); // argbargb..
    }

#undef PARAMS
#define PARAMS \
    rtd r, \
    rtd g, \
    rtd b, \
    rtd a, \
    rtd r1, \
    rtd g1, \
    rtd b1, \
    rtd a1 \
     
    template<eAlphaMode ALPHAMODE>
    inline void packRGBA( PARAMS )
    {
        // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
        r = _mm_packus_epi16( r, r1 );         // rrrr.. saturated
        g = _mm_packus_epi16( g, g1 );         // gggg.. saturated
        b = _mm_packus_epi16( b, b1 );         // bbbb.. saturated
        g1 = _mm_packus_epi16( a, a1 );         // aaaa.. saturated
    }

    template<>
    inline void packRGBA<VAM_FILL>( PARAMS )
    {
        // Packs the 16 signed 16-bit integers from a and b into 8-bit unsigned integers and saturates.
        r = _mm_packus_epi16( r, r1 );         // rrrr.. saturated
        g = _mm_packus_epi16( g, g1 );         // gggg.. saturated
        b = _mm_packus_epi16( b, b1 );         // bbbb.. saturated
        g1 = _mm_packus_epi16( a, a1 );         // aaaa.. saturated
    }

#undef PARAMS
#define PARAMS \
    rtd r, \
    rtd g, \
    rtd b, \
    rtd a, \
    rtd r1, \
    rtd g1, \
    rtd b1, \
    rtd a1, \
    rtd rgba0, rtd rgba1, \
    rtd rgba2, rtd rgba3 \
     
    template<eByteOrder COLOR_DST_FMT>
    inline void correctByteOrder( PARAMS )
    {};

    template<>
    inline void correctByteOrder<VBO_RGBA>( PARAMS )
    {
        interleaveResult<VIM_LOW>( r, g1, b, g, r1, b1, rgba0, rgba1 );
        interleaveResult<VIM_HIGH>( r, g1, b, g, r1, b1, rgba2, rgba3 );
    };

    template<>
    inline void correctByteOrder<VBO_BGRA>( PARAMS )
    {
        interleaveResult<VIM_LOW>( b, g1, r, g, r1, b1, rgba0, rgba1 );
        interleaveResult<VIM_HIGH>( b, g1, r, g, r1, b1, rgba2, rgba3 );
    };

    // Saturating and pack the results into chunky pixels efficiently.
    template<eByteOrder COLOR_DST_FMT, eAlphaMode ALPHAMODE>
    inline void packResult( PARAMS )
    {
        // g1 is used for alpha, because not needed anymore (and for constant alpha it would overwrite the constant if we would use a1)
        packRGBA<ALPHAMODE>( r, g, b, a, r1, g1, b1, a1 );

        // r1 and b1 is overwritten, because both are not needed anymore for this row and can thus be reused
        correctByteOrder<COLOR_DST_FMT>( r, g, b, a, r1, g1, b1, a1, rgba0, rgba1, rgba2, rgba3 );
    };

#undef PARAMS
#define PARAMS \
    rtd y00r0, rtd y01r0, \
    rtd rv00, rtd rv01, \
    rtd gu00, rtd gv00, \
    rtd gu01, rtd gv01, \
    rtd bu00, rtd bu01, \
    rtd r00, rtd r01, \
    rtd g00, rtd g01, \
    rtd b00, rtd b01, \
    rtd a0r0, rtd a0r1, \
    rtd a00, rtd a01, \
    rtd rgb0123, rtd rgb4567, \
    rtd rgb89ab, rtd rgbcdef, \
    SAlphaGenParam& ap, \
    rtdp dstrgb128

    template<eByteOrder COLOR_DST_FMT, eAlphaMode ALPHAMODE>
    inline void processRow( PARAMS )
    {
        // calculate 16 pixel in this row

        // Now it's trivial to calculate the r/g/b planar values by summing things together as specified and shifting down by 6,
        // the multiplier we used on the factors.
        calcRows(
            y00r0, y01r0, //all
            rv00, rv01, // r
            gu00, gv00, gu01, gv01, // g
            bu00, bu01, // b
            r00, r01, g00, g01, b00, b01 // result
        );

        // calculate alpha
        //calcAlpha<ALPHAMODE>( r00, g00, b00, a00, y00r0 /*a0r0*/, ap );
        //calcAlpha<ALPHAMODE>( r01, g01, b01, a01, y01r0 /*a0r0*/, ap );
        //r00 = g00 = b00 = a00;
        //r01 = g01 = b01 = a01;

        calcAlpha<ALPHAMODE>( r00, g00, b00, a00, a0r0, ap );
        calcAlpha<ALPHAMODE>( r01, g01, b01, a01, a0r1, ap );

        // The remaining challenge is saturating and packing the results into chunky pixels efficiently.
        packResult<COLOR_DST_FMT, ALPHAMODE>(
            r00, g00, b00, a00, // next 8 pixel
            r01, g01, b01, a01, // next 8 pixel
            rgb0123, rgb4567, rgb89ab, rgbcdef // result 16 pixel
        );

        // Store the finished 16 pixels
        _mm_store_si128( dstrgb128++, rgb0123 );
        _mm_store_si128( dstrgb128++, rgb4567 );
        _mm_store_si128( dstrgb128++, rgb89ab );
        _mm_store_si128( dstrgb128++, rgbcdef );
    }

#undef PARAMS
#define PARAMS \
    rtdp srca128r0, \
    rtdp srca128r1, \
    uint32_t offsetSrcA

    template<eAlphaMode ALPHAMODE>
    inline void nextAlpha( PARAMS )
    {}

    template<>
    inline void nextAlpha<VAM_PASSTROUGH>( PARAMS )
    {
        srca128r0 += offsetSrcA;
        srca128r1 += offsetSrcA;
    }

#undef PARAMS
#define PARAMS \
    rtdp srca128r0, \
    rtdp srca128r1, \
    rtd a0r0, \
    rtd a0r1, \
    SAlphaGenParam& ap

    template<eAlphaMode ALPHAMODE>
    inline void loadAlpha( PARAMS )
    {}

    template<>
    inline void loadAlpha<VAM_PASSTROUGH>( PARAMS )
    {
        a0r0 = _mm_load_si128( srca128r0++ );
        a0r1 = _mm_load_si128( srca128r1++ );
    }

#undef PARAMS
#define PARAMS uint8_t *yp, uint8_t *up, uint8_t *vp, uint8_t *yap, \
    uint32_t sy, uint32_t suv, uint32_t sa, \
    int width, int height, \
    uint32_t *rgb, uint32_t srgb, SAlphaGenParam& ap

    template<eByteOrder COLOR_DST_FMT, eAlphaMode ALPHAMODE>
    inline void SSE2_YUV420_2_( PARAMS )
    {
        __m128i y0r0, y0r1, u0, v0;
        __m128i a0r0, a0r1;
        __m128i y00r0, y01r0, y00r1, y01r1;
        __m128i u00, u01, v00, v01;
        __m128i rv00, rv01, gu00, gu01, gv00, gv01, bu00, bu01;
        __m128i r00, r01, g00, g01, b00, b01, a00, a01;
        __m128i rgb0123, rgb4567, rgb89ab, rgbcdef;
        __m128i ysub, uvsub;
        __m128i setall, zero, facy, facrv, facgu, facgv, facbu;
        __m128i* srcy128r0, *srcy128r1;
        __m128i* srca128r0, *srca128r1;
        __m128i* dstrgb128r0, *dstrgb128r1;
        __m64* srcu64, *srcv64;

        // Some necessary constants first
        ysub = _mm_set1_epi32( 0x00100010 );
        uvsub = _mm_set1_epi32( 0x00800080 );

        facy  = _mm_set1_epi32( 0x004a004a );
        facrv = _mm_set1_epi32( 0x00660066 );
        facgu = _mm_set1_epi32( 0x00190019 );
        facgv = _mm_set1_epi32( 0x00340034 );
        facbu = _mm_set1_epi32( 0x00810081 );

        zero  = _mm_set1_epi32( 0x00000000 );
        setall = _mm_set1_epi32( 0x0F0F0F0F );
        a00 = setall; // standard fill mode
        a01 = setall;

        // Precalculate offsets;
        uint32_t offsetSrcY = ( sy - width ) + sy;
        uint32_t offsetSrcUV = suv - width / 2;
        uint32_t offsetSrcA = ( sa - width ) + sa;
        uint32_t offsetDstRGB = ( srgb - width ) + srgb;
        // offsetSrcY  // 1 byte luminance
        offsetDstRGB *= 4; // 4 byte RGBA
        offsetSrcY /= sizeof( __m128i );
        offsetSrcA /= sizeof( __m128i );
        offsetSrcUV /= sizeof( __m64 );
        offsetDstRGB /= sizeof( __m128i );

        uint32_t offsetRowEnd = width / sizeof( __m128i );

        // Calculate Start
        srcy128r0 = ( __m128i* )( yp );
        srcy128r1 = ( __m128i* )( yp + sy );
        srca128r0 = ( __m128i* )( yap );
        srca128r1 = ( __m128i* )( yap + sa );
        srcu64 = ( __m64* )( up );
        srcv64 = ( __m64* )( vp );
        dstrgb128r0 = ( __m128i* )( rgb );
        dstrgb128r1 = ( __m128i* )( rgb + srgb );

        // Calculate End
        __m128i* endDstRGB = ( __m128i* )( rgb + srgb * height ); // Image End
        __m128i* endSrcY = srcy128r0 + offsetRowEnd; // Row End

        // Process the whole image
        while ( dstrgb128r0 != endDstRGB )
        {
            // Process 2*16 pixel each step (completes 2 rows)
            while ( srcy128r0 != endSrcY )
            {
                // We start off by loading 8 bytes of data from u and v, and 16 from the two y rows. So we're processing 32 pixels at a time.
                u0 = _mm_loadl_epi64( ( __m128i* )srcu64++ );
                v0 = _mm_loadl_epi64( ( __m128i* )srcv64++ );

                // Loads 128-bit value.
                y0r0 = _mm_load_si128( srcy128r0++ );
                y0r1 = _mm_load_si128( srcy128r1++ );

                // Load Alpha if required
                loadAlpha<ALPHAMODE>(
                    srca128r0, srca128r1,
                    a0r0, a0r1, ap
                );

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
                processRow<COLOR_DST_FMT, ALPHAMODE>( y00r0, y01r0, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, r00, r01, g00, g01, b00, b01, a0r0, a0r1, a00, a01, rgb0123, rgb4567, rgb89ab, rgbcdef, ap, dstrgb128r0 );

                // row 1
                processRow<COLOR_DST_FMT, ALPHAMODE>( y00r1, y01r1, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, r00, r01, g00, g01, b00, b01, a0r0, a0r1, a00, a01, rgb0123, rgb4567, rgb89ab, rgbcdef, ap, dstrgb128r1 );
            }

            // goto next row
            srcy128r0 += offsetSrcY;
            srcy128r1 += offsetSrcY;
            nextAlpha<ALPHAMODE>( srca128r0, srca128r1, offsetSrcA );
            srcu64 += offsetSrcUV;
            srcv64 += offsetSrcUV;
            dstrgb128r0 += offsetDstRGB;
            dstrgb128r1 += offsetDstRGB;

            // refresh row end
            endSrcY = srcy128r0 + offsetRowEnd;
        }
    }

    void SSE2_YUV420_2_dummy()
    {
        // force the compiler to include these template variations
        SAlphaGenParam d;
#define PARAMS 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, d

        SSE2_YUV420_2_<VBO_RGBA, VAM_FILL>( PARAMS );
        SSE2_YUV420_2_<VBO_RGBA, VAM_PASSTROUGH>( PARAMS );
        SSE2_YUV420_2_<VBO_RGBA, VAM_FALLOF>( PARAMS );
        SSE2_YUV420_2_<VBO_RGBA, VAM_COLORMASK>( PARAMS );

        SSE2_YUV420_2_<VBO_BGRA, VAM_FILL>( PARAMS );
        SSE2_YUV420_2_<VBO_BGRA, VAM_PASSTROUGH>( PARAMS );
        SSE2_YUV420_2_<VBO_BGRA, VAM_FALLOF>( PARAMS );
        SSE2_YUV420_2_<VBO_BGRA, VAM_COLORMASK>( PARAMS );
    }

}
