/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <Renderer/CVideoRenderer.h>
#include <d3d11.h>

namespace VideoplayerPlugin
{

    class CVideoRendererDX11 :
        public CVideoRenderer
    {
            //  IDirect3DDevice9*           m_pD3DDevice;
            //  ID3D11Texture2D*            m_pTex;
            //  IDirect3DSurface9*          m_pSurfaceYUV;
            int                         m_iTex;

        public:
            CVideoRendererDX11();
            virtual ~CVideoRendererDX11();

            virtual eRendererType GetRendererType()
            {
                return VRT_DX11;
            };

            virtual bool CreateResources( unsigned nSourceWidth, unsigned nSourceHeight, unsigned nTargetWidth, unsigned nTargetHeight );
            virtual void ReleaseResources();

            virtual void RenderFrame( void* pData );
    };

}