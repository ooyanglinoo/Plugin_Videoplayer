/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <CVideoplayerSystem.h>
#include "CCE3SoundWrapper.h"
#include <IMovieSystem.h>

#undef PlaySound // undefine Windows PlaySound Macro

// Macros handling various variations of fmod usage

#undef SOUNDWRAPPER_PRELOAD // Doesn't help so disable, for preloading a stream but it doesn't seem to make a difference.
#undef SOUNDWRAPPER_FMODLOOP // Doesn't help so disable, for preventing unload if a loop is played but brings to much trouble.

#if defined(SOUNDWRAPPER_FMODLOOP)
#define SOUNDFLAG_LOOP (m_bLoop ? FLAG_SOUND_LOOP : 0)
#else
#define SOUNDFLAG_LOOP 0
#endif

// always loop so sound doesn't get unloaded without stop
#if defined(SOUNDWRAPPER_PRELOAD)
#define SOUNDFLAGS(x) (x) | SOUNDFLAG_LOOP | FLAG_SOUND_LOAD_SYNCHRONOUSLY
#else
#define SOUNDFLAGS(x) (x) | SOUNDFLAG_LOOP
#endif

#define PLAYSTATE_CONSISTENT(x) (m_bPaused == !IsSoundPlaying(x)) //!< Check if playback state of sound and flag is consistent
#define POSITION_CONSISTENT(x) // TODO or maybe not since it isn't most of the time
#define DURATION_CONSISTENT(x) // TODO or maybe not since it isn't most of the time

namespace VideoplayerPlugin
{
    SSoundEntity::~SSoundEntity()
    {
        Close();
    }

    bool IsSoundPlaying( ISound* pSound )
    {
        // query combination of flags since sometimes one alone is unreliable in this fmod integration
        return pSound && pSound->IsInitialized() && pSound->IsLoaded() && pSound->IsPlaying();
    }

    bool SSoundEntity::IsPlaying()
    {
        // query combination of flags sine sometimes one alone is unreliable
        return IsActive() && pSound->IsPlaying();
    }

    bool SSoundEntity::IsActive()
    {
        // is everything required initialized and loaded?
        return IsExisting() && pSound->IsInitialized() && pSound->IsLoaded();
    }

    bool SSoundEntity::IsExisting()
    {
        // is everything required existing
        return nSoundID != INVALID_SOUNDID && pEntity && pSoundProxy && pSound;
    }

    void SSoundEntity::Seek( float fPos )
    {
        if ( pSound )
        {
            pSound->GetInterfaceExtended()->SetCurrentSamplePos( fPos * MILLISECOND, true );
#ifdef SOUNDWRAPPER_PRELOAD
            pSound->GetInterfaceExtended()->Preload();
#endif
        }
    }

    void SSoundEntity::Pause()
    {
        if ( pSoundProxy )
        {
            if ( nSoundID != INVALID_SOUNDID )
            {
                pSoundProxy->PauseSound( nSoundID, true );
            }
        }
    }

    void SSoundEntity::Close()
    {
        if ( gEnv->pSystem && !gEnv->pSystem->IsQuitting() )
        {
            // If not quitting (releasing stuff while quitting is pointless and dangerous since its going to be released automatically and might already be released)
            if ( pSoundProxy && nSoundID != INVALID_SOUNDID && gEnv->pSoundSystem->GetSound( nSoundID ) )
            {
                pSoundProxy->SetStaticSound( nSoundID, false ); // probably static sound meant or means that its always loaded
                pSoundProxy->StopSound( nSoundID, ESoundStopMode_AtOnce );
            }
        }

        // clean up internal data
        pEntity = NULL;
        pSoundProxy = NULL;
        nSoundID = INVALID_SOUNDID;
        pSound = NULL;
        vOffset = Vec3( 0, 0, 0 );
        vDirection = Vec3( 0, 0, 0 );
        nSoundFlags = 0;
        fVolume = -1;
        fMinRadius = -1;
        fMaxRadius = -1;
        eSemantic = eSoundSemantic_None;
    }

