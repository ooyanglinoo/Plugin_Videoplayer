/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <CPluginVideoplayer.h>
#include <Renderer/CVideoRenderer.h>
#include <windows.h>

#define USE_SSE2 // use fast software conversion if available

namespace VideoplayerPlugin
{
    unsigned char* copyPlane( unsigned int cols, unsigned int lines, unsigned char* dst, unsigned int dstStride, unsigned char* src, unsigned int srcStride )
    {
        if ( srcStride == dstStride )
        {
            // source and destination paddings equal
            memcpy( dst, src, srcStride * lines );
            dst += srcStride * lines;
        }

        else
        {
            // source and destination paddings need to be converted
            ++lines;

            while ( --lines )
            {
                memcpy( dst, src, cols );
                src += srcStride;
                dst += dstStride;
            }
        }

        return dst;
    }

    // create conditional parameter that resolves at compile time
#define PARAMS uint32_t* dst, unsigned char r, unsigned char g, unsigned char b, unsigned char a, SAlphaGenParam& ap
    template<eByteOrder COLOR_DST_FMT, eAlphaMode ALPHAMODE>
    inline void write_pixel( PARAMS )
    {};

    template<> inline void write_pixel<VBO_RGBA, VAM_PASSTROUGH>( PARAMS )
    {
        *dst = ( ( ( a ) << 24 ) | ( ( b ) << 16 ) | ( ( g ) << 8 ) | ( r ) ); // RGBA (little endian)
    }

    template<> inline void write_pixel<VBO_BGRA, VAM_PASSTROUGH>( PARAMS )
    {
        write_pixel<VBO_RGBA, VAM_PASSTROUGH>( dst, b, g, r, a, ap );
    }

    template<> inline void write_pixel<VBO_RGBA, VAM_FILL>( PARAMS )
    {
        *dst = ( 0xff000000 | ( ( b ) << 16 ) | ( ( g ) << 8 ) | ( r ) ); // RGBX (little endian)
    }

    template<> inline void write_pixel<VBO_BGRA, VAM_FILL>( PARAMS )
    {
        write_pixel<VBO_RGBA, VAM_FILL>( dst, b, g, r, a, ap );
    }

    template<> inline void write_pixel<VBO_RGBA, VAM_FALLOF>( PARAMS )
    {
        if ( a <= ap.tolerance )
        {
            a = 0;
        }

        else if ( a >= ap.fallof )
        {
            a = 255;
        }

        else
        {
            a = a * ap.mull;
        }

        write_pixel<VBO_RGBA, VAM_PASSTROUGH>( dst, r, g, b, a, ap );
    }

    template<> inline void write_pixel<VBO_BGRA, VAM_FALLOF>( PARAMS )
    {
        write_pixel<VBO_RGBA, VAM_FALLOF>( dst, b, g, r, a, ap );
    }

    template<> inline void write_pixel<VBO_RGBA, VAM_COLORMASK>( PARAMS )
    {
        ap.temp2 = abs( ap.r - r );
        ap.temp = ap.temp2 * ap.temp2 * ap.wr;
        ap.temp2 = abs( ap.g - g );
        ap.temp += ap.temp2 * ap.temp2 * ap.wg;
        ap.temp2 = abs( ap.b - b );
        ap.temp += ap.temp2 * ap.temp2 * ap.wb;
        ap.temp /= ap.ws;

        //ap.temp = (abs(ap.r - r) * ap.wr + abs(ap.g - g) * ap.wg + abs(ap.b - b) * ap.wb) / ap.ws;

        a = ap.temp;
        write_pixel<VBO_RGBA, VAM_FALLOF>( dst, r, g, b, a, ap );
    }

    template<> inline void write_pixel<VBO_BGRA, VAM_COLORMASK>( PARAMS )
    {
        write_pixel<VBO_RGBA, VAM_FALLOF>( dst, b, g, r, a, ap );
    }

#define SAT(x) CLAMP(x, 0, 255) // Saturate

