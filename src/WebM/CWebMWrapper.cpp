/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <Stdafx.h>

#include "CWebMWrapper.h"
#include <CPluginVideoplayer.h>
#include <CVideoplayerSystem.h>

namespace VideoplayerPlugin
{
    CWebMWrapper::CWebMWrapper( int nVideoId )
    {
        m_iCE3Tex = 0;
        m_pCE3Tex = NULL;
        m_sCE3Tex = "";

        m_bSkippable = true;
        m_bSkipping = false;
        m_bPaused = true;
        m_nVideoId = nVideoId;
        m_fSpeed = 1;
        m_nWidth = 0;
        m_nHeight = 0;
        m_nRendererWidth = 0;
        m_nRendererHeight = 0;
        m_eTS = VTS_Default;
        m_eDM = VDM_Default;

        m_VRenderer = NULL;
    }

    CWebMWrapper::~CWebMWrapper()
    {
        Close();
        SAFE_RELEASE( m_VRenderer );
    }

    void CWebMWrapper::Close()
    {
        m_bPaused = true;

        m_Sound.Close();

        ReleaseResources( true );
#ifdef _DEBUG
        gPlugin->LogAlways( "Close id(%d)", m_nVideoId );
#endif
        m_bSkipping = false;
        m_bSkippable = true;
    }

    bool CWebMWrapper::ReleaseResources( bool bResetOverride )
    {
        gVideoplayerSystem->RestoreMaterials( this, bResetOverride ); // restore materials using this video

        SAFE_RELEASE( m_VRenderer );
        m_iCE3Tex = 0;
        m_sCE3Tex = "";
        m_pCE3Tex = NULL;
        m_nRendererWidth = 0;
        m_nRendererHeight = 0;

        return true;
    }

    bool CWebMWrapper::CreateResources()
    {
        // needed for 2d placement
        m_nRendererWidth = gEnv->pRenderer->GetWidth();
        m_nRendererHeight = gEnv->pRenderer->GetHeight();

        // release old data
        m_pCE3Tex = NULL;
        SAFE_RELEASE( m_VRenderer );

        // create new data
        if ( m_VRenderer = createVideoRenderer( VRT_AUTO ) )
        {
            if ( m_VRenderer->CreateResources( m_decoder.m_nWidth, m_decoder.m_nHeight, m_nWidth, m_nHeight ) )
            {
                m_pCE3Tex = reinterpret_cast<ITexture*>( m_VRenderer->GetRenderTarget( VRT_CE3 ) );
            }
        }

        // override material with the new textures
        if ( m_pCE3Tex )
        {
            m_sCE3Tex   = m_pCE3Tex->GetName();
            m_iCE3Tex   = m_pCE3Tex->GetTextureID();

            gVideoplayerSystem->OverrideMaterials( this );
            return m_pCE3Tex;
        }

        else
        {
            gPlugin->LogAlways( "Could not create texture." );
        }

        return false;
    }

    ISoundplayer* CWebMWrapper::GetSoundplayer()
    {
        return &m_Sound;
    }

    bool CWebMWrapper::ReOpen()
    {
        // TODO
        //Open(const char* sFile, const char* sSound, bool bLoop, eTimeSource eTS, float fStartAt, float fEndAfter, int nCustomWidth, int nCustomHeight)
        return true;
    }

    bool CWebMWrapper::Open( const char* sFile, const char* sSound, bool bLoop, bool bSkippable, bool bBlockGame, eTimeSource eTS, eDropMode eDM, float fStartAt, float fEndAfter, int nCustomWidth, int nCustomHeight )
    {
        Close();
        SetTimesource( eTS );
        m_eDM = eDM;

        if ( EXIT_SUCCESS == m_decoder.open( ( char* )sFile, bLoop, fStartAt, fEndAfter, this ) )
        {
            m_nWidth = nCustomWidth > 0 ? nCustomWidth : m_decoder.m_nWidth;
            m_nHeight = nCustomHeight > 0 ? nCustomHeight : m_decoder.m_nHeight;
            m_bSkippable = bSkippable;
            CreateResources();
        }

        m_Sound.Open( sSound, this, bLoop );

        gPlugin->LogAlways( "Open id(%d) file(%s) sound(%s)", m_nVideoId, sFile, sSound );

        return m_pCE3Tex;
    }