    void SSoundEntity::Resume( const char* sSoundOrEvent, float fPos )
    {
        if ( pSoundProxy )
        {
            // first pause the current sound
            if ( nSoundID != INVALID_SOUNDID )
            {
                pSoundProxy->PauseSound( nSoundID, false );
            }

            pSound = nSoundID != INVALID_SOUNDID ? pSoundProxy->GetSound( nSoundID ) : NULL;

            if ( !IsSoundPlaying( pSound ) )
            {
                // if the current sound is not playing anymore then open the new sound
#ifndef SDK_VERSION_343
                nSoundID = pSoundProxy->PlaySoundEx( sSoundOrEvent, vOffset, vDirection, nSoundFlags, fVolume, fMinRadius, fMaxRadius, eSemantic );
#else
                nSoundID = pSoundProxy->PlaySoundEx( sSoundOrEvent, vOffset, vDirection, nSoundFlags, 0, fVolume, fMinRadius, fMaxRadius, eSemantic );
#endif
                pSound = nSoundID != INVALID_SOUNDID ? pSoundProxy->GetSound( nSoundID ) : NULL;

                if ( !pSound )
                {
                    // if not successful then try again while also doing some cleanup on the proxy
                    pSoundProxy->StopAllSounds();
                    pSoundProxy->UpdateSounds();

#ifndef SDK_VERSION_343
                    nSoundID = pSoundProxy->PlaySoundEx( sSoundOrEvent, vOffset, vDirection, nSoundFlags, fVolume, fMinRadius, fMaxRadius, eSemantic );
#else
                    nSoundID = pSoundProxy->PlaySoundEx( sSoundOrEvent, vOffset, vDirection, nSoundFlags, 0, fVolume, fMinRadius, fMaxRadius, eSemantic );
#endif

                    pSound = nSoundID != INVALID_SOUNDID ? pSoundProxy->GetSound( nSoundID ) : NULL;
                }

                if ( pSound )
                {
                    pSound->GetInterfaceExtended()->SetSoundPriority( MOVIE_SOUND_PRIORITY - 1 ); // 3D sound have lower priority then 2D
#ifdef SOUNDWRAPPER_PRELOAD
                    pSound->GetInterfaceExtended()->Preload();
#endif
                }
            }

            Seek( fPos ); // seek to the position requested

            // Resume playback
            if ( nSoundID != INVALID_SOUNDID )
            {
                pSoundProxy->PauseSound( nSoundID, false );
            }
        }
    }




    CCE3SoundWrapper::CCE3SoundWrapper()
    {
        m_fSpeed = 1;
        m_fSoundDuration = -1;
        m_pSyncTimesource = NULL;
        m_bPaused = true;
        m_sSoundOrEvent = "";
        m_bLoop = false;

        m_p2DSound = NULL;
        m_b2DSoundActive = false;
        m_n2DSoundFlags = 0;
        m_f2DVolume = -1;
    }

    CCE3SoundWrapper::~CCE3SoundWrapper()
    {
        Close();
    }

    bool CCE3SoundWrapper::ReOpen()
    {
        return Open( m_sSoundOrEvent, m_pSyncTimesource, m_bLoop );
    }

    void CCE3SoundWrapper::SetSpeed( float fSpeed )
    {
        /*
        // again this doesn't work reliable in this fmod integration
        // sometimes it isn't the right speed or it seems like a random speed
        // also the positions/durations returned aren't affected at all so any kind of synchronization is then impossible
        // when you use bullet time in game with synchronized videos containing sound you might see the effects..

        if(fabs(m_fSpeed - 1) > 0.05 || fabs(fSpeed - 1) > 0.05)
        {
            m_fSpeed = fSpeed;

            if(m_p2DSound)
            {
                // Not working needs to be presampled..
                ptParamF32 param(0.0f);
                if(m_pCE3Sound->GetParam(spPITCH, &param))
                { // Seems to not work.. so couldn't test
                    param.SetValue(float(m_fSpeed-1)); //  * MILLISECOND // which unit? semitones how to calculate this.. but since its only working randomly

                    if(!m_pCE3Sound->SetParam(spPITCH, &param))
                    { // Changes pitch even if returns failure -> Reset it on failure
                        param.SetValue(0.0f);
                        m_pCE3Sound->SetParam(spPITCH, &param);
                    }
                }

                //int n = m_pCE3Sound->GetParam("pitch", &fSpeed, true);
                //m_fSpeed = fSpeed;

                ////     Gets parameter defined by index and float value.
                //float fRangeMin=NULL;
                //float fRangeMax=NULL;
                //const char* name = NULL;

                //n = 0;
                //while(true)
                //  m_pCE3Sound->GetParam(n++, &fSpeed, &fRangeMin, &fRangeMax, &name);

                //n = m_pCE3Sound->SetParam("pitch", m_fSpeed, true);

                //int n = m_pCE3Sound->SetParam("speed", m_fSpeed, true);
                //m_pCE3Sound->GetInterfaceExtended()->SetPitch(m_fSpeed * MILLISECOND);
            }
        }
        */
    }

