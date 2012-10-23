/* Videoplayer_Plugin - for licensing and copyright see license.txt
* based on libvpx - vpxdec.c
*/

#include <StdAfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#include <CPluginVideoplayer.h>

#include "vpxdec_ext.h"

#pragma comment(lib, "vpxmt.lib") // link the library (libvpx)
//#pragma comment(lib, "vpxmtd.lib") // debug versions for stack trace regarding eider crash

// Override File Operations for CryEngine to support pak files
#include <platform.h>
#include <ICryPak.h>
#define fread gEnv->pCryPak->FReadRaw
#define fopen gEnv->pCryPak->FOpen
#define fclose gEnv->pCryPak->FClose
#define fseek gEnv->pCryPak->FSeek
#define ftell gEnv->pCryPak->FTell
#define feof gEnv->pCryPak->FEof
#define ferror gEnv->pCryPak->FError
#define rewind(stream) gEnv->pCryPak->FSeek(stream, 0L, SEEK_SET)

// Override Error Log for CryEngine
#define fprintf(fh, fmt, ...) gPlugin->LogError( fmt, __VA_ARGS__ )
#define vfprintf(fh, fmt, args) gEnv->pLog->LogV( ILog::eAlways, format, args )

#define VP8_FOURCC (0x00385056)

// check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

namespace VideoplayerPlugin
{
    /**
    * @brief vp8 codec definition for FOURCC
    */
    static const struct
    {
        char const* name;
        const vpx_codec_iface_t* iface;
        unsigned int             fourcc;
        unsigned int             fourcc_mask;
    } ifaces[] =
    {
#if CONFIG_VP8_DECODER
        {"vp8",  &vpx_codec_vp8_dx_algo,   VP8_FOURCC, 0x00FFFFFF},
#endif
    };

    /**
    * @brief Little endian 16 bit conversion
    */
    static unsigned int mem_get_le16( const void* vmem )
    {
        unsigned int  val;
        const unsigned char* mem = ( const unsigned char* )vmem;

        val = mem[1] << 8;
        val |= mem[0];
        return val;
    }

    /**
    * @brief Little endian 32 bit conversion
    */
    static unsigned int mem_get_le32( const void* vmem )
    {
        unsigned int  val;
        const unsigned char* mem = ( const unsigned char* )vmem;

        val = mem[3] << 24;
        val |= mem[2] << 16;
        val |= mem[1] << 8;
        val |= mem[0];
        return val;
    }

