/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <Renderer/CVideoRenderer.h>
#include <Game.h>

namespace VideoplayerPlugin
{

    class CVideoRendererCE3 :
        public CVideoRenderer
    {
            int             m_iTex;

        public:
            CVideoRendererCE3();
            virtual ~CVideoRendererCE3();

            virtual eRendererType GetRendererType()
            {
                return VRT_CE3;
            };

            virtual bool CreateResources( unsigned nSourceWidth, unsigned nSourceHeight, unsigned nTargetWidth, unsigned nTargetHeight );
            virtual void ReleaseResources();

            virtual INT_PTR GetRenderTarget( eRendererType eType );

            virtual void RenderFrame( void* pData );
            virtual void UpdateTexture();
    };
}