    void CWebMWrapper::SetTimesource( eTimeSource eTS )
    {
        if ( IsPlaying() )
        {
            // needs to be resynchronized
            Pause();
            m_eTS = eTS;
            Resume();
        }

        else
        {
            m_eTS = eTS;
        }
    }

    void CWebMWrapper::SetSpeed( float fSpeed )
    {
        if ( fabs( m_fSpeed - 1 ) > 0.05 || fabs( fSpeed - 1 ) > 0.05 )
        {
            // modify the speed only if there is a difference
            m_fSpeed = fSpeed;
        }
    }

    void CWebMWrapper::Skip( bool bForce )
    {
        if ( m_bSkippable || bForce )
        {
            m_bSkipping = true;
        }
    }

    bool CWebMWrapper::IsActive()
    {
        return m_decoder.isOpen();
    }

    bool CWebMWrapper::IsPlaying()
    {
        return IsActive() ? !m_bPaused : false;
    }

    void CWebMWrapper::Resume()
    {
        m_bPaused = false;

        // initialize timers
        if ( m_eTS & VTS_SystemTime )
        {
            vpx_usec_timer_start( &m_timer );
        }

        m_fTimer = m_decoder.getPosition();
        m_fTimerNextFrame = m_fTimer - GetFrameDuration(); // forces output of next frame

        // resume playback at last position
        m_Sound.Resume();

        gPlugin->LogAlways( "Resume id(%d) video(%.2fs) sound(%.2fs) duration(%.2fs)", m_nVideoId, GetPosition(), m_Sound.GetPosition(), GetDuration() );
    }

    void CWebMWrapper::Pause()
    {
        m_bPaused = true;
        m_Sound.Pause();

        gPlugin->LogAlways( "Pause id(%d) video(%.2fs) sound(%.2fs) duration(%.2fs)", m_nVideoId, GetPosition(), m_Sound.GetPosition(), GetDuration() );
    }

    int CWebMWrapper::GetId()
    {
        return m_nVideoId;
    }

    bool CWebMWrapper::OverrideMaterial( SMaterialOverride& mOverride )
    {
#if defined(VP_DISABLE_OVERRIDE)
        return false;
#endif

        if ( !m_pCE3Tex )
        {
            return false;
        }

        // get ShaderItem
        const SShaderItem& shaderItem( mOverride.pMaterial->GetShaderItem() );

        if ( !shaderItem.m_pShaderResources )
        {
            return false;
        }

        SInputShaderResources ResTemp( shaderItem.m_pShaderResources );
        const char* sShaderName = shaderItem.m_pShader ? shaderItem.m_pShader->GetName() : NULL;

        IShader* pShader = shaderItem.m_pShader; //TODOTODO: Read Shader gen params etc
        uint64 uGenerationMask = pShader ? pShader->GetGenerationMask() : 0;

        if ( mOverride.bRecommendedSettings )
        {
            // TODO maybe sometime decide this based on tone mapping settings

            // general
            sShaderName = "Illum"; // "Monitor"

            pShader = gEnv->pRenderer->EF_LoadShader( sShaderName ); // Important to release it (else crash on exit)
            //uGenerationMask = pShader->GetGenerationMask();
            SAFE_RELEASE( pShader ); // Important else CE3 will crash on exit

            ResTemp.m_LMaterial.m_SpecShininess = 0;
            //Res.m_LMaterial.m_Specular = ColorF(0,0,0,0);
            //Res.m_LMaterial.m_Emission = ColorF(0,0,0,0);

            // "Monitor"
            //Res.m_GlowAmount = 0.5;
            //Res.m_LMaterial.m_Diffuse = ColorF(1,1,1,1);

            // Laptop Screen Wars
            //Res.m_GlowAmount = 0;
            //Res.m_LMaterial.m_Diffuse = ColorF(0.5,0.5,0.5);

            // Billboard Screen C2 "Monitor" looks best for general use imho
            ResTemp.m_GlowAmount = 0.66;
            ResTemp.m_LMaterial.m_Diffuse = ColorF( 0.5, 0.5, 0.5, 0 );
        }

        bool bRet = false;

        // if pShader
        // now replace texture slot by switching it in the sampler
        SEfResTexture* pTex = &ResTemp.m_Textures[mOverride.nTextureslot];
        STexSamplerRT* pSamp = pTex ? &pTex->m_Sampler : NULL;

        if ( m_pCE3Tex && pSamp )
        {
            pSamp->m_pITex = m_pCE3Tex;
            ULONG nrefcount = m_pCE3Tex->AddRef();

            pTex->m_TexFlags = FT_USAGE_RENDERTARGET;
            pTex->m_Name = m_sCE3Tex; // name is important

            mOverride.pMaterial->AssignShaderItem( gEnv->pRenderer->EF_LoadShaderItem( sShaderName, true, 0, &ResTemp, uGenerationMask ) );// TODOTODO Generation params pShader->GetGenerationParams()
            bRet = true;
        }

        // end if pShader

        return bRet;
    }