    /**
    * @brief Read the next frame
    * @param[in] input Which file
    * @param[out] buf buffer pointer storage
    * @param[out] buf_sz buffer size (filled with data)
    * @param[out] buf_alloc_sz buffer memory size
    * @param[out] fTimeStamp timestamp of frame read (WebM format)
    * @param[out] bEnd end reached
    * @return success=0 error!=0
    */
    static int read_frame( struct input_ctx* input,
                           uint8_t** buf,
                           size_t* buf_sz,
                           size_t* buf_alloc_sz,
                           float* fTimeStamp,
                           bool* bEnd )
    {
        char            raw_hdr[IVF_FRAME_HDR_SZ];
        size_t          new_buf_sz;
        FILE*           infile = input->infile;
        enum file_kind  kind = input->kind;

        *bEnd = false;

        if ( kind == WEBM_FILE )
        {
            if ( input->chunk >= input->chunks )
            {
                unsigned int track;

                do
                {
                    /* End of this packet, get another. */
                    if ( input->pkt )
                    {
                        nestegg_free_packet( input->pkt );
                    }

                    int nret = nestegg_read_packet( input->nestegg_ctx, &input->pkt );

                    if ( 0 == nret )
                    {
                        *bEnd = true;
                    }

                    if ( nret <= 0 || nestegg_packet_track( input->pkt, &track ) )
                    {
                        return 1;
                    }

                }
                while ( track != input->video_track );

                if ( nestegg_packet_count( input->pkt, &input->chunks ) )
                {
                    return 1;
                }

                input->chunk = 0;
            }

            if ( nestegg_packet_data( input->pkt, input->chunk, buf, buf_sz ) )
            {
                return 1;
            }

            input->chunk++;

            uint64_t tstamp; //, scale; // 0 == nestegg_tstamp_scale(input->nestegg_ctx, &scale) &&

            if ( 0 == nestegg_packet_tstamp( input->pkt, &tstamp ) )
            {
                *fTimeStamp = float( tstamp ) / NANOSECOND;
            }

            return 0;
        }

        /* For both the raw and ivf formats, the frame size is the first 4 bytes
         * of the frame header. We just need to special case on the header
         * size.
         */
        else if ( fread( raw_hdr, kind == IVF_FILE ? IVF_FRAME_HDR_SZ : RAW_FRAME_HDR_SZ, 1, infile ) != 1 )
        {
            if ( !feof( infile ) )
            {
                fprintf( stderr, "Failed to read frame size\n" );
            }

            new_buf_sz = 0;
        }

        else
        {
            new_buf_sz = mem_get_le32( raw_hdr );

            if ( new_buf_sz > 256 * 1024 * 1024 )
            {
                fprintf( stderr, "Error: Read invalid frame size (%u)\n", ( unsigned int )new_buf_sz );
                new_buf_sz = 0;
            }

            if ( kind == RAW_FILE && new_buf_sz > 256 * 1024 )
            {
                fprintf( stderr, "Warning: Read invalid frame size (%u) - not a raw file?\n", ( unsigned int )new_buf_sz );
            }

            if ( new_buf_sz > *buf_alloc_sz )
            {
                uint8_t* new_buf = ( uint8_t* )realloc( *buf, 2 * new_buf_sz );

                if ( new_buf )
                {
                    *buf = new_buf;
                    *buf_alloc_sz = 2 * new_buf_sz;
                }

                else
                {
                    fprintf( stderr, "Failed to allocate compressed data buffer\n" );
                    new_buf_sz = 0;
                }
            }
        }

        *buf_sz = new_buf_sz;

        if ( !feof( infile ) )
        {
            if ( fread( *buf, 1, *buf_sz, infile ) != *buf_sz )
            {
                fprintf( stderr, "Failed to read full frame\n" );
                *bEnd = true;
                return 1;
            }

            return 0;
        }

        else
        {
            *bEnd = true;
        }

        return 1;
    }

    /**
    * @brief Is file IVF format
    * @attention Call only after opening, since it will rewind the file
    * @param[in] infile Which file
    * @param[out] fourcc FOURCC code of file
    * @param[out] width width of video
    * @param[out] height height of video
    * @param[out] fps_den FPS denominator
    * @param[out] fps_num FPS numerator
    * @return true if file is IVF format
    */
    unsigned int file_is_ivf( FILE* infile,
                              unsigned int* fourcc,
                              unsigned int* width,
                              unsigned int* height,
                              unsigned int* fps_den,
                              unsigned int* fps_num )
    {
        char raw_hdr[32];
        int is_ivf = 0;

        if ( fread( raw_hdr, 1, 32, infile ) == 32 )
        {
            if ( raw_hdr[0] == 'D' && raw_hdr[1] == 'K' && raw_hdr[2] == 'I' && raw_hdr[3] == 'F' )
            {
                is_ivf = 1;

                if ( mem_get_le16( raw_hdr + 4 ) != 0 )
                {
                    fprintf( stderr, "Error: Unrecognized IVF version! This file may not decode properly." );
                }

                *fourcc = mem_get_le32( raw_hdr + 8 );
                *width = mem_get_le16( raw_hdr + 12 );
                *height = mem_get_le16( raw_hdr + 14 );
                *fps_num = mem_get_le32( raw_hdr + 16 );
                *fps_den = mem_get_le32( raw_hdr + 20 );

                /* Some versions of vpxenc used 1/(2*fps) for the timebase, so
                 * we can guess the frame rate using only the timebase in this
                 * case. Other files would require reading ahead to guess the
                 * timebase, like we do for webm.
                 */
                if ( *fps_num < 1000 )
                {
                    /* Correct for the factor of 2 applied to the timebase in the
                     * encoder.
                     */
                    if ( *fps_num & 1 )
                    {
                        *fps_den <<= 1;
                    }

                    else
                    {
                        *fps_num >>= 1;
                    }
                }

                else
                {
                    /* Don't know FPS for sure, and don't have read ahead code
                     * (yet?), so just default to 30fps.
                     */
                    *fps_num = 30;
                    *fps_den = 1;
                }
            }
        }

        if ( !is_ivf )
        {
            rewind( infile );
        }

        return is_ivf;
    }