    template<eByteOrder COLOR_DST_FMT, eAlphaMode ALPHAMODE>
    void YV12_2_( unsigned char* y, unsigned char* u, unsigned char* v, unsigned char* a, unsigned int cols, unsigned int lines, uint32_t* dst, unsigned int dstStride, unsigned int srcStrideY, unsigned int srcStrideU, unsigned int srcStrideV, unsigned int srcStrideA, SAlphaGenParam& ap )
    {
#ifdef USE_SSE2

        if ( ::IsProcessorFeaturePresent( PF_XMMI64_INSTRUCTIONS_AVAILABLE ) )
        {
            //SSE2 available
            SSE2_YUV420_2_<COLOR_DST_FMT, ALPHAMODE>( y, u, v, a, srcStrideY, srcStrideV, srcStrideA, cols, lines, dst, dstStride, ap );
            return;
        }

        //else slow fall back
#endif

        int Y;

        int CY;
        signed char CU;
        signed char CV;

        float fCY;
        float fCV;
        float fCUV;
        float fCU;

        float R;
        float G;
        float B;

        unsigned int lcols;
        uint32_t* ldst;
        unsigned char* ly;
        unsigned char* la;

        uint32_t* ldst2;
        unsigned char* ly2;
        unsigned char* la2;

        signed char* lu;
        signed char* lv;

        if ( !a )
        {
            // if alpha not available use luminance so I don't need another function
            a = y;
            srcStrideA = srcStrideY;
        }

        unsigned int srcStrideA2 = srcStrideA << 1;
        unsigned int srcStrideY2 = srcStrideY << 1;
        unsigned int dstStride2 = dstStride << 1;

        cols >>= 1;
        lines >>= 1;

        ++lines;

        while ( --lines )
        {
            ldst = dst;
            ly = y;
            la = a;

            ldst2 = dst + dstStride;
            ly2 = y   + srcStrideY;
            la2 = a   + srcStrideA;

            lu = ( signed char* )u;
            lv = ( signed char* )v;

            lcols = cols + 1;

            while ( --lcols )
            {
                // process 4 pixels each time

                // the UV values are valid for 4 pixel
                CU = ( *lu ) - 128;
                ++lu;
                CV = ( *lv ) - 128;
                ++lv;

                fCV = 1.596f * CV;
                fCUV = -0.391f * CU - 0.813f * CV;
                fCU = 2.018f * CU;

                // first line first col
                Y = *ly;
                ++ly;
                CY = Y - 16;
                fCY = 1.164f * CY;

                R = fCY + fCV + 0.5f;
                G = fCY + fCUV + 0.5f;
                B = fCY + fCU + 0.5f;

                write_pixel<COLOR_DST_FMT, ALPHAMODE>( ldst, SAT( R ), SAT( G ), SAT( B ), *la++, ap );
                ++ldst;

                // first line second col
                Y = *ly;
                ++ly;
                CY = Y - 16;
                fCY = 1.164f * CY;

                R = fCY + fCV + 0.5f;
                G = fCY + fCUV + 0.5f;
                B = fCY + fCU + 0.5f;

                write_pixel<COLOR_DST_FMT, ALPHAMODE>( ldst, SAT( R ), SAT( G ), SAT( B ), *la++, ap );
                ++ldst;

                // second line first col
                Y = *ly2;
                ++ly2;
                CY = Y - 16;
                fCY = 1.164f * CY;

                R = fCY + fCV + 0.5f;
                G = fCY + fCUV + 0.5f;
                B = fCY + fCU + 0.5f;

                write_pixel<COLOR_DST_FMT, ALPHAMODE>( ldst2,  SAT( R ), SAT( G ), SAT( B ), *la2++, ap );
                ++ldst2;

                // second line second col
                Y = *ly2;
                ++ly2;
                CY = Y - 16;
                fCY = 1.164f * CY;

                R = fCY + fCV + 0.5f;
                G = fCY + fCUV + 0.5f;
                B = fCY + fCU + 0.5f;

                write_pixel<COLOR_DST_FMT, ALPHAMODE>( ldst2, SAT( R ), SAT( G ), SAT( B ), *la2++, ap );
                ++ldst2;
            }

            y += srcStrideY2;
            a += srcStrideA2;

            dst += dstStride2;
            u += srcStrideU;
            v += srcStrideV;
        }
    }

    void YV12_2_TEX( unsigned char* y, unsigned char* u, unsigned char* v, unsigned char* a, unsigned int cols, unsigned int lines, uint32_t* dst, unsigned int dstStride, unsigned int srcStrideY, unsigned int srcStrideU, unsigned int srcStrideV, unsigned int srcStrideA, SAlphaGenParam& ap )
    {
        if ( gEnv->pRenderer->GetRenderType() == eRT_DX11 )
        {
            if ( a )
            {
                YV12_2_<VBO_RGBA, VAM_PASSTROUGH>( y, u, v, a, cols, lines, dst, dstStride, srcStrideY, srcStrideU, srcStrideV, srcStrideA, ap );
            }

            else
            {
                YV12_2_<VBO_RGBA, VAM_FILL>( y, u, v, a, cols, lines, dst, dstStride, srcStrideY, srcStrideU, srcStrideV, srcStrideA, ap );
            }
        }

        else
        {
            if ( a )
            {
                YV12_2_<VBO_BGRA, VAM_PASSTROUGH>( y, u, v, a, cols, lines, dst, dstStride, srcStrideY, srcStrideU, srcStrideV, srcStrideA, ap );
            }

            else
            {
                YV12_2_<VBO_BGRA, VAM_FILL>( y, u, v, a, cols, lines, dst, dstStride, srcStrideY, srcStrideU, srcStrideV, srcStrideA, ap );
            }
        }
    }
}