    bool CCE3SoundWrapper::IsActive()
    {
        // At least one sound is there
        return GetPreferredSound();
    }

    bool CCE3SoundWrapper::IsPlaying()
    {
        if ( IsActive() )
        {
            // Returns true if at least one sound is playing
            ISound* sound = GetPreferredSound();

            bool bPlaying = IsSoundPlaying( sound );

            if ( !bPlaying && !m_bPaused && !m_sSoundOrEvent.empty() )
            {
                Resume(); // restore consistency
            }

            return IsSoundPlaying( sound );
        }

        return false;
    }

    void CCE3SoundWrapper::Close()
    {
        m_nPreferredDataSource = INVALID_SOUNDID;
        m_sSoundOrEvent = "";
        m_bPaused = true;
        m_fSoundDuration = -1;

        if ( gEnv && gEnv->pSystem && !gEnv->pSystem->IsQuitting() )
        {
            // If not quitting
            Set2DSound( false );
            m_SoundProxies.clear(); // TODO decide if this is ok for each close // seems so for now
        }
    }

    void CCE3SoundWrapper::Resume()
    {
        if ( m_b2DSoundActive && !m_p2DSound )
        {
            m_p2DSound = gEnv->pSystem->GetISoundSystem()->CreateSound( m_sSoundOrEvent, m_n2DSoundFlags ); // create sound without entity proxy

            if ( m_p2DSound )
            {
                m_p2DSound->SetSemantic( eSoundSemantic_HUD );
                m_p2DSound->GetInterfaceExtended()->SetSoundPriority( MOVIE_SOUND_PRIORITY ); // 2d sound gets highest priority
#ifdef SOUNDWRAPPER_PRELOAD
                m_p2DSound->GetInterfaceExtended()->Preload();
#endif
            }
        }

        if ( m_p2DSound && !IsSoundPlaying( m_p2DSound ) )
        {
            RefreshMaxDuration( m_p2DSound );
            m_p2DSound->Play( m_f2DVolume );
            m_p2DSound->GetInterfaceExtended()->SetCurrentSamplePos( m_pSyncTimesource->GetPosition() * MILLISECOND, true );
#ifdef SOUNDWRAPPER_PRELOAD
            m_p2DSound->GetInterfaceExtended()->Preload();
#endif
            m_p2DSound->SetPaused( false ); // start playback
            RefreshMaxDuration( m_p2DSound ); // retrieve duration later it becomes sometimes unreliable
        }

        for ( tEntitySoundProxyMap::iterator iterSound = m_SoundProxies.begin(); iterSound != m_SoundProxies.end(); ++iterSound )
        {
            SSoundEntity& se = iterSound->second;
            se.Resume( m_sSoundOrEvent, m_pSyncTimesource->GetPosition() );
            RefreshMaxDuration( se.pSound );
        }

        // goto position of sync target
        Seek( m_pSyncTimesource->GetPosition() );

        m_bPaused = false;
    }

    void CCE3SoundWrapper::Pause()
    {
        // pause the 2d sound
        if ( m_p2DSound )
        {
            m_p2DSound->SetPaused( true );
        }

        // pause all entity proxies
        for ( tEntitySoundProxyMap::iterator iterSound = m_SoundProxies.begin(); iterSound != m_SoundProxies.end(); ++iterSound )
        {
            iterSound->second.Pause();
        }

        m_bPaused = true;
    }

    bool CCE3SoundWrapper::Seek( float fPos )
    {
        // clamp to not jump over the end or start
        fPos = clamp( fPos, m_pSyncTimesource->GetStart(), m_pSyncTimesource->GetEnd() );

        // seek 2d Sound
        if ( m_p2DSound )
        {
            m_p2DSound->GetInterfaceExtended()->SetCurrentSamplePos( fPos * MILLISECOND, true );
#ifdef SOUNDWRAPPER_PRELOAD
            m_p2DSound->GetInterfaceExtended()->Preload();
#endif
        }

        // Seek the proxies
        for ( tEntitySoundProxyMap::iterator iterSound = m_SoundProxies.begin(); iterSound != m_SoundProxies.end(); ++iterSound )
        {
            iterSound->second.Seek( fPos );
        }

        return true;
    }