    /**
    * @brief Is file RAW format
    * @attention Call only after opening, since it will rewind the file
    * @param[in] infile Which file
    * @param[out] fourcc FOURCC code of file
    * @param[out] width width of video
    * @param[out] height height of video
    * @param[out] fps_den FPS denominator (=1 for raw)
    * @param[out] fps_num FPS numerator (=30 for raw)
    * @return true if file is RAW format
    */
    unsigned int file_is_raw( FILE* infile,
                              unsigned int* fourcc,
                              unsigned int* width,
                              unsigned int* height,
                              unsigned int* fps_den,
                              unsigned int* fps_num )
    {
        unsigned char buf[32];
        int is_raw = 0;
        vpx_codec_stream_info_t si;

        si.sz = sizeof( si );

        if ( fread( buf, 1, 32, infile ) == 32 )
        {
            int i;

            if ( mem_get_le32( buf ) < 256 * 1024 * 1024 )
                for ( i = 0; i < sizeof( ifaces ) / sizeof( ifaces[0] ); i++ )
                    if ( !vpx_codec_peek_stream_info( ifaces[i].iface, buf + 4, 32 - 4, &si ) )
                    {
                        is_raw = 1;
                        *fourcc = ifaces[i].fourcc;
                        *width = si.w;
                        *height = si.h;
                        *fps_num = 30;
                        *fps_den = 1;
                        break;
                    }
        }

        rewind( infile );
        return is_raw;
    }

    /**
    * @brief Read Callback WebM format
    */
    static int nestegg_read_cb( void* buffer, size_t length, void* userdata )
    {
        FILE* f = ( FILE* )userdata;

        if ( fread( buffer, 1, length, f ) < length )
        {
            if ( ferror( f ) )
            {
                return -1;
            }

            if ( feof( f ) )
            {
                return 0;
            }
        }

        return 1;
    }

    /**
    * @brief Seek Callback WebM format
    */
    static int nestegg_seek_cb( int64_t offset, int whence, void* userdata )
    {
        switch ( whence )
        {
            case NESTEGG_SEEK_SET:
                whence = SEEK_SET;
                break;

            case NESTEGG_SEEK_CUR:
                whence = SEEK_CUR;
                break;

            case NESTEGG_SEEK_END:
                whence = SEEK_END;
                break;
        };

        return fseek( ( FILE* )userdata, offset, whence ) ? -1 : 0;
    }

    /**
    * @brief Tell Callback WebM format
    */
    static int64_t nestegg_tell_cb( void* userdata )
    {
        return ftell( ( FILE* )userdata );
    }

    /**
    * @brief Log Callback WebM format
    */
    static void nestegg_log_cb( nestegg* context, unsigned int severity, char const* format, ... )
    {
        if ( severity >= NESTEGG_LOG_INFO )
        {
            va_list ap;
            va_start( ap, format );
            vfprintf( stderr, format, ap );
            fprintf( stderr, "\n" );
            va_end( ap );
        }
    }

    /**
    * @brief WebM format framerate guessing
    * reads the first second of video to determine FPS
    * @param[in] input Which file
    * @param[out] fps_den FPS denominator
    * @param[out] fps_num FPS numerator
    * @return success=0 fail!=0
    */
    static int webm_guess_framerate( struct input_ctx* input,
                                     unsigned int*     fps_den,
                                     unsigned int*     fps_num )
    {
        unsigned int i;
        uint64_t     tstamp = 0;

        long nFallbackPos = ftell( input->infile );

        /* Guess the framerate. Read up to 1 second, or 50 video packets,
         * whichever comes first.
         */
        for ( i = 0; tstamp < 1000000000 && i < 50; )
        {
            nestegg_packet* pkt;
            unsigned int track;

            if ( nestegg_read_packet( input->nestegg_ctx, &pkt ) <= 0 )
            {
                break;
            }

            nestegg_packet_track( pkt, &track );

            if ( track == input->video_track )
            {
                nestegg_packet_tstamp( pkt, &tstamp );
                i++;
            }

            nestegg_free_packet( pkt );
        }

        if ( nestegg_track_seek( input->nestegg_ctx, input->video_track, 0 ) )
        {
            fseek( input->infile, nFallbackPos, SEEK_SET );    //goto fail;
        }

        *fps_num = ( i - 1 ) * 1000000;
        *fps_den = tstamp / 1000;
        return 0;
fail:
        nestegg_destroy( input->nestegg_ctx );
        input->nestegg_ctx = NULL;
        rewind( input->infile );
        return 1;
    }