    void CWebMWrapper::Draw2D( S2DVideo& info )
    {
        if ( m_pCE3Tex )
        {
            m_nRendererWidth = gEnv->pRenderer->GetWidth();
            m_nRendererHeight = gEnv->pRenderer->GetHeight();

            float fWidth = 0;
            float fHeight = 0;
            float fWidthOffset = 0;
            float fHeightOffset = 0;

            float fSourceAR         = info.fCustomAR >= 0.1 ? info.fCustomAR : ( float )m_nWidth / ( float )m_nHeight;
            float fDestinationAR    = ( float )m_nRendererWidth / ( float )m_nRendererHeight;
            float fAR               = fSourceAR / fDestinationAR;

            RectF fArea;
            fArea.w = info.fRelWidth * VIRTUAL_SCREEN_WIDTH;
            fArea.h = info.fRelHeight * VIRTUAL_SCREEN_HEIGHT;
            fArea.x = info.fRelLeft * VIRTUAL_SCREEN_WIDTH;
            fArea.y = info.fRelTop * VIRTUAL_SCREEN_HEIGHT;

            switch ( info.nResizeMode )
            {
                case VRM_Original: // TODO center in area... (maybe optional parameter)
                    fWidth = m_nWidth * float( VIRTUAL_SCREEN_WIDTH / float( m_nRendererWidth ) );
                    fHeight = m_nHeight * float( VIRTUAL_SCREEN_HEIGHT / float( m_nRendererHeight ) );
                    break;

                case VRM_Stretch: // doesn't keep AR
                    fWidth = fArea.w;
                    fHeight = fArea.h;
                    break;

                case VRM_TouchInside: // Border
                    if ( fAR >= 0.99 )
                    {
                        // Calculate based on width
                        fWidth = fArea.w; // fit
                        fHeight = fArea.h * ( 1.0 / fAR ); // border
                        fHeightOffset =  ( fArea.h - fHeight ) / 2.0; // border
                    }

                    else
                    {
                        // Calculate based on height
                        fHeight = fArea.h; // fit
                        fWidth = fArea.w * fAR; // border
                        fWidthOffset = ( fArea.w - fWidth ) / 2.0; // border
                    }

                    break;

                case VRM_TouchOutside: // Crop until AR fits
                    if ( fAR >= 0.99 ) // TODO real crop by modifying texture coords (maybe optional)
                    {
                        // Calculate based on height
                        fHeight = fArea.h; // fit
                        fWidth = fArea.w * fAR; // crop
                        fWidthOffset = ( fArea.w - fWidth ) / 2.0; // crop (place outside)
                    }

                    else
                    {
                        // Calculate based on width
                        fWidth = fArea.w; // fit
                        fHeight = fArea.h * ( 1.0 / fAR ); // crop (place outside)
                        fHeightOffset =  ( fArea.h - fHeight ) / 2.0; // crop
                    }

                    break;
            }

            float fLeft = fArea.x + fWidthOffset;
            float fTop = fArea.y + fHeightOffset;

            // height and width are in virtual resolution so they are now converted and ready to use
            if ( info.cRGBA.a >= 0.01 )
            {
                bool bDrawBG = info.cBG_RGBA.a >= 0.01 && ( info.nResizeMode == VRM_TouchInside || info.nResizeMode == VRM_Original );

                if ( gVideoplayerSystem->IsGameLoopActive() && info.nZPos == VZP_AboveMenu )
                {
                    if ( bDrawBG )
                    {
                        // Draw background if enabled
                        gEnv->pRenderer->Push2dImage(
                            fArea.x,
                            fArea.y,
                            fArea.w,
                            fArea.h,
#ifdef SDK_VERSION_339
                            -1,
#else
                            gEnv->pRenderer->GetWhiteTextureId(),
#endif
                            0, 1, 1, 0,
                            info.fAngle,
                            info.cBG_RGBA.r, info.cBG_RGBA.g, info.cBG_RGBA.b, info.cBG_RGBA.a,
                            0 );
                    }

                    // Draw video
                    gEnv->pRenderer->Push2dImage(
                        fLeft,
                        fTop,
                        fWidth,
                        fHeight,
                        m_iCE3Tex,
                        0, 1, 1, 0,
                        info.fAngle,
                        info.cRGBA.r, info.cRGBA.g, info.cRGBA.b, info.cRGBA.a,
                        0 );
                }

                else
                {
                    if ( bDrawBG )
                    {
                        // Draw background if enabled
                        gEnv->pRenderer->Draw2dImage(
                            fArea.x,
                            fArea.y,
                            fArea.w,
                            fArea.h,
#ifdef SDK_VERSION_339
                            -1,
#else
                            gEnv->pRenderer->GetWhiteTextureId(),
#endif
                            0, 1, 1, 0,
                            info.fAngle,
                            info.cBG_RGBA.r, info.cBG_RGBA.g, info.cBG_RGBA.b, info.cBG_RGBA.a,
                            0 );
                    }

                    // Draw video
                    gEnv->pRenderer->Draw2dImage(
                        fLeft,
                        fTop,
                        fWidth,
                        fHeight,
                        m_iCE3Tex,
                        0, 1, 1, 0,
                        info.fAngle,
                        info.cRGBA.r, info.cRGBA.g, info.cRGBA.b, info.cRGBA.a,
                        0 );
                }
            }
        }
    }

