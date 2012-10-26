/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <Game.h>
#include <IPluginVideoplayer.h>

#include <CVideoplayerSystem.h>
#include <WebM/vpxdec_ext.h>
#include <Sound/CCE3SoundWrapper.h>
#include <Renderer/CVideoRenderer.h>

#pragma once

namespace VideoplayerPlugin
{
    /**
    * @brief Wrapper for WebM Decoder implementing interfaces
    * @see the interfaces for details
    */
    class CWebMWrapper :
        public IVideoplayer,
        private IVideoplayerEventListener
    {
        private:
            std::vector<IVideoplayerEventListener*>     vecQueue; //!< Event listeners
            float GetFrameDuration(); //!< Frametime (1 / FPS)

        public:
            CWebMWrapper( int nVideoId );
            ~CWebMWrapper();

            virtual int GetId();
            bool ReleaseResources( bool bResetOverride = false );
            bool CreateResources();

            // IMediaPlayback
            virtual bool ReOpen();
            virtual void SetSpeed( float fSpeed = 1.0f );
            virtual void Skip( bool bForce = false );
            virtual bool IsPlaying();
            virtual bool IsActive();
            virtual void Close();
            virtual void Resume();
            virtual void Pause();
            virtual bool Seek( float fPos );

            // IMediaTimesource
            virtual float GetStart();
            virtual float GetDuration();
            virtual float GetPosition();
            virtual float GetEnd();

            // IVideoplayer
            virtual bool Open( const char* sFile, const char* sSound = "", bool bLoop = false, bool bSkippable = true, bool bBlockGame = false, eTimeSource eTS = VTS_Default, eDropMode eDM = VDM_Default, float fStartAt = 0, float fEndAfter = 0, int nCustomWidth = -1, int nCustomHeight = -1 );
            virtual void SetTimesource( eTimeSource eTS = VTS_Default );
            virtual bool OverrideMaterial( SMaterialOverride& mOverride );
            virtual void Draw2D( S2DVideo& info );
            virtual ITexture* GetTexture();
            virtual float GetFPS();
            virtual unsigned GetHeight();
            virtual unsigned GetWidth();
            virtual ISoundplayer* GetSoundplayer();

            // IVideoplayerEventListener
            virtual void RegisterListener( IVideoplayerEventListener* item );
            virtual void UnregisterListener( IVideoplayerEventListener* item );
#define BROADCAST_EVENT(METHOD) \
    virtual void METHOD() { \
        for(std::vector<IVideoplayerEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue!=vecQueue.end(); ++iterQueue) \
            (*iterQueue)->METHOD(); \
    }

            BROADCAST_EVENT( OnFrame );
            virtual void OnSeek();
            virtual void OnStart();
            virtual void OnEnd();

            virtual void Advance( float deltaTime );

        private:
            bool m_bSkippable; //!< video can skip
            bool m_bSkipping; //!< vide is currently skipping

            int m_iCE3Tex; //!< CE3 texture id
            ITexture* m_pCE3Tex; //!< CE3 texture interface
            string m_sCE3Tex; //!< CE3 texture name

            CCE3SoundWrapper m_Sound; //!< Sound output of this video

            IVideoRenderer* m_VRenderer; //!< Video render for this video

            int m_nVideoId; //!< video id
            bool m_bPaused; //!< currently paused
            VPXDec m_decoder; //!< libvpx decoder
            vpx_usec_timer m_timer; //!< system time timer

            float m_fTimer; //!< internal timer
            float m_fTimerNextFrame; //!< the next frame should be displayed at this time
            float m_fSpeed; //! the speed of the video

            int  m_nWidth,
                 m_nHeight;

            int  m_nRendererWidth,
                 m_nRendererHeight;

            eTimeSource m_eTS; //!< time source to be used
            eDropMode m_eDM; //!< active drop mode
    };
}