    /**
    * @brief Is file WEBM format
    * @attention Call only after opening, since it will rewind the file
    * @param[in] infile Which file
    * @param[out] fourcc FOURCC code of file
    * @param[out] width width of video
    * @param[out] height height of video
    * @param[out] fps_den FPS denominator
    * @param[out] fps_num FPS numerator
    * @param[in] bRetry will perform a recursive retry if set to false.
    * @return true if file is WEBM format
    */
    static int file_is_webm( struct input_ctx* input,
                             unsigned int*     fourcc,
                             unsigned int*     width,
                             unsigned int*     height,
                             unsigned int*     fps_den,
                             unsigned int*     fps_num,
                             bool             bRetry = false )
    {
        unsigned int i, n;
        int          track_type = -1;

        nestegg_io io = {nestegg_read_cb, nestegg_seek_cb, nestegg_tell_cb,
                         input->infile
                        };
        nestegg_video_params params;

        input->nestegg_ctx = NULL;

        if ( nestegg_init( &input->nestegg_ctx, io, NULL /*nestegg_log_cb*/ ) )
        {
            goto fail;
        }

        if ( nestegg_track_count( input->nestegg_ctx, &n ) )
        {
            goto fail;
        }

        for ( i = 0; i < n; i++ )
        {
            track_type = nestegg_track_type( input->nestegg_ctx, i );

            if ( track_type == NESTEGG_TRACK_VIDEO )
            {
                break;
            }

            else if ( track_type < 0 )
            {
                goto fail;
            }
        }

        if ( nestegg_track_codec_id( input->nestegg_ctx, i ) != NESTEGG_CODEC_VP8 )
        {
            fprintf( stderr, "Not VP8 video codec.\n" );
            goto fail;
        }

        input->video_track = i;

        if ( nestegg_track_video_params( input->nestegg_ctx, i, &params ) )
        {
            goto fail;
        }

        *fourcc = VP8_FOURCC;
        *width = params.width;
        *height = params.height;

        if ( !bRetry )
        {
            *fps_den = 0;
            *fps_num = 0;

            if ( webm_guess_framerate( input, fps_den, fps_num ) )
            {
                goto fail;
            }

            rewind( input->infile );

            if ( input->nestegg_ctx )
            {
                nestegg_destroy( input->nestegg_ctx );
            }

            return file_is_webm( input,
                                 fourcc,
                                 width,
                                 height,
                                 fps_den,
                                 fps_num,
                                 true );
        }

        return 1;
fail:

        if ( input->nestegg_ctx )
        {
            nestegg_destroy( input->nestegg_ctx );
        }

        rewind( input->infile );
        input->nestegg_ctx = NULL;
        return 0;
    }