    ITexture* CWebMWrapper::GetTexture()
    {
        return m_pCE3Tex;
    }

    float CWebMWrapper::GetEnd()
    {
        if ( !m_decoder.isOpen() )
        {
            return -1;
        }

        if ( m_decoder.m_fEndAfter > VIDEO_EPSILON )
        {
            return min( m_decoder.getDuration(), m_decoder.m_fEndAfter );
        }

        return m_decoder.getDuration();
    }

    float CWebMWrapper::GetStart()
    {
        if ( !m_decoder.isOpen() )
        {
            return -1;
        }

        if ( m_decoder.m_fStartAt > VIDEO_EPSILON )
        {
            return m_decoder.m_fStartAt;
        }

        return 0;
    }

    float CWebMWrapper::GetDuration()
    {
        return m_decoder.isOpen() ? m_decoder.getDuration() : -1;
    }

    float CWebMWrapper::GetPosition()
    {
        return m_decoder.isOpen() ? m_decoder.getPosition() : -1;
    }

    float CWebMWrapper::GetFPS()
    {
        return m_decoder.isOpen() ? m_decoder.getFPS() : 0;
    }

    unsigned CWebMWrapper::GetHeight()
    {
        return m_pCE3Tex ? m_decoder.m_nHeight : 0;
    }

    unsigned CWebMWrapper::GetWidth()
    {
        return m_pCE3Tex ? m_decoder.m_nWidth : 0;
    }

    // IVideoplayerEventListener
    void CWebMWrapper::RegisterListener( IVideoplayerEventListener* item )
    {
        vecQueue.push_back( item );
    }

