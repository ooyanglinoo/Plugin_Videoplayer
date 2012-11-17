/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <Game.h>
#include <IPluginVideoplayer.h>

#pragma once

namespace VideoplayerPlugin
{
    /**
    * @brief Holds information about a entity proxy
    */
    struct SSoundEntity : public IEntityEventListener
    {
        private:
            IEntity* pEntity; //!< affected entity
            IEntitySoundProxy* pSoundProxy; //!< current soundproxy for this sound and entity
            tSoundID nSoundID; //!< sound id of current sound
            ISound* pSound; //! sound interface pointer

            Vec3 vOffset; //!< offset from the entity
            Vec3 vDirection; //!< sound direction
            uint32 nSoundFlags; //!< sound flags of this sound proxy
            float fVolume; //!< overall volume of this sound
            float fMinRadius; //!< minimal radius this sound can be heared from (at full volume)
            float fMaxRadius; //!< maximum radius in meters this sound proxy can be heared from (at low volume)
            ESoundSemantic eSemantic; //!< sound semantic of this proxy
            bool bEntityListenerInstalled; //!< entity listener present

        public:
            SSoundEntity() :
                pEntity( NULL ),
                pSoundProxy( NULL ),
                nSoundID( INVALID_SOUNDID ),
                pSound( NULL ),
                vOffset( Vec3( 0, 0, 0 ) ),
                vDirection( Vec3( 0, 0, 0 ) ),
                nSoundFlags( 0 ),
                fVolume( -1 ),
                fMinRadius( -1 ),
                fMaxRadius( -1 ),
                eSemantic( eSoundSemantic_None ),
                bEntityListenerInstalled( false )
            {};

            ~SSoundEntity();

            /**
            * @brief Soundproxy is playing
            * @return playing
            */
            bool IsPlaying();

            /**
            * @brief Soundproxy is active/present
            * @return active
            */
            bool IsActive();

            /**
            * @brief Soundproxy is at least existent
            * @return existent
            */
            bool IsExisting();

            /**
            * @brief Seek this soundproxy
            * @param fPos Position in seconds
            */
            void Seek( float fPos );

            /**
            * @brief Resume playback of a sound
            * @param sSoundOrEvent path to sound or fmod event name
            * @param fPos position in seconds
            */
            void Resume( const char* sSoundOrEvent, float fPos );

            /**
            * @brief stops the sound
            */
            void Close();

            /**
            * @brief pauses the currently playing sound
            */
            void Pause();

            /**
            * @brief returns the associated sound interface
            */
            ISound* GetSound()
            {
                return pSound;
            };

            /**
            * @brief Used to handle entity deletion (called automatically from EntitySystem)
            * @param pEntity pEntity
            * @param event event
            */
            void OnEntityEvent( IEntity* pEntity, SEntityEvent& event )
            {
                if ( event.event == ENTITY_EVENT_DONE )
                {
                    this->bEntityListenerInstalled = false;
                    this->pEntity = NULL;
                    this->pSoundProxy = NULL;
                }
            }

            /**
            * @brief Set/Initialize all parameters
            * @param _pEntity Entity this Sound is attached to
            * @param _vOffset Pos Offset
            * @param _vDirection Sound Direction
            * @param _nSoundFlags Sound Flags for FMOD
            * @param _fVolume Sound Volume
            * @param _fMinRadius Min Radius sound can be heard from
            * @param _fMaxRadius Max Radius sound can be heard from
            * @param _eSemantic Sound Semantic
            * @return void
            */
            void Set( IEntity* _pEntity, Vec3 _vOffset, Vec3 _vDirection, uint32 _nSoundFlags, float _fVolume, float _fMinRadius, float _fMaxRadius, ESoundSemantic _eSemantic );
    };

    /**
    * @brief type for entity proxy association
    */
    typedef std::map<IEntity*, SSoundEntity> tEntitySoundProxyMap;

    /**
    * @brief Wrapper for the CE3 Fmod API
    * Used to work around the unreliable fmod integration and to synchronize multiple sound playbacks to a synchronization target.
    * @see the interfaces for details
    * @attention its recommended to use fmod events (mp3, mp2, ogg don't work always
    */
    class CCE3SoundWrapper :
        public ISoundplayer
    {
        public:
            CCE3SoundWrapper();
            ~CCE3SoundWrapper();

            // IMediaPlayback
            virtual bool ReOpen();
            virtual void SetSpeed( float fSpeed = 1.0f );
            virtual bool IsPlaying();
            virtual bool IsActive();

            virtual void Close();
            virtual void Resume();
            virtual void Pause();
            virtual bool Seek( float fPos );
            virtual void Skip( bool bForce = false ) {};

            // IMediaTimesource
            virtual float GetStart()
            {
                return 0;
            };

            virtual float GetEnd()
            {
                return GetDuration();
            };

            virtual float GetDuration();
            virtual float GetPosition();

            // ISoundplayer
            virtual bool Open( const char* sSoundOrEvent, IMediaTimesource* pSyncTimesource, bool bLoop = false );
            virtual void AddSoundProxy( IEntity* pEntity, const Vec3 vOffset = Vec3( ZERO ), const Vec3 vDirection = FORWARD_DIRECTION, uint32 nSoundFlags = FLAG_SOUND_3D | FLAG_SOUND_DEFAULT_3D, float fVolume = 1, float fMinRadius = 0, float fMaxRadius = 100, ESoundSemantic eSemantic = eSoundSemantic_Living_Entity );
            virtual void RemoveSoundProxy( IEntity* pEntity );
            virtual void Set2DSound( bool bActivate = false, float fVolume = 1, uint32 nSoundFlags = FLAG_SOUND_MOVIE | FLAG_SOUND_2D );
            virtual bool Is2DSoundActive();

        private:
            /**
            * @brief Get a reliable information source
            * since a sound can play on 2d anr or multiple entities its needed to get one information source which is deemed as reliable.
            * cause for this is again the unreliable fmod integration.
            * @return reliable sound or NULL
            */
            ISound* GetPreferredSound();

            /**
            * @brief Stores the largest duration for this sound
            * needed because of unreliable fmod integration
            * @param sound Sound interface to retrieve duration from
            */
            void RefreshMaxDuration( ISound* sound );

            /**
            * @brief Get the durston
            * @param Sound Interface to retrieve duration from
            * @return duration in seconds
            */
            float GetDuration( ISound* sound );

            IMediaTimesource* m_pSyncTimesource; //!< sound synchronization target
            tSoundID m_nPreferredDataSource; //!< currently most reliable sound information source

            bool m_b2DSoundActive; //!< 2D sound currently acitve?

            ISound* m_p2DSound; //!< 2d sound interface
            uint32 m_n2DSoundFlags; //!< 2d sound flags
            float m_f2DVolume; //!< stored 2d volume

            tEntitySoundProxyMap m_SoundProxies; //!< all entitiy sound proxies
            float m_fSpeed; //!< speed of the sound (not implemented)
            bool m_bPaused; //!< currently playing or not
            string m_sSoundOrEvent; //!< sound path or fmod event name

            float m_fSoundDuration; //!< stored duration of the sound
            bool m_bLoop; //!< looped sound
    };
};