    ISound* CCE3SoundWrapper::GetPreferredSound()
    {
        // Return last preferred sound if he is still consistent, else search a new one
        ISound* pPreferredSound = NULL;
        ISound* pFallbackSound = NULL; // better then nothing...

        // last preferred source still there?
        if ( gEnv && gEnv->pSoundSystem )
        {
            pPreferredSound = gEnv->pSoundSystem->GetSound( m_nPreferredDataSource );
        }

        // is his playback state still consistent?
        if ( pPreferredSound && pPreferredSound->IsLoaded() && pPreferredSound->IsInitialized() && PLAYSTATE_CONSISTENT( pPreferredSound ) )
        {
            goto success;
        }

        // If not the check the 2d sound
        if ( Is2DSoundActive() && PLAYSTATE_CONSISTENT( m_p2DSound ) )
        {
            m_nPreferredDataSource = m_p2DSound->GetId();
            pPreferredSound = m_p2DSound;
            goto success;
        }

        pFallbackSound = m_p2DSound; // better then nothing...

        // Or one from the proxies
        for ( tEntitySoundProxyMap::iterator iterSound = m_SoundProxies.begin(); iterSound != m_SoundProxies.end(); ++iterSound )
        {
            if ( iterSound->second.IsActive() && PLAYSTATE_CONSISTENT( iterSound->second.pSound ) )
            {
                m_nPreferredDataSource = iterSound->second.pSound->GetId();
                pPreferredSound = iterSound->second.pSound;
                goto success;
            }

            // better then nothing...
            if ( !pFallbackSound && iterSound->second.IsExisting() )
            {
                pFallbackSound = iterSound->second.pSound;
            }
        }

        // if nothing reliable was found
        m_nPreferredDataSource = INVALID_SOUNDID;

        if ( pFallbackSound )
        {
            return pFallbackSound;
        }

        return NULL;
success:
        // RefreshMaxDuration( pPreferredSound );
        return pPreferredSound;
    }

    float CCE3SoundWrapper::GetDuration()
    {
        float fDuration = m_pSyncTimesource->GetDuration(); // use sync target as fall back

        if ( m_fSoundDuration >= VIDEO_EPSILON && fabs( fDuration - m_fSoundDuration ) < ( fDuration * 0.05 ) ) // If is in an 5% margin use sound length
        {
            fDuration = m_fSoundDuration; // sound duration if it matched up
        }

        return fDuration;
    }

    float CCE3SoundWrapper::GetDuration( ISound* sound )
    {
        if ( sound && sound->IsInitialized() )
        {
            int nSoundDuration = 0;

            if ( nSoundDuration = sound->GetLengthMs() )
            {
                float fSoundDuration = float( nSoundDuration ) / MILLISECOND;

                if ( fSoundDuration >= VIDEO_EPSILON ) // Use sound length if available
                {
                    return fSoundDuration;
                }
            }
        }

        return -1;
    }

    void CCE3SoundWrapper::RefreshMaxDuration( ISound* pSound )
    {
        // store duration which is most like sync target / the longest
        float fDuration = GetDuration( pSound );

        if ( fDuration > m_fSoundDuration )
        {
            m_fSoundDuration = fDuration;

            if ( pSound && pSound->IsLoaded() && pSound->IsInitialized() && PLAYSTATE_CONSISTENT( pSound ) )
            {
                m_nPreferredDataSource = pSound->GetId(); // also set this as preferred source since duration ok
            }
        }
    }

    float CCE3SoundWrapper::GetPosition()
    {
        ISound* sound = GetPreferredSound();

        if ( sound )
        {
            unsigned int nSoundPos = sound->GetInterfaceExtended()->GetCurrentSamplePos( true );

            if ( nSoundPos )
            {
                // successfully got a position
                float fSoundPos = float( nSoundPos ) / MILLISECOND;

                // make loops possible so use float modulo with sync target end (loops in this FMOD integration don't reset position)
                float fEnd = m_pSyncTimesource->GetEnd();

                if ( fEnd >= VIDEO_EPSILON )
                {
                    bool bSeek = fSoundPos > fEnd;
                    fSoundPos = fmod( fSoundPos, fEnd );

                    if ( bSeek )
                    {
                        // if soundpos is beyond end then seek to normal starting position again
                        fSoundPos += m_pSyncTimesource->GetStart();
                        Seek( fSoundPos );
                    }
                }

                return fSoundPos;
            }
        }

        return -1;
    }

