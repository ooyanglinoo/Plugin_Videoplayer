/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <CPluginVideoplayer.h>
#include <Renderer/CVideoRendererDX11.h>
#include <WebM/vpxdec_ext.h>

namespace VideoplayerPlugin
{

    CVideoRendererDX11::CVideoRendererDX11()
    {
        gPlugin->LogAlways( "Created DX11 VideoRenderer" );
    }

    CVideoRendererDX11::~CVideoRendererDX11()
    {
    }

    void CVideoRendererDX11::ReleaseResources()
    {
        //  SAFE_RELEASE(m_pTex);
        //  SAFE_RELEASE(m_pSurfaceYUV);
    }

    bool CVideoRendererDX11::CreateResources( unsigned nSourceWidth, unsigned nSourceHeight, unsigned nTargetWidth, unsigned nTargetHeight )
    {
        ReleaseResources();

        return false;//m_pTex;
    }

    void CVideoRendererDX11::RenderFrame( void* pData )
    {
        if ( pData )
        {
            // if no img then frame was dropped

            //virtual unsigned int DownLoadToVideoMemory(unsigned char *data,int w, int h, ETEX_Format eTFSrc, ETEX_Format eTFDst, int nummipmap, bool repeat=true, int filter=FILTER_BILINEAR, int Id=0, const char *szCacheName=NULL, int flags=0, EEndian eEndian = eLittleEndian, RectI * pRegion = NULL, bool bAsynDevTexCreation = false)=0;
            //virtual void UpdateTextureInVideoMemory(uint32 tnum, unsigned char *newdata,int posx,int posy,int w,int h,ETEX_Format eTFSrc=eTF_R8G8B8)=0;

            /*
                    D3DLOCKED_RECT LockedRect; memset(&LockedRect, 0, sizeof(LockedRect));
                    if(m_pTex)
                    { // Rendertarget is present
                        IDirect3DSurface9* surfaceTemp = NULL;
                        HRESULT hr = m_pTex->GetSurfaceLevel(0, &surfaceTemp);
                        if(SUCCEEDED(hr) && surfaceTemp)
                        {
                            if(m_pSurfaceYUV)
                            {   // Hardware YUV Conversion:
                                // YV12 source -> copyPlane -> YV12 surface -> StretchRect -> X8R8G8B8 render target surface
                                hr = m_pSurfaceYUV->LockRect(&LockedRect, NULL, 0);
                                if(SUCCEEDED(hr))
                                {
                                    unsigned char* pPict = (unsigned char*) LockedRect.pBits; // Pointer to the locked bits.

                                    if(pPict)
                                    {
                                        vpx_image_t* img = (vpx_image_t*)pData;

                                        // divisible by RESBASE (ATI YUV hardware conversion compatible)
                                        unsigned int w = (img->d_w >> RESBASE) << RESBASE;
                                        unsigned int h = (img->d_h >> RESBASE) << RESBASE;
                                        unsigned int w2 = w >> 1;
                                        unsigned int h2 = h >> 1;

                                        pPict = copyPlane(w,    h,  pPict, LockedRect.Pitch,        img->planes[VPX_PLANE_Y], img->stride[VPX_PLANE_Y]);
                                        pPict = copyPlane(w2,   h2, pPict, LockedRect.Pitch >> 1,   img->planes[VPX_PLANE_V], img->stride[VPX_PLANE_V]);
                                        pPict = copyPlane(w2,   h2, pPict, LockedRect.Pitch >> 1,   img->planes[VPX_PLANE_U], img->stride[VPX_PLANE_U]);
                                    }

                                    m_pSurfaceYUV->UnlockRect();

                                    if(pPict)
                                        hr = m_pD3DDevice->StretchRect(m_pSurfaceYUV, NULL, surfaceTemp, NULL, D3DTEXF_POINT);
                                }
                            } else
                            {   // Software YUV Conversion
                                //hr = surfaceTemp->LockRect(&LockedRect, NULL, 0);
                                //if(SUCCEEDED(hr))
                                //{
                                //  unsigned char* pPict = (unsigned char*) LockedRect.pBits; // Pointer to the locked bits.
                                //  // TODO: Staging texture..
                                //  surfaceTemp->UnlockRect();
                                //}
                            }
                        }

                        if(FAILED(hr))
                            outputError(hr);

                        if(surfaceTemp)
                            surfaceTemp->Release();
                    }*/
        }
    }
}