    int VPXDec::open( char* fn, bool bLoop, float fStartAt, float fEndAfter, IVideoplayerEventListener* pBroadcast )
    {
        int i;
        cleanup();

        m_bLoop = bLoop;
        m_fStartAt = fStartAt;
        m_fEndAfter = fEndAfter;
        m_fLastReportedEnd = -1;
        m_pBroadcast = pBroadcast;

        /* Parse command line, uncommented for later post processing configuration */

        /*
        exec_name = argv_[0];
        argv = argv_dup(argc - 1, argv_ + 1);

        for (argi = argj = argv; (*argj = *argi); argi += arg.argv_step)
        {
            memset(&arg, 0, sizeof(arg));
            arg.argv_step = 1;

            if (arg_match(&arg, &codecarg, argi))
            {
                int j, k = -1;

                for (j = 0; j < sizeof(ifaces) / sizeof(ifaces[0]); j++)
                    if (!strcmp(ifaces[j].name, arg.val))
                        k = j;

                if (k >= 0)
                    iface = ifaces[k].iface;
                else
                    die("Error: Unrecognized argument (%s) to --codec\n",
                        arg.val);
            }
            else if (arg_match(&arg, &outputfile, argi))
                outfile_pattern = arg.val;
            else if (arg_match(&arg, &use_yv12, argi))
            {
                use_y4m = 0;
                flipuv = 1;
            }
            else if (arg_match(&arg, &use_i420, argi))
            {
                use_y4m = 0;
                flipuv = 0;
            }
            else if (arg_match(&arg, &flipuvarg, argi))
                flipuv = 1;
            else if (arg_match(&arg, &noblitarg, argi))
                noblit = 1;
            else if (arg_match(&arg, &limitarg, argi))
                stop_after = arg_parse_uint(&arg);
            else if (arg_match(&arg, &postprocarg, argi))
                postproc = 1;
            else if (arg_match(&arg, &threadsarg, argi))
                cfg.threads = arg_parse_uint(&arg);
        #if CONFIG_VP8_DECODER
            else if (arg_match(&arg, &addnoise_level, argi))
            {
                postproc = 1;
                vp8_pp_cfg.post_proc_flag |= VP8_ADDNOISE;
                vp8_pp_cfg.noise_level = arg_parse_uint(&arg);
            }
            else if (arg_match(&arg, &demacroblock_level, argi))
            {
                postproc = 1;
                vp8_pp_cfg.post_proc_flag |= VP8_DEMACROBLOCK;
                vp8_pp_cfg.deblocking_level = arg_parse_uint(&arg);
            }
            else if (arg_match(&arg, &deblock, argi))
            {
                postproc = 1;
                vp8_pp_cfg.post_proc_flag |= VP8_DEBLOCK;
            }
            else if (arg_match(&arg, &mfqe, argi))
            {
                postproc = 1;
                vp8_pp_cfg.post_proc_flag |= VP8_MFQE;
            }
            else if (arg_match(&arg, &pp_debug_info, argi))
            {
                unsigned int level = arg_parse_uint(&arg);

                postproc = 1;
                vp8_pp_cfg.post_proc_flag &= ~0x7;

                if (level)
                    vp8_pp_cfg.post_proc_flag |= level;
            }
            else if (arg_match(&arg, &pp_disp_ref_frame, argi))
            {
                unsigned int flags = arg_parse_int(&arg);
                if (flags)
                {
                    postproc = 1;
                    vp8_dbg_color_ref_frame = flags;
                }
            }
            else if (arg_match(&arg, &pp_disp_mb_modes, argi))
            {
                unsigned int flags = arg_parse_int(&arg);
                if (flags)
                {
                    postproc = 1;
                    vp8_dbg_color_mb_modes = flags;
                }
            }
            else if (arg_match(&arg, &pp_disp_b_modes, argi))
            {
                unsigned int flags = arg_parse_int(&arg);
                if (flags)
                {
                    postproc = 1;
                    vp8_dbg_color_b_modes = flags;
                }
            }
            else if (arg_match(&arg, &pp_disp_mvs, argi))
            {
                unsigned int flags = arg_parse_int(&arg);
                if (flags)
                {
                    postproc = 1;
                    vp8_dbg_display_mv = flags;
                }
            }
            else if (arg_match(&arg, &error_concealment, argi))
            {
                ec_enabled = 1;
            }

        #endif
            else
                argj++;
        }
        */

        /* Open file */
        m_infile = fopen( fn, "rb" );

        if ( !m_infile )
        {
            fprintf( stderr, "Failed to open file '%s'", strcmp( fn, "-" ) ? fn : "stdin" );
            goto error_open;
        }

        m_input.infile = m_infile;

        if ( file_is_ivf( m_infile, &m_fourcc, &m_nWidth, &m_nHeight, &m_nFPSDen, &m_nFPSNum ) )
        {
            m_input.kind = IVF_FILE;
        }

        else if ( file_is_webm( &m_input, &m_fourcc, &m_nWidth, &m_nHeight, &m_nFPSDen, &m_nFPSNum ) )
        {
            m_input.kind = WEBM_FILE;
        }

        else if ( file_is_raw( m_infile, &m_fourcc, &m_nWidth, &m_nHeight, &m_nFPSDen, &m_nFPSNum ) )
        {
            m_input.kind = RAW_FILE;
        }

        else
        {
            fprintf( stderr, "Unrecognized input file type.\n" );
            goto error_open;
        }

        /* Make divisible by RESBASE to fix ATI Hardware YUV Color conversion */
        m_nWidth = ( m_nWidth >> RESBASE ) << RESBASE;
        m_nHeight = ( m_nHeight >> RESBASE ) << RESBASE;

        /* Try to determine duration */
        if ( m_input.kind == WEBM_FILE && m_input.nestegg_ctx )
        {
            uint64_t nPosU = 0;

            if ( 0 == nestegg_duration( m_input.nestegg_ctx, &nPosU ) )
            {
                m_fDuration = nPosU / NANOSECOND;    // 10^9
            }

            else
            {
                m_fDuration = -1;
            }
        }

        /* Try to determine the codec from the fourcc. */
        for ( i = 0; i < sizeof( ifaces ) / sizeof( ifaces[0] ); i++ )
        {
            if ( ( m_fourcc & ifaces[i].fourcc_mask ) == ifaces[i].fourcc )
            {
                vpx_codec_iface_t*  ivf_iface = ifaces[i].iface;

                if ( m_iface && m_iface != ivf_iface )
                {
                    fprintf( stderr, "Notice -- IVF header indicates codec: %s\n", ifaces[i].name );
                }

                else
                {
                    m_iface = ivf_iface;
                }

                break;
            }
        }

        m_nDecFlags = ( m_bPostProc ? VPX_CODEC_USE_POSTPROC : 0 ) | ( m_bECEnabled ? VPX_CODEC_USE_ERROR_CONCEALMENT : 0 );

        if ( vpx_codec_dec_init( &m_decoder, m_iface ? m_iface :  ifaces[0].iface, &m_cfg, m_nDecFlags ) )
        {
            fprintf( stderr, "Failed to initialize decoder: %s\n", vpx_codec_error( &m_decoder ) );
            goto error_open;
        }

#if CONFIG_VP8_DECODER

        if ( m_vp8_pp_cfg.post_proc_flag && vpx_codec_control( &m_decoder, VP8_SET_POSTPROC, &m_vp8_pp_cfg ) )
        {
            fprintf( stderr, "Failed to configure postproc: %s\n", vpx_codec_error( &m_decoder ) );
            goto error_open;
        }

        if ( m_vp8_dbg_color_ref_frame && vpx_codec_control( &m_decoder, VP8_SET_DBG_COLOR_REF_FRAME, m_vp8_dbg_color_ref_frame ) )
        {
            fprintf( stderr, "Failed to configure reference block visualizer: %s\n", vpx_codec_error( &m_decoder ) );
            goto error_open;
        }

        if ( m_vp8_dbg_color_mb_modes && vpx_codec_control( &m_decoder, VP8_SET_DBG_COLOR_MB_MODES, m_vp8_dbg_color_mb_modes ) )
        {
            fprintf( stderr, "Failed to configure macro block visualizer: %s\n", vpx_codec_error( &m_decoder ) );
            goto error_open;
        }

        if ( m_vp8_dbg_color_b_modes && vpx_codec_control( &m_decoder, VP8_SET_DBG_COLOR_B_MODES, m_vp8_dbg_color_b_modes ) )
        {
            fprintf( stderr, "Failed to configure block visualizer: %s\n", vpx_codec_error( &m_decoder ) );
            goto error_open;
        }

        if ( m_vp8_dbg_display_mv && vpx_codec_control( &m_decoder, VP8_SET_DBG_DISPLAY_MV, m_vp8_dbg_display_mv ) )
        {
            fprintf( stderr, "Failed to configure motion vector visualizer: %s\n", vpx_codec_error( &m_decoder ) );
            goto error_open;
        }

#endif

        return EXIT_SUCCESS;
error_open:
        cleanup();
        return EXIT_FAILURE;
    }