#include <Renderer/CVideoRendererDX9.h>
#include <Renderer/CVideoRendererDX11.h>
#include <Renderer/CVideoRendererCE3.h>

#include <CVideoplayerSystem.h>

#include <queue>
#include <map>
#include <concrt.h>

namespace VideoplayerPlugin
{
    void CVideoRenderer::Release()
    {
        if ( --m_nReferences < 0 )
        {
            m_nReferences = 0;
        }

        if ( !m_nReferences )
        {
            markVideoResourceForCleanup( this );
        }
    };

    void CVideoRenderer::Cleanup()
    {
        if ( !m_nReferences )
        {
            ReleaseResources();
            delete this;
        }

        else
        {
            gPlugin->LogWarning( "Cleanup called but references still exist." );
        }
    };

    std::queue<IVideoResource*> qVideoResourcesCleanup;

    typedef std::map<IVideoRenderer*, IVideoRenderer*> tVideoRendererMap;
    tVideoRendererMap mVideoRendererUpdate;

    Concurrency::critical_section csVideoResources;

    IVideoRenderer* createVideoRenderer( eRendererType eType )
    {
        Concurrency::critical_section::scoped_lock lock( csVideoResources );

        IVideoRenderer* pRet = NULL;

        if ( !gD3DSystem )
        {
            eType = VRT_CE3;
        }

        switch ( eType )
        {
            case VRT_AUTO:
                switch ( gD3DSystem->GetType() )
                {
                        //case D3D_DX11:
                        //  return new CVideoRendererDX11();
                    case D3DPlugin::D3D_DX9:
                        pRet = new CVideoRendererDX9();
                        goto finished;
                        // else use CE3
                }

            case VRT_CE3:
                pRet = new CVideoRendererCE3();
                goto finished;

            case VRT_DX9:
                pRet = gD3DSystem->GetType() == D3DPlugin::D3D_DX9 ? new CVideoRendererDX9() : NULL;
                goto finished;

            case VRT_DX11:
                pRet =  gD3DSystem->GetType() == D3DPlugin::D3D_DX11 ? new CVideoRendererDX11() : NULL;
                goto finished;
        }

finished:

        if ( pRet )
        {
            mVideoRendererUpdate[pRet] = pRet;
        }

        return pRet;
    };

    void markVideoResourceForCleanup( IVideoResource* res )
    {
        Concurrency::critical_section::scoped_lock lock( csVideoResources );
        qVideoResourcesCleanup.push( res );
        mVideoRendererUpdate.erase( ( IVideoRenderer* )res );
    };

    void cleanupVideoResources()
    {
        Concurrency::critical_section::scoped_lock lock( csVideoResources ) ;

        while ( !qVideoResourcesCleanup.empty() )
        {
            IVideoResource* item = qVideoResourcesCleanup.front();

            if ( item )
            {
                item->Cleanup();
            }

            qVideoResourcesCleanup.pop();
        }
    };

    // TODO: Keep watching http://code.google.com/p/webm/issues/detail?id=162
    void updateVideoResources( eRendererType nType )
    {
        Concurrency::critical_section::scoped_lock lock( csVideoResources );

        // TODO: 32 bit DX11 Version will crash if this takes longer then 2-3 ms ingame so disable this for now.
        if ( sizeof( void* ) == 4 && gEnv->pRenderer->GetRenderType() == eRT_DX11 && nType == VRT_CE3 && gVideoplayerSystem->GetScreenState() == eSS_InGameScreen )
        {
            return;
        }

#if !defined(VP_DISABLE_RENDER)

        for ( tVideoRendererMap::const_iterator iter = mVideoRendererUpdate.begin(); iter != mVideoRendererUpdate.end(); ++iter )
        {
            if ( iter->first->GetRendererType() == nType )
            {
                iter->first->UpdateTexture();
            }
        }

#endif
    };
}