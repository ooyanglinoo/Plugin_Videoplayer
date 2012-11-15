/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Renderer/CVideoRendererDX9.h>
#include <WebM/vpxdec_ext.h>
#include <CPluginVideoplayer.h>
#include <IPluginManager.h>
#include <IPluginD3D.h>

#ifdef _DEBUG
#include <DxErr.h>
#pragma comment( lib, "dxerr" )
#endif

// Did not improve performance
#define USE_DISCARD 0
//#define USE_DISCARD D3DLOCK_DISCARD

// Did not improve performance
// but reduces functionality (no scaling)
#undef USE_UPDATE_SURFACE
//#define USE_UPDATE_SURFACE 1

namespace VideoplayerPlugin
{

    void outputError( HRESULT hr )
    {
#ifdef _DEBUG
        gPlugin->LogError( "Error hr=%u %s error description: %s\n", ( unsigned )hr, DXGetErrorString( hr ), DXGetErrorDescription( hr ) );
#else
        gPlugin->LogError( "Error hr=%u", ( unsigned )hr );
#endif
    }

#define D3DFMT_YV12 (D3DFORMAT)(MAKEFOURCC('Y', 'V', '1', '2'))

    CVideoRendererDX9::CVideoRendererDX9()
    {
        m_pSurfaceYUV = NULL;
        m_pTex = NULL;
        m_pStagingSurface = NULL;

        m_pD3DDevice = static_cast<IDirect3DDevice9*>( gD3DSystem->GetDevice() );
        m_iTex = 0;
#if defined(_DEBUG)
        gPlugin->LogAlways( "Created DX9 VideoRenderer" );
#endif
    }

    CVideoRendererDX9::~CVideoRendererDX9()
    {
    }

    template<typename tResource>
    UINT safeRelease( tResource& pResource, bool bWarn = false )
    {
        UINT nRet = 0;

        if ( pResource )
        {
            nRet = pResource->Release();

            if ( nRet > 1 )
            {
                // normally 0 but it seems one is kept internally even after removetexture (probably released in a delayed way)
                gPlugin->LogWarning( "safeRelease references left %d on resource %p", nRet, pResource );
            }

            pResource = NULL;
        }

        return nRet;
    }

    void CVideoRendererDX9::ReleaseResources()
    {
        CVideoRenderer::ReleaseResources();

        if ( m_iTex )
        {
            gEnv->pRenderer->RemoveTexture( m_iTex );
            m_iTex = 0;
        }

        safeRelease( m_pTex, true );
        safeRelease( m_pSurfaceYUV, true  );
        safeRelease( m_pStagingSurface, true  );
    }

    bool CVideoRendererDX9::CreateResources( unsigned nSourceWidth, unsigned nSourceHeight, unsigned nTargetWidth, unsigned nTargetHeight )
    {
        ReleaseResources();

        bool bMemSuccess = CVideoRenderer::CreateResources( nSourceWidth, nSourceHeight, nTargetWidth, nTargetHeight );

        IDirect3D9* pD3D = NULL;

        if ( m_pD3DDevice )
        {
            m_pD3DDevice->GetDirect3D( &pD3D );
        }

#if !defined(VP_DISABLE_RESOURCE)

        if ( pD3D )
        {
            HRESULT hr = m_pD3DDevice->CreateTexture( nTargetWidth, nTargetHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pTex, NULL );

            // Directly locking this was doesn't perform at all and brought some problems so use a staging texture
            //HRESULT hr = m_pD3DDevice->CreateTexture((nSourceWidth >> RESBASE) << RESBASE, (nSourceHeight >> RESBASE) << RESBASE, 1, D3DUSAGE_DYNAMIC, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pTex, NULL);

            if ( FAILED( hr ) || !m_pTex )
            {
                // Could not create render target
                outputError( hr );
                m_pTex = NULL;
            }

            if ( m_pTex )
            {
#if !defined(USE_SEPERATEMEMORY)

                // YV12 format possible?
                if (    FAILED( pD3D->CheckDeviceFormatConversion( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_UNKNOWN, D3DFMT_X8R8G8B8 ) )
                        &&  SUCCEEDED( pD3D->CheckDeviceFormatConversion( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_YV12, D3DFMT_X8R8G8B8 ) ) )
                {
#if defined(_DEBUG)
                    gPlugin->LogAlways( "Creating YUV surface." );
#endif
                    hr = m_pD3DDevice->CreateOffscreenPlainSurface( nSourceWidth, nSourceHeight, D3DFMT_YV12, D3DPOOL_DEFAULT, &m_pSurfaceYUV, NULL );

                    if ( FAILED( hr ) || !m_pSurfaceYUV )
                    {
                        // Could not create YUV surface
                        outputError( hr );
                        m_pSurfaceYUV = NULL;
                    }
                }

#endif

                if ( !m_pSurfaceYUV )
                {
#if !defined(USE_SEPERATEMEMORY)
                    gPlugin->LogWarning( "Couldn't create YUV surface, switching to fallback." );
#endif

#if !defined(USE_UPDATE_SURFACE)
                    hr = m_pD3DDevice->CreateOffscreenPlainSurface( nSourceWidth, nSourceHeight, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pStagingSurface, NULL );
#else
                    hr = m_pD3DDevice->CreateOffscreenPlainSurface( nSourceWidth, nSourceHeight, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &m_pStagingSurface, NULL );
#endif

                    if ( FAILED( hr ) || !m_pStagingSurface )
                    {
                        // Could not create staging surface
                        outputError( hr );
                        m_pStagingSurface = NULL;
                    }
                }

                if ( m_pSurfaceYUV || m_pStagingSurface )
                {
                    ITexture* pTex = gD3DSystem->InjectTexture( m_pTex, nTargetWidth, nTargetHeight, eTF_A8R8G8B8, FT_USAGE_RENDERTARGET | FT_FILTER_BILINEAR );

                    if ( pTex )
                    {
                        m_iTex = pTex->GetTextureID();
                    }

                    else
                    {
                        gPlugin->LogError( "Couldn't inject texture" );
                    }
                }
            }
        }

#endif

        return bMemSuccess && m_pTex && m_iTex > 0 && ( m_pSurfaceYUV || m_pStagingSurface );
    }