    int VPXDec::seek( float fTimepos )
    {
        if ( m_fStartAt > VIDEO_EPSILON )
        {
            fTimepos = max( fTimepos, m_fStartAt );
        }

        if ( m_fEndAfter > VIDEO_EPSILON )
        {
            fTimepos = min( fTimepos, m_fEndAfter );
        }

        if ( fTimepos < VIDEO_EPSILON )
        {
            fTimepos = 0;
        }

        if ( m_input.kind == WEBM_FILE && m_input.nestegg_ctx )
        {
            uint64_t nPos = fTimepos * NANOSECOND;
            m_fPos = fTimepos;
            m_nFrameIn =  fTimepos * getFPS();
            m_nFrameOut = fTimepos * getFPS();
            m_nCorrupted = 0;
            m_nFramesCorrupted = 0;
            m_fLastReportedEnd = -1;

            int nRet = nestegg_track_seek( m_input.nestegg_ctx, m_input.video_track, nPos );

            if ( nRet && fTimepos <= VIDEO_EPSILON )
            {
                // fall back loop for damaged files
                rewind( m_input.infile );

                if ( m_input.nestegg_ctx )
                {
                    nestegg_destroy( m_input.nestegg_ctx );
                }

                m_input.nestegg_ctx = NULL;

                if ( file_is_webm( &m_input, &m_fourcc, &m_nWidth, &m_nHeight, &m_nFPSDen, &m_nFPSNum, true ) )
                {
                    nRet = 0;
                }

                // Make divisible by RESBASE to fix ATI Hardware YUV Color conversion
                m_nWidth = ( m_nWidth >> RESBASE ) << RESBASE;
                m_nHeight = ( m_nHeight >> RESBASE ) << RESBASE;
            }

            if ( nRet == 0 && m_pBroadcast )
            {
                m_pBroadcast->OnSeek();
            }

            return nRet;
        }

        return -1;
    }