    void CWebMWrapper::UnregisterListener( IVideoplayerEventListener* item )
    {
        for ( std::vector<IVideoplayerEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); iterQueue++ )
        {
            if ( ( *iterQueue ) == item )
            {
                iterQueue = vecQueue.erase( iterQueue );

                if ( iterQueue == vecQueue.end() )
                {
                    break;
                }
            }
        }
    }

    bool CWebMWrapper::Seek( float fPos )
    {
        bool bRet = ( 0 == m_decoder.seek( fPos ) );
        return bRet;
    }

    void CWebMWrapper::OnStart()
    {
        // resume playback
        Resume();

        // broadcast start event
        for ( std::vector<IVideoplayerEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnStart();
        }

        gPlugin->LogAlways( "OnStart id(%d) video(%.2fs) sound(%.2fs) duration(%.2fs)", m_nVideoId, GetPosition(), m_Sound.GetPosition(), GetDuration() );
    }

    void CWebMWrapper::OnEnd()
    {
        // pause playback
        if ( m_decoder.m_bLoop )
        {
            m_Sound.Pause();
        }

        else
        {
            Pause();
        }

        // broadcast end event
        for ( std::vector<IVideoplayerEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnEnd();
        }

        gPlugin->LogAlways( "OnEnd id(%d) video(%.2fs) sound(%.2fs) duration(%.2fs)", m_nVideoId, GetPosition(), m_Sound.GetPosition(), GetDuration() );
    }

    void CWebMWrapper::OnSeek()
    {
        // read at least one frame to get position
        bool bDirty;
        m_decoder.readFrame( NULL, bDirty, false, true );

        // reset timers
        m_fTimer = m_decoder.getPosition();
        m_fTimerNextFrame = m_fTimer - GetFrameDuration(); // forces output of next frame

        // seek synchronized sound
        m_Sound.Seek( m_fTimer );

        // broadcast seek event
        for ( std::vector<IVideoplayerEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnSeek();
        }

        // restart timer
        if ( m_eTS & VTS_SystemTime )
        {
            vpx_usec_timer_start( &m_timer );
        }

        gPlugin->LogAlways( "OnSeek id(%d) video(%.2fs) sound(%.2fs) duration(%.2fs)", m_nVideoId, GetPosition(), m_Sound.GetPosition(), GetDuration() );
    }

    float CWebMWrapper::GetFrameDuration()
    {
        if ( !( ( m_eTS | VTS_Sound ) && m_Sound.IsActive() ) )
        {
            // only modify based on speed if the time source is based on sound (but not implemented anyways)
            return 1.0f / ( m_fSpeed * m_decoder.getFPS() );
        }

        else
        {
            return 1.0f / m_decoder.getFPS();
        }
    }

    void CWebMWrapper::Advance( float fDeltaTime )
    {
        if ( m_bSkipping )
        {
            // Dispatch events to listeners
            OnEnd();
            Close();
            return;
        }

        if ( m_iCE3Tex > 0 && !m_bPaused && m_decoder.isOpen() )
        {
            // decoder is in initialized state
            float fActualDelta = 0.0f;
            float fSoundPos = 0.0f;
            bool bSoundPlaying = m_Sound.IsPlaying(); // this will also resume playback if an inconsistent play state is detected

            // set if videos should keep playing
            // (sound and game are not working in editing mode and need to be suppressed to go into callback mode)
            bool bEditorPlayback = gVideoplayerSystem->m_bEditing && gVideoplayerSystem->vp_playbackmode == VPM_KeepPlaying;

            if ( !bEditorPlayback && ( m_eTS & VTS_Sound ) && bSoundPlaying && m_fTimer > VIDEO_EPSILON && ( fSoundPos = m_Sound.GetPosition() ) > VIDEO_EPSILON )
            {
                fActualDelta = fSoundPos - m_fTimer; // no speed since pitch doesn't work on sounds...

                // needed if sound end should trigger end/loop event
                if ( fActualDelta < -VIDEO_TIMEOUT )
                {
                    goto videoend;
                }
            }

            else if ( bEditorPlayback || ( m_eTS & VTS_SystemTime ) )
            {
                vpx_usec_timer_mark( &m_timer );
                fActualDelta = float( vpx_usec_timer_elapsed( &m_timer ) ) / MICROSECOND;

                // in initialization phase of editor playback the timer first needs to be initialized
                if ( bEditorPlayback && fabs( fActualDelta ) > gVideoplayerSystem->vp_dropthreshold )
                {
                    fActualDelta = 0;
                }

                vpx_usec_timer_start( &m_timer );

                if ( !( ( m_eTS | VTS_Sound ) && m_Sound.IsActive() ) )
                {
                    fActualDelta *= m_fSpeed;
                }
            }

            else if ( !bEditorPlayback && ( m_eTS & VTS_GameTime ) )
            {
                fActualDelta = fDeltaTime;

                if ( !( ( m_eTS | VTS_Sound ) && m_Sound.IsActive() ) )
                {
                    fActualDelta *= m_fSpeed;
                }
            }

            else
            {
                fActualDelta = max( fDeltaTime, VIDEO_EPSILON ); // use game time or fall back
            }

            // Handle cases related to video end
            float fEnd = GetEnd();

            if ( fEnd > VIDEO_EPSILON )
            {
                // Somehow we shot over the end (e.g. seek/drop)
                if ( m_fTimerNextFrame > ( fEnd + VIDEO_TIMEOUT ) )
                {
                    goto videoend;
                }

                // If near the end guide video definitely to end so the end event is triggered (sound end could be different from video end or custom end)
                if ( fEnd < ( m_fTimerNextFrame + VIDEO_TIMEOUT ) && ( fActualDelta < VIDEO_EPSILON ) )
                {
                    fActualDelta = max( fDeltaTime, VIDEO_EPSILON ); // use game time or fall back
                }
            }

            m_fTimer += max( fActualDelta, 0.0f );

            // CryLogAlways(PLUGIN_CONSOLE_PREFIX "Advance id(%d) delta(%.2fs) current(%.2fs) target(%.2fs)", m_nVideoId, fActualDelta, m_fTimerNextFrame, m_fTimer);

            float fDifference = max( 0.0f, m_fTimer - m_fTimerNextFrame );

            // Which actions should be taken
            bool bNeedSeek = fDifference >= gVideoplayerSystem->vp_seekthreshold;
            bool bNeedDrop = fDifference >= gVideoplayerSystem->vp_dropthreshold;

            unsigned uFrames = 0.5f + ( fDifference / GetFrameDuration() );
            unsigned uMaxDrop = 0.5f + ( gVideoplayerSystem->vp_dropmaxduration / GetFrameDuration() );

            if ( bNeedSeek && ( m_eDM & VDM_Seek ) )
            {
                // Trigger seek
#ifdef _DEBUG
                gPlugin->LogAlways( "Advance Seek id(%d) frames(%u) diff(%.2f) current(%.2fs) target(%.2fs)",  m_nVideoId, uFrames, fDifference, m_fTimerNextFrame, m_fTimer );
#endif

                Seek( m_fTimer );
                return;
            }

            bool bDirty;

            if ( bNeedDrop && ( m_eDM & ( VDM_Drop | VDM_DropOutput ) ) )
            {
                // Trigger frame drop
#ifdef _DEBUG
                gPlugin->LogAlways( "Advance Drop id(%d) frames(%u) diff(%.2f) current(%.2fs) target(%.2fs)",  m_nVideoId, uFrames, fDifference, m_fTimerNextFrame, m_fTimer );
#endif

                // read until one frame to be rendered is left (drop data) or the max drop duration is reached
                while ( uFrames > 1 && uMaxDrop > 0 )
                {
                    //m_decoder.readFrame( NULL, bDirty, m_eDM & VDM_Drop, m_eDM & VDM_DropOutput ); // drop frame
                    m_decoder.readFrame( NULL, bDirty, false, true ); // drop frame
                    --uFrames;
                    --uMaxDrop;
                }
            }

            // output the current frame
            if ( uFrames )
            {
                vpx_image_t* img = NULL;

                // decode the next frame
                if ( !m_decoder.readFrame( &img, bDirty ) )
                {
                    m_fTimerNextFrame = m_decoder.getPosition() + GetFrameDuration(); // output next frame

                    // decoded frame needs now to be transfered into video memory
                    if ( m_VRenderer && img && bDirty )
                    {
                        m_VRenderer->RenderFrame( img ); // let the video renderer handle this
                    }
                }
            }
        }

        return;

videoend:
        OnEnd(); // dispatch events to listeners

        if ( m_decoder.m_bLoop )
        {
            Seek( 0.0f ); // seek to beginning
            OnStart(); // dispatch events to listeners
        }
    }

}