    INT_PTR CVideoRendererDX9::GetRenderTarget( eRendererType eType )
    {
        switch ( eType )
        {
            case VRT_DX9:
                return INT_PTR( m_pTex );

            case VRT_AUTO:
            case VRT_CE3:
                return INT_PTR( m_iTex ? gEnv->pRenderer->EF_GetTextureByID( m_iTex ) : NULL );
        }

        return NULL;
    }

    void CVideoRendererDX9::RenderFrame( void* pData )
    {
        if ( pData )
        {
            // if no img then frame was dropped

#if defined(USE_SEPERATEMEMORY)
            vpx_image_t* img = ( vpx_image_t* )pData;
            SAlphaGenParam ap;
            YV12_2_TEX( img->planes[VPX_PLANE_Y], img->planes[VPX_PLANE_U], img->planes[VPX_PLANE_V], NULL, m_nSourceWidth, m_nSourceHeight, ( uint32_t* ) m_pData, m_nSourceWidth, img->stride[VPX_PLANE_Y], img->stride[VPX_PLANE_V], img->stride[VPX_PLANE_U], 0, ap );
            //YV12_2_TEX( img->planes[VPX_PLANE_Y], img->planes[VPX_PLANE_U], img->planes[VPX_PLANE_V], img->planes[VPX_PLANE_Y], m_nSourceWidth, m_nSourceHeight, ( uint32_t* ) m_pData, m_nSourceWidth, img->stride[VPX_PLANE_Y], img->stride[VPX_PLANE_V], img->stride[VPX_PLANE_U], img->stride[VPX_PLANE_Y], ap );
            m_bDirty = true;
#else
            D3DLOCKED_RECT LockedRect;
            memset( &LockedRect, 0, sizeof( LockedRect ) );

            if ( m_pTex )
            {
                // Rendertarget is present
                IDirect3DSurface9* surfaceTemp = NULL;
                HRESULT hr = m_pTex->GetSurfaceLevel( 0, &surfaceTemp );

                if ( SUCCEEDED( hr ) && surfaceTemp )
                {
                    if ( m_pSurfaceYUV )
                    {
                        // Hardware YUV Conversion:
                        // YV12 source -> copyPlane -> YV12 surface -> StretchRect -> X8R8G8B8 render target surface
                        hr = m_pSurfaceYUV->LockRect( &LockedRect, NULL, USE_DISCARD );

                        if ( SUCCEEDED( hr ) )
                        {
                            unsigned char* pPict = ( unsigned char* ) LockedRect.pBits; // Pointer to the locked bits.

                            if ( pPict )
                            {
                                vpx_image_t* img = ( vpx_image_t* )pData;

                                // divisible by RESBASE (ATI YUV hardware conversion compatible)
                                unsigned int w = ( img->d_w >> RESBASE ) << RESBASE;
                                unsigned int h = ( img->d_h >> RESBASE ) << RESBASE;
                                unsigned int w2 = w >> 1;
                                unsigned int h2 = h >> 1;

                                pPict = copyPlane( w,    h,  pPict, LockedRect.Pitch,        img->planes[VPX_PLANE_Y], img->stride[VPX_PLANE_Y] );
                                pPict = copyPlane( w2,   h2, pPict, LockedRect.Pitch >> 1,   img->planes[VPX_PLANE_V], img->stride[VPX_PLANE_V] );
                                pPict = copyPlane( w2,   h2, pPict, LockedRect.Pitch >> 1,   img->planes[VPX_PLANE_U], img->stride[VPX_PLANE_U] );
                            }

                            m_pSurfaceYUV->UnlockRect();

                            if ( pPict )
                            {
                                hr = m_pD3DDevice->StretchRect( m_pSurfaceYUV, NULL, surfaceTemp, NULL, D3DTEXF_POINT );
                            }
                        }
                    }

                    else if ( m_pStagingSurface )
                    {
                        hr = m_pStagingSurface->LockRect( &LockedRect, NULL, USE_DISCARD );

                        if ( SUCCEEDED( hr ) )
                        {
                            unsigned char* pPict = ( unsigned char* ) LockedRect.pBits; // Pointer to the locked bits.

                            if ( pPict )
                            {
                                vpx_image_t* img = ( vpx_image_t* )pData;

                                // divisible by RESBASE (ATI YUV hardware conversion compatible)
                                unsigned int w = ( img->d_w >> RESBASE ) << RESBASE;
                                unsigned int h = ( img->d_h >> RESBASE ) << RESBASE;

                                SAlphaGenParam ap;
                                YV12_2_TEX( img->planes[VPX_PLANE_Y], img->planes[VPX_PLANE_U], img->planes[VPX_PLANE_V], NULL, w, h, ( uint32_t* ) pPict, LockedRect.Pitch >> 2, img->stride[VPX_PLANE_Y], img->stride[VPX_PLANE_V], img->stride[VPX_PLANE_U], 0, ap );
                            }

                            m_pStagingSurface->UnlockRect();

                            if ( pPict )
                            {
#if defined(USE_UPDATE_SURFACE)
                                hr = m_pD3DDevice->UpdateSurface( m_pStagingSurface, NULL, surfaceTemp, NULL );
#else
                                hr = m_pD3DDevice->StretchRect( m_pStagingSurface, NULL, surfaceTemp, NULL, D3DTEXF_POINT );
#endif
                            }
                        }
                    }
                }

                if ( FAILED( hr ) )
                {
                    outputError( hr );
                }

                SAFE_RELEASE( surfaceTemp );
            }

#endif
        }
    }

    void CVideoRendererDX9::UpdateTexture()
    {
#if defined(USE_SEPERATEMEMORY)

        if ( m_pData && m_bDirty )
        {
            D3DLOCKED_RECT LockedRect;
            memset( &LockedRect, 0, sizeof( LockedRect ) );

            if ( m_pTex )
            {
                // Rendertarget is present
                IDirect3DSurface9* surfaceTemp = NULL;
                HRESULT hr = m_pTex->GetSurfaceLevel( 0, &surfaceTemp );

                if ( surfaceTemp && m_pStagingSurface )
                {
                    hr = m_pStagingSurface->LockRect( &LockedRect, NULL, USE_DISCARD );

                    if ( SUCCEEDED( hr ) )
                    {
                        unsigned char* pPict = ( unsigned char* ) LockedRect.pBits; // Pointer to the locked bits.

                        if ( pPict )
                        {
                            uint32 nDestPitch = LockedRect.Pitch;
                            uint32 nSourcePitch = m_nSourceWidth * 4;
                            copyPlane( nSourcePitch, m_nSourceHeight, pPict, nDestPitch, m_pData, nSourcePitch );
                        }

                        m_pStagingSurface->UnlockRect();

                        if ( pPict )
                        {
#if defined(USE_UPDATE_SURFACE)
                            hr = m_pD3DDevice->UpdateSurface( m_pStagingSurface, NULL, surfaceTemp, NULL );
#else
                            hr = m_pD3DDevice->StretchRect( m_pStagingSurface, NULL, surfaceTemp, NULL, D3DTEXF_POINT );
#endif
                            assert ( SUCCEEDED( hr ) );
                        }

                        m_bDirty = false;
                    }
                }

                SAFE_RELEASE( surfaceTemp );
            }
        }

#endif
    };

}