    bool CCE3SoundWrapper::Open( const char* sSoundOrEvent, IMediaTimesource* pSyncTimesource, bool bLoop )
    {
        Close(); // close already open sound, if present

        m_pSyncTimesource = pSyncTimesource;
        m_sSoundOrEvent = sSoundOrEvent;
        m_bLoop = bLoop;

        return true;
    }

    void CCE3SoundWrapper::AddSoundProxy( IEntity* pEntity, const Vec3 vOffset, const Vec3 vDirection, uint32 nSoundFlags, float fVolume, float fMinRadius, float fMaxRadius, ESoundSemantic eSemantic )
    {
        if ( pEntity )
        {
            if ( m_SoundProxies.find( pEntity ) == m_SoundProxies.end() )
            {
                // If entity proxy doesn't already exists, add it to our proxy<->entity association map
                m_SoundProxies.insert( std::make_pair( pEntity, SSoundEntity() ) );
                tEntitySoundProxyMap::iterator iter = m_SoundProxies.find( pEntity );

                if ( iter != m_SoundProxies.end() )
                {
                    // if this worked initialize the sound proxy
                    SSoundEntity& se = iter->second;

                    se.pSoundProxy = ( IEntitySoundProxy* )pEntity->GetProxy( ENTITY_PROXY_SOUND );

                    if ( !se.pSoundProxy )
                    {
                        se.pSoundProxy = ( IEntitySoundProxy* )pEntity->CreateProxy( ENTITY_PROXY_SOUND );
                    }

                    // set infos for resume
                    se.pSound       = NULL;
                    se.pEntity      = pEntity;
                    se.vOffset      = vOffset;
                    se.vDirection   = vDirection;
                    se.nSoundFlags  = SOUNDFLAGS( nSoundFlags | FLAG_SOUND_START_PAUSED ); // internally always start paused
                    se.fVolume      = fVolume;
                    se.fMinRadius   = fMinRadius;
                    se.fMaxRadius   = fMaxRadius;
                    se.eSemantic    = eSemantic;

                    if ( !m_bPaused )
                    {
                        // if was already playing then resume now to start playing this proxy
                        Resume();
                    }
                }
            }
        }
    }

    void CCE3SoundWrapper::RemoveSoundProxy( IEntity* pEntity = NULL )
    {
        tEntitySoundProxyMap::iterator iter = m_SoundProxies.find( pEntity );

        if ( iter != m_SoundProxies.end() )
        {
            // proxy was found so delete it

            if ( iter->second.pSound && m_nPreferredDataSource == iter->second.pSound->GetId() )
            {
                // this proxy was the current preferred source

                m_nPreferredDataSource = INVALID_SOUNDID; // in the next call a new preferred source will be automatically selected
            }

            m_SoundProxies.erase( iter );
        }
    }

    bool CCE3SoundWrapper::Is2DSoundActive()
    {
        // query combination of flags sine sometimes one alone is unreliable
        return m_p2DSound && m_p2DSound->IsLoaded() && m_p2DSound->IsInitialized();
    }

    void CCE3SoundWrapper::Set2DSound( bool bActivate, float fVolume, uint32 nSoundFlags )
    {
        if ( gEnv->pSystem && !gEnv->pSystem->IsQuitting() )
        {
            // if system present and not quitting

            if ( m_p2DSound && !bActivate )
            {
                // sound is to be deactivated, so stop it

                if ( m_nPreferredDataSource == m_p2DSound->GetId() )
                {
                    // the 2d sound was the current preferred source
                    m_nPreferredDataSource = INVALID_SOUNDID; // in the next call a new preferred source will be automatically selected
                }

                m_p2DSound->Stop( ESoundStopMode_AtOnce );
                m_p2DSound = NULL;
            }

            m_b2DSoundActive = bActivate;

            if ( bActivate )
            {
                // set infos for resume
                m_n2DSoundFlags = SOUNDFLAGS( nSoundFlags | FLAG_SOUND_START_PAUSED ); // internally always start paused
                m_f2DVolume     = fVolume;

                if ( m_b2DSoundActive && !m_bPaused )
                {
                    // if was already playing then resume now to start playing this proxy
                    Resume();
                }
            }
        }
    }

}
