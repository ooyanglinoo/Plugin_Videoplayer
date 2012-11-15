/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Renderer/CVideoRendererCE3.h>
#include <WebM/vpxdec_ext.h>
#include <CPluginVideoplayer.h>

// LockRect is faster because one copy operation less then without it
// (because of pitch=0 when using UpdateTextureInVideoMemory an internal copy has to be made)
// dont use in combination with FT_USAGE_DYNAMIC or it'll lock up your system.
//#define USE_LOCK_RECT //DX9 works (but crashes sporadic when quitting), DX11 doesn't work

// As Fast as LockRect for DX9
// Under DX11 the CryGame freezes sometimes but its the only working solution
// (Problem probably multithreading?)
//
#define USE_MAP // DX9 works, DX11 works (But the texture format is different and alpha has to be set)

// Else
// Slowest Method
// UpdateTexture //DX9 works, DX11 doesn't work

namespace VideoplayerPlugin
{
    CVideoRendererCE3::CVideoRendererCE3()
    {
        m_pData = NULL;
        m_iTex = 0;

#if defined(_DEBUG)
        gPlugin->LogAlways( "Created CE3 VideoRenderer" );
#endif
    }

    CVideoRendererCE3::~CVideoRendererCE3()
    {
    }

    void CVideoRendererCE3::ReleaseResources()
    {
        CVideoRenderer::ReleaseResources();

        if ( m_iTex )
        {
            // TODO: maybe handle USE_LOCK_RECT special because there are crashes
            gEnv->pRenderer->RemoveTexture( m_iTex );
            m_iTex = 0;
        }
    }

    INT_PTR CVideoRendererCE3::GetRenderTarget( eRendererType eType )
    {
        switch ( eType )
        {
            case VRT_AUTO:
            case VRT_CE3:
                return INT_PTR( m_iTex ? gEnv->pRenderer->EF_GetTextureByID( m_iTex ) : NULL );
        }

        return NULL;
    }

    bool CVideoRendererCE3::CreateResources( unsigned nSourceWidth, unsigned nSourceHeight, unsigned nTargetWidth, unsigned nTargetHeight )
    {
        ReleaseResources();

        bool bMemSuccess = CVideoRenderer::CreateResources( nSourceWidth, nSourceHeight, nTargetWidth, nTargetHeight );

#if !defined(VP_DISABLE_RESOURCE)
#if defined(USE_LOCK_RECT)
        m_iTex = gEnv->pRenderer->SF_CreateTexture( m_nSourceWidth, m_nSourceHeight, 1, m_pData, eTF_X8R8G8B8, 0 | FT_FILTER_BILINEAR );
#else
        m_iTex = gEnv->pRenderer->SF_CreateTexture( m_nSourceWidth, m_nSourceHeight, 1, m_pData, eTF_X8R8G8B8, FT_USAGE_DYNAMIC | FT_FILTER_BILINEAR );
#endif
#endif
        return bMemSuccess && m_pData && m_iTex > 0;
    }

    void CVideoRendererCE3::RenderFrame( void* pData )
    {
        if ( pData && m_iTex > 0 )
        {
            // if no img then frame was dropped
            vpx_image_t* img = ( vpx_image_t* )pData;
            SAlphaGenParam ap;

#if defined(USE_SEPERATEMEMORY)
            YV12_2_TEX( img->planes[VPX_PLANE_Y], img->planes[VPX_PLANE_U], img->planes[VPX_PLANE_V], NULL, m_nSourceWidth, m_nSourceHeight, ( uint32_t* ) m_pData, m_nSourceWidth, img->stride[VPX_PLANE_Y], img->stride[VPX_PLANE_V], img->stride[VPX_PLANE_U], 0, ap );
            m_bDirty = true;
#elif defined(USE_LOCK_RECT)

            int nPitch;

            if ( tex )
            {
                uint8* pData = tex->LockData( nPitch, 0, 0 );

                if ( pData )
                {
                    nPitch >>= 2;
                    YV12_2_TEX( img->planes[VPX_PLANE_Y], img->planes[VPX_PLANE_U], img->planes[VPX_PLANE_V], NULL, m_nSourceWidth, m_nSourceHeight, ( uint32_t* ) pData, nPitch, img->stride[VPX_PLANE_Y], img->stride[VPX_PLANE_V], img->stride[VPX_PLANE_U], 0, ap );
                    tex->UnlockData( 0, 0 );
                }
            }

#elif defined(USE_MAP)

            uint32 nPitch = 0;
            void* pData = NULL;
            bool bRet = gEnv->pRenderer->SF_MapTexture( m_iTex, 0, pData, nPitch );

            if ( pData )
            {
                nPitch >>= 2;

                YV12_2_TEX( img->planes[VPX_PLANE_Y], img->planes[VPX_PLANE_U], img->planes[VPX_PLANE_V], NULL, m_nSourceWidth, m_nSourceHeight, ( uint32_t* ) pData, nPitch, img->stride[VPX_PLANE_Y], img->stride[VPX_PLANE_V], img->stride[VPX_PLANE_U], 0, ap );
            }

            else
            {
                gPlugin->LogError( "Could not map texture." );
            }

            bRet = gEnv->pRenderer->SF_UnmapTexture( m_iTex, 0 );
#else
            YV12_2_TEX( img->planes[VPX_PLANE_Y], img->planes[VPX_PLANE_U], img->planes[VPX_PLANE_V], NULL, m_nSourceWidth, m_nSourceHeight, ( uint32_t* ) m_pData, m_nSourceWidth, img->stride[VPX_PLANE_Y], img->stride[VPX_PLANE_V], img->stride[VPX_PLANE_U], 0, ap );
            gEnv->pRenderer->UpdateTextureInVideoMemory( m_iTex, m_pData, 0, 0, m_nSourceWidth, m_nSourceHeight, eTF_X8R8G8B8 );
#endif
        }
    }

    void CVideoRendererCE3::UpdateTexture()
    {
#if defined(USE_SEPERATEMEMORY)

        if ( m_pData && m_bDirty )
        {
            uint32 nDestPitch = 0;
            uint32 nSourcePitch = 0;
            void* pData = NULL;
            bool bRet = gEnv->pRenderer->SF_MapTexture( m_iTex, 0, pData, nDestPitch );

            if ( pData )
            {
                nSourcePitch = m_nSourceWidth << 2;
                copyPlane( nSourcePitch, m_nSourceHeight, ( unsigned char* )pData, nDestPitch, m_pData, nSourcePitch );
            }

            else
            {
                gPlugin->LogError( "Could not map texture." );
            }

            bRet = gEnv->pRenderer->SF_UnmapTexture( m_iTex, 0 );
            m_bDirty = false;
        }

#endif
    };

}