    float VPXDec::getDuration()
    {
        return m_fDuration;
    }

    float VPXDec::getPosition()
    {
        return m_fPos;
    }

    float VPXDec::getFPS()
    {
        return float( m_nFPSNum ) / float( m_nFPSDen );
    }

    int VPXDec::readFrame( vpx_image_t** pData, bool& bDirty, bool bDropDecode, bool bDropOutput )
    {
        float fCurrentPos = -1;
        bool bEnd = false;
        int nRet = 0;

        // Nothing changed for now
        bDirty = false;

        if ( pData )
        {
            *pData = NULL;
        }

        // If nothing is decoded then there is nothing to output
        if ( bDropDecode || !pData )
        {
            bDropOutput = true;
        }

        // Custom End reached
        if ( m_fEndAfter >= VIDEO_EPSILON && m_fPos >= m_fEndAfter )
        {
            goto end;
        }

        // Goto Custom Start
        bool bStart = m_fStartAt >= VIDEO_EPSILON && m_fPos < m_fStartAt;

        if ( bStart )
        {
            seek( m_fStartAt );
        }

        bStart |= ( m_nFrameIn == 0 ); // Normal Start

        if ( m_pBroadcast && bStart )
        {
            m_pBroadcast->OnStart();
        }

        // Decoder closed?
        if ( !isOpen() )
        {
            goto fail;
        }

        // read file
        nRet = read_frame( &m_input, &m_buf, &m_buf_sz, &m_buf_alloc_sz, &fCurrentPos, &bEnd );

        // Calculate current position
        if ( fCurrentPos >= 0.0f )
        {
            m_fPos = fCurrentPos;
        }

        else
        {
            m_fPos = float( m_nFrameIn ) / getFPS();
        }

        if ( bEnd ) // reached end?
        {
            goto end;
        }

        if ( nRet || !isOpen() ) // error, decoder closed?
        {
            goto fail;
        }

        // Dropping like this will produce a crash since 1.1 Eider
        if ( !bDropDecode )
        {
            // Decode frame // TODO: Deadline if post processing is added sometime in the future
            if ( vpx_codec_decode( &m_decoder, m_buf, m_buf_sz, NULL, 0 ) )
            {
                const char* detail = vpx_codec_error_detail( &m_decoder );
                fprintf( stderr, "Failed to decode frame: %s\n", vpx_codec_error( &m_decoder ) );

                if ( detail )
                {
                    fprintf( stderr, "  Additional information: %s\n", detail );
                }

                goto fail;
            }
        }

        //else // added because of eider release but didn't help so for now uncommented again
        //{
        //    vpx_codec_decode( &m_decoder, NULL, 0, NULL, 0 ) ;
        //}

        ++m_nFrameIn;

        // Output frame
        if ( !bDropOutput )
        {
            if ( !isOpen() ) // Decoder closed?
            {
                goto fail;
            }

            m_iter = NULL;

            if ( m_img = vpx_codec_get_frame( &m_decoder, &m_iter ) )
            {
                ++m_nFrameOut;

                if ( pData )
                {
                    *pData = m_img;
                    bDirty = true;
                }
            }
        }

        // Now refresh actual start pos
        if ( m_fStartAt >= VIDEO_EPSILON && m_fPos < m_fStartAt )
        {
            m_fStartAt = m_fPos;
        }

        // Broadcast New Frame event to listeners
        if ( m_pBroadcast && bDirty )
        {
            m_pBroadcast->OnFrame();
        }

        return EXIT_SUCCESS;

end:

        if ( m_pBroadcast && m_fLastReportedEnd != m_fPos )
        {
            m_fLastReportedEnd = m_fPos;
            m_pBroadcast->OnEnd();
        }

        if ( m_bLoop )
        {
            seek( m_fStartAt );

            if ( m_pBroadcast )
            {
                m_pBroadcast->OnStart();
            }
        }

        return EXIT_SUCCESS;

fail:
        return EXIT_FAILURE;
    }

