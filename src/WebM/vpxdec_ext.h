/* Videoplayer_Plugin - for licensing and copyright see license.txt
* based on libvpx - vpxdec.c
*/

#define VPX_CODEC_DISABLE_COMPAT 1

#include <vpx_config.h>
#include <vpx/vpx_decoder.h>
#include <vpx_ports/vpx_timer.h>

#if CONFIG_VP8_DECODER
#include <vpx/vp8dx.h>
#endif

#include <IPluginVideoplayer.h>

#include <tools_common.h>
#include <nestegg/include/nestegg/nestegg.h>

#if CONFIG_OS_SUPPORT
#if defined(_MSC_VER)
#include <io.h>
#define snprintf _snprintf
#define isatty   _isatty
#define fileno   _fileno
#else
#include <unistd.h>
#endif
#endif

#pragma once

namespace VideoplayerPlugin
{
    /**
    * @brief Supported filetypes by libvpx
    */
    enum file_kind
    {
        RAW_FILE,
        IVF_FILE,
        WEBM_FILE
    };

    /**
    * @brief Information about openend file
    */
    struct input_ctx
    {
        enum file_kind  kind;
        FILE*           infile;
        nestegg*        nestegg_ctx;
        nestegg_packet* pkt;
        unsigned int    chunk;
        unsigned int    chunks;
        unsigned int    video_track;
    };

#define IVF_FRAME_HDR_SZ (sizeof(uint32_t) + sizeof(uint64_t))
#define RAW_FRAME_HDR_SZ (sizeof(uint32_t))

    /**
    * @brief Decoderclass for each opened file
    * @attention currently webm format is recommend (RAW and IVF may require additional work, but the general support is there)
    */
    class VPXDec
    {
        private:
            vpx_codec_ctx_t        m_decoder;
            char*                  m_fn;

            uint8_t*               m_buf;
            size_t                 m_buf_sz,
                                   m_buf_alloc_sz;

            FILE*                  m_infile;

            float                   m_fPos,
                                    m_fDuration;

            int                     m_nFrameIn,
                                    m_nFrameOut,
                                    m_bNoBlit,
                                    m_bPostProc;

            string                  m_sFile;

            int                     m_bECEnabled;

            vpx_codec_iface_t*       m_iface;
            unsigned int           m_fourcc;

            unsigned int            m_nFPSDen;
            unsigned int            m_nFPSNum;

            vpx_codec_dec_cfg_t     m_cfg;

            vp8_postproc_cfg_t      m_vp8_pp_cfg;
            int                     m_vp8_dbg_color_ref_frame;
            int                     m_vp8_dbg_color_mb_modes;
            int                     m_vp8_dbg_color_b_modes;
            int                     m_vp8_dbg_display_mv;

            struct input_ctx        m_input;
            int                     m_nFramesCorrupted;
            int                     m_nDecFlags;

            vpx_codec_iter_t        m_iter;
            vpx_image_t*             m_img;

            int                     m_nCorrupted;

            IVideoplayerEventListener*  m_pBroadcast;

        public:
            unsigned int m_nWidth; //!< video width
            unsigned int m_nHeight; //!< video height
            bool m_bLoop; //!< loop the video

            float   m_fEndAfter, //!< custom end position
                    m_fStartAt; //!< custom start position

            float m_fLastReportedEnd; //!< last end reached

            /**
            * @brief Open Video file
            * @param fStartAt custom start position
            * @param fEndAfter custom end position
            * @param pBroadcast event dispatcher.
            * @return success (EXIT_SUCCESS)
            */
            int open( char* fn, bool bLoop = false, float fStartAt = 0, float fEndAfter = 0, IVideoplayerEventListener* pBroadcast = NULL );

            /**
            * @brief Read the next frame
            * @param[out] pData Pointer to Pointer that should hold the decoded planar YV12 raw data
            * @param[out] bDirty Set if new data was written.
            * @param bDropDecode Only read data but don't decode/output it (sets implicit drop output)
            * @param bDropOutput Read and decode data but don't output it
            * @attention dispatches some of the video events.
            * @return success
            */
            int readFrame( vpx_image_t** pData, bool& bDirty, bool bDropDecode = false, bool bDropOutput = false );

            /**
            * @brief seek video stream
            * @attention only avaible in WebM format.
            * @return success
            */
            int seek( float fTimepos = 0 );

            /**
            * @brief Retrieve the duration of the video file
            * @return Duration in seconds
            */
            float getDuration();

            /**
            * @brief Retrieve the current position in the file
            * @return Position in seconds.
            */
            float getPosition();

            /**
            * @brief Retrieve frames per second of the current video stream
            * @return Frames per second (e.g. 25,0)
            */
            float getFPS();

            /**
            * @brief Has the decoder currently a video file open
            * @return open
            */
            bool isOpen();

            /**
            * @brief Free all decoder resources
            * @return success
            */
            int cleanup();

            /**
            * @brief Destructor with cleanup
            * @see cleanup
            */
            ~VPXDec()
            {
                cleanup();
            }

            /**
            * @brief Constructor resetting all data fields
            */
            VPXDec()
            {
                m_fPos = 0;
                m_fDuration = 0;

                m_fn = NULL;

                m_buf = NULL;
                m_buf_sz = 0;
                m_buf_alloc_sz = 0;
                m_infile = NULL;
                m_nFrameIn = 0;
                m_nFrameOut = 0;
                m_bNoBlit = 0;

                m_fEndAfter = 0;
                m_fStartAt = 0;
                m_fLastReportedEnd = -1;

                m_bPostProc = 0;
                m_bECEnabled = 0;

                m_iface = NULL;
                m_fourcc  = 0;

                m_nFPSDen = 0;
                m_nFPSNum = 0;
                m_sFile = "";
                m_bLoop = false;

                memset( &m_decoder, 0, sizeof( m_decoder ) );
                memset( &m_cfg, 0, sizeof( m_cfg ) );
                memset( &m_vp8_pp_cfg, 0, sizeof( m_vp8_pp_cfg ) );

                m_vp8_dbg_color_ref_frame = 0;
                m_vp8_dbg_color_mb_modes = 0;
                m_vp8_dbg_color_b_modes = 0;
                m_vp8_dbg_display_mv = 0;

                memset( &m_input, 0, sizeof( m_input ) );

                m_nCorrupted = 0;
                m_nFramesCorrupted = 0;
                m_nDecFlags = 0;

                m_nWidth  = 0;
                m_nHeight  = 0;

                m_iter = NULL;
                m_img = NULL;
                m_pBroadcast = NULL;
            }
    };
}
