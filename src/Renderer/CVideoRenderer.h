/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include "stdint.h"
#include "stdlib.h"
#include <BaseTsd.h>
#include <memory.h>

#pragma once

#define RESBASE 2
#define USE_SEPERATEMEMORY // for thread safety split texture update and yuv conversion
#define USE_ALIGNEDMEMORY // for sse functions
#define ALIGNEDMEMORY 16

namespace VideoplayerPlugin
{
    /**
    * @brief byte order of the texture
    */
    enum eByteOrder
    {
        VBO_RGBA, //!< DX11 byte order
        VBO_BGRA, //!< DX9 byte order
    };

    /**
    * @brief this is a work in progress
    */
    enum eAlphaMode
    {
        VAM_FILL, //!< set alpha to 255
        VAM_PASSTROUGH, //!< write alpha channel as it is
        VAM_FALLOF, //!<  modify alpha channel using a fallof effect // TODO
        VAM_COLORMASK //!< create alpha channel based on RGB channel // TODO
    };

    /**
    * @brief this is a work in progress
    */
    typedef struct SAlphaGenParam_
    {
        SAlphaGenParam_()
        {
            r = 54;
            g = 198;
            b = 43;

            ws = r + g + b;
            wr = 1 << 8; //(r << 8) / ws;
            wg = 1 << 8; //(g << 8) / ws;
            wb = 1 << 8; //(b << 8) / ws;
            ws = wr + wg + wb;
            ws *= ws;

            fallof = 50;
            tolerance = 40;
            diff = fallof - tolerance;
            mull = 255 / diff;
        }

        int r, g, b; // colormask
        int wr, wg, wb, ws; // color weights
        int temp;
        int temp2;
        int fallof;
        int tolerance;
        int diff;
        int mull;
    } SAlphaGenParam;

    unsigned char*  copyPlane( unsigned int cols, unsigned int lines, unsigned char* dst, unsigned int dstStride, unsigned char* src, unsigned int srcStride );

    template<eByteOrder COLOR_DST_FMT, eAlphaMode ALPHAMODE>
    void YV12_2_( unsigned char* y, unsigned char* u, unsigned char* v, unsigned char* a, unsigned int cols, unsigned int lines, uint32_t* dst, unsigned int dstStride, unsigned int srcStrideY, unsigned int srcStrideU, unsigned int srcStrideV, unsigned int srcStrideA, SAlphaGenParam& ap );

    void YV12_2_TEX( unsigned char* y, unsigned char* u, unsigned char* v, unsigned char* a, unsigned int cols, unsigned int lines, uint32_t* dst, unsigned int dstStride, unsigned int srcStrideY, unsigned int srcStrideU, unsigned int srcStrideV, unsigned int srcStrideA, SAlphaGenParam& ap );

    template<eByteOrder COLOR_DST_FMT, eAlphaMode ALPHAMODE>
    void SSE2_YUV420_2_( uint8_t* yp, uint8_t* up, uint8_t* vp,
                         uint32_t sy, uint32_t suv,
                         int width, int height,
                         uint32_t* rgb, uint32_t srgb, SAlphaGenParam& ap );

    enum eVideoType
    {
        VT_LIBVPX, //!< WebM/IVF/RAW vp8 video
        VT_CACHE, // TODO
        CT_DIRECTSHOW, // TODO
        CT_KINECT, // TODO
    };

    enum eRendererType
    {
        VRT_AUTO, //!< select the correct renderer
        VRT_DX9, //!< force dx9 renderer
        VRT_DX11, //!< force dx11 renderer // TODO partial implementation present
        VRT_CE3, //!< force cryengine3 api based renderer
    };

    struct IVideoResource
    {
        virtual void AddRef() = 0;
        virtual void Release() = 0;
        virtual void Cleanup() = 0;
    };

    struct IVideoRenderer : public IVideoResource
    {
        virtual eRendererType GetRendererType() = 0;

        virtual void SetSourceType( eVideoType eType ) = 0;
        virtual eVideoType GetSourceType() = 0;

        virtual bool CreateResources( unsigned nSourceWidth, unsigned nSourceHeight, unsigned nTargetWidth, unsigned nTargetHeight ) = 0;
        virtual void ReleaseResources() = 0;
        virtual INT_PTR GetRenderTarget( eRendererType eType ) = 0;
        virtual void RenderFrame( void* pData ) = 0;
        virtual void UpdateTexture() = 0;
    };

    IVideoRenderer* createVideoRenderer( eRendererType eType );

    /**
    * @brief Mark Video Resource for later cleanup
    * @param res resource to be deleted
    */
    void markVideoResourceForCleanup( IVideoResource* res );

    /**
    * @brief Cleanup all video resources marked for cleanup
    */
    void cleanupVideoResources();

    /**
    * @brief Transfer video resources into gpu memory
    */
    void updateVideoResources( eRendererType nType );

    class CVideoRenderer :
        public IVideoRenderer
    {
        protected:

            eVideoType m_eSourceType;
            int m_nReferences;

            unsigned char* m_pData;
            unsigned int m_nSourceWidth;
            unsigned int m_nSourceHeight;
            unsigned int m_nSize;
            bool m_bDirty;

            CVideoRenderer()
            {
                m_nReferences = 1;
                m_pData = NULL;
                m_nSourceWidth = 0;
                m_nSourceHeight = 0;
                m_nSize = 0;
                m_bDirty = false;
            };

        public:

            virtual void AddRef()
            {
                m_nReferences += 1;
            };

            virtual void Release();

            virtual void Cleanup();

            virtual ~CVideoRenderer()
            {
            };

            virtual void SetSourceType( eVideoType eType )
            {
                m_eSourceType = eType;
            };
            virtual eVideoType GetSourceType()
            {
                return m_eSourceType;
            };

        protected:

            virtual bool CreateResources( unsigned nSourceWidth, unsigned nSourceHeight, unsigned nTargetWidth, unsigned nTargetHeight )
            {
                ReleaseResources();

                m_nSourceWidth  = ( nSourceWidth >> RESBASE ) << RESBASE;
                m_nSourceHeight = ( nSourceHeight >> RESBASE ) << RESBASE;

                m_nSize = m_nSourceWidth * m_nSourceHeight * 4;

#if defined(USE_ALIGNEDMEMORY)
#if defined(_WIN32)
                m_pData = ( unsigned char* )_aligned_malloc( m_nSize, ALIGNEDMEMORY );
#else
                m_pData = ( unsigned char* )memalign( ALIGNEDMEMORY, m_nSize );
#endif
#else
                m_pData = new unsigned char[m_nSize];
#endif

                memset( m_pData, 255, m_nSize );

                return m_pData;
            };

            virtual void ReleaseResources()
            {
                if ( m_pData )
                {
#if defined(USE_ALIGNEDMEMORY)
#if defined(_WIN32)
                    _aligned_free( m_pData );
#else
                    free( m_pData );
#endif
#else
                    delete [] m_pData;
#endif
                    m_pData = NULL;
                }

                m_bDirty = false;
            };

        private:

            virtual INT_PTR GetRenderTarget( eRendererType eType )
            {
                return 0;
            };

            virtual void RenderFrame( void* pData ) {};
            virtual void UpdateTexture() {};
    };

}