    bool VPXDec::isOpen()
    {
        return m_infile && m_decoder.iface && gEnv->pSystem && !gEnv->pSystem->IsQuitting();
    }

    int VPXDec::cleanup()
    {
        m_iter = NULL;
        m_img = NULL;
        m_pBroadcast = NULL;

        m_nCorrupted = 0;
        m_nFramesCorrupted = 0;
        m_fPos = 0;
        m_fDuration = 0;
        m_nFrameIn = 0;
        m_nFrameOut = 0;
        m_nFPSDen = 0;
        m_nFPSNum = 0;

        m_nWidth = 0;
        m_nHeight = 0;

        if ( m_decoder.name )
        {
            if ( vpx_codec_destroy( &m_decoder ) )
            {
                fprintf( stderr, "Failed to destroy decoder: %s\n", vpx_codec_error( &m_decoder ) );
            }
        }

        memset( &m_decoder, 0, sizeof( m_decoder ) );

        if ( m_input.nestegg_ctx )
        {
            nestegg_destroy( m_input.nestegg_ctx );
        }

        if ( m_input.kind != WEBM_FILE )
        {
            free( m_buf );
        }

        memset( &m_input, 0, sizeof( m_input ) );

        if ( m_infile )
        {
            fclose( m_infile );
            m_infile = NULL;
        }

        m_fn = NULL;
        m_buf = NULL;
        m_buf_sz = 0;
        m_buf_alloc_sz = 0;
        m_bNoBlit = 0;

        m_fEndAfter = 0;
        m_fStartAt = 0;
        m_fLastReportedEnd = -1;

        m_bPostProc = 0;
        m_bECEnabled = 0;

        m_iface = NULL;
        m_fourcc  = 0;

        m_sFile = "";
        m_bLoop = false;

        memset( &m_cfg, 0, sizeof( m_cfg ) );
        memset( &m_vp8_pp_cfg, 0, sizeof( m_vp8_pp_cfg ) );

        m_vp8_dbg_color_ref_frame = 0;
        m_vp8_dbg_color_mb_modes = 0;
        m_vp8_dbg_color_b_modes = 0;
        m_vp8_dbg_display_mv = 0;

        m_nDecFlags = 0;

        return EXIT_SUCCESS;
    }
}
