/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <Renderer/CVideoRenderer.h>
#include <d3d9.h>

#pragma once

namespace VideoplayerPlugin
{

    class CVideoRendererDX9 :
        public CVideoRenderer
    {
            IDirect3DDevice9*           m_pD3DDevice;
            IDirect3DTexture9*          m_pTex;
            IDirect3DSurface9*          m_pSurfaceYUV;
            IDirect3DSurface9*          m_pStagingSurface;
            int                         m_iTex;

        public:
            CVideoRendererDX9();
            virtual ~CVideoRendererDX9();

            virtual eRendererType GetRendererType()
            {
                return VRT_DX9;
            };

            virtual bool CreateResources( unsigned nSourceWidth, unsigned nSourceHeight, unsigned nTargetWidth, unsigned nTargetHeight );
            virtual void ReleaseResources();

            virtual INT_PTR GetRenderTarget( eRendererType eType );

            virtual void RenderFrame( void* pData );
            virtual void UpdateTexture();
    };

}