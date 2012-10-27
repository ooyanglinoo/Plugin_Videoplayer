/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#pragma once

#include <IPluginBase.h>
//
#include <IRenderer.h>

#define AUTOPLAY_SPLASHSCREEN "Videos/Auto_Start.xml" //!< Automatically triggered playlist on game startup (splashscreen)
#define AUTOPLAY_MENU "Videos/Auto_Menu.xml" //!< Automatically triggered playlist while the main menu is displayed. (menu back and foreground)
#define AUTOPLAY_MENU_INGAME "Videos/Auto_Menu_InGame.xml" //!< Automatically triggered playlist while the menu is displayed while in game. (menu back and foreground)
#define AUTOPLAY_LEVEL "Auto_Video.xml" //!< Automatically triggered playlist for each level loaded (Path is the same as the level/map that is loaded)

#define RESBASE 2 //!< Shift resolution by x in this case make resolution divisible by 4 / best however would be 4 -> 16
#define NANOSECOND 1000000000.0f //!< Nanoseconds for internal usage 10 ^ 9
#define MICROSECOND 1000000.0f //!< Microseconds for internal usage  10 ^ 6
#define MILLISECOND 1000.0f //!< Milliseconds for internal usage 10 ^ 3
#define VIDEO_EPSILON 0.015f //!< Minimal duration to detect time differences in seconds
#define VIDEO_TIMEOUT 0.5f //!< Needed to handle problematic timing situations (e.g. video ending or timers becoming unreliable)

#define DROP_MAXDURATION 0.1f //!< Drop at most x seconds before outputing again one frame
#define DROP_THRESHOLD 0.1f //!< Start dropping frames when detecting at least 100ms lag
#define SEEK_THRESHOLD 5.0f //!< Start seeking when detecting at least 5s lag

// CryEngine internal stuff that was just exposed in version 3.4
// for backward compatibility defines those values here
#ifndef R_DX11_RENDERER
#define SDK_VERSION_339 1 //!< Further special handling of old SDK releases.
#define VIRTUAL_SCREEN_WIDTH 800.0f //!< Width of the virtual screen, inside the plugin relative sizes are used.
#define VIRTUAL_SCREEN_HEIGHT 600.0f //!< Height of the virtual screen, inside the plugin relative sizes are used.
#endif

/**
* @brief Videoplayer Plugin Namespace
*/
namespace VideoplayerPlugin
{
    /**
    * @brief resize modes for 2D output
    */
    enum eResizeMode
    {
        VRM_Original = 0, //!< Keep orginal resolution and aspect ratio (not recommended)
        VRM_Stretch = 1, //!< Stretch the video to fit the screen, aspect ratio is not kept.
        VRM_TouchInside = 2, //!< Keep aspect ratio and touch the specified rectangle inside.
        VRM_TouchOutside = 3, //!< Keep aspect ratio and touch the specified rectangle from the outside.
        VRM_Default = VRM_TouchInside, //!< current default setting
    };

    /**
    * @brief Z-Position for 2D output
    */
    enum eZPos
    {
        VZP_BehindMenu = 0, //!< Draw video behind the UI
        VZP_AboveMenu = 1, //!< Draw video infront of the UI
        VZP_Default = VZP_BehindMenu, //!< current default setting
    };

    /**
    * @brief Time source types for media playback
    * Each of those time sources have different uses choose the correct one or your
    * media might be paused
    * @see eDropMode
    */
    enum eTimeSource
    {
        VTS_GameTime = 1, //!< Use game time (supporting pause, bullettime)
        VTS_Sound = 2, //!< Use sound synchronization (supporting pause)
        VTS_SystemTime = 4, //!< Use system time (supporting playback during pause)
        VTS_SoundOrGameTime = VTS_GameTime | VTS_Sound, //!< Use soundsource and fallback to game time if problems occur
        VTS_SoundOrSystemTime = VTS_SystemTime | VTS_Sound, //!< Use soundsource and fallback to system time if problems occur
        VTS_Live = 8, //!< Display data as fast as it comes in (supporting playback during pause)
        VTS_DefaultPlaylist = VTS_SoundOrSystemTime, //!< Current default playlist setting
        VTS_Default = VTS_SoundOrGameTime, //!< Current default setting
    };

    /**
    * @brief Handles Editor Video Playback
    * Please note Flownode events are not active when not in game mode.
    * Because of this the video will stop after the last frame is reached if not looped.
    * this is globally set by the cvar: vp_playbackmode: 0/1
    */
    enum ePlaybackMode
    {
        VPM_Restore = 0, //!< Restore materials to their orginal state when exiting game mode (new default mode)
        VPM_DontRestore = 1, //!< Don't restore materials when exiting the game mode
        VPM_KeepPlaying = 2, //!< Don't restore materials and keep videos playing when exiting the game mode (old default mode) (this mode is dangerous and resource intensive, so use it wisely)
        VPM_Default = VPM_Restore, //!< Current default setting
    };

    /**
    * @brief Drop Mode to handle synchronization
    * @see eTimeSource
    */
    enum eDropMode
    {
        VDM_None = 0, //!< No synchronization possible
        VDM_Drop = 1, //!< Drop Frames if small time differences are detected (artifacts can appear)
        VDM_Seek = 2, //!< Seek if larger time difference is detected (audio seek is also triggered)
        VDM_DropOutput = 4, //!< Multiple decode calls until no difference is detected (postprocessing and output for these frames is disabled). will slow down game even further  to keep sync.
        VDM_DropOrSeek = VDM_Drop | VDM_Seek, //!< Good combination for not so important videos that can show artifacts (old default mode)
        VDM_DropOutputOrSeek = VDM_DropOutput | VDM_Seek, //!< Good combination for important videos (new default mode)
        VDM_Live = 8, //!< Display most current frame
        VDM_Default = VDM_DropOutputOrSeek, //!< Current default setting
    };

    /**
    * @ingroup vp_interface
    * @brief Listener Interface for videoplayer events dispatched by a videoplayer
    * @warning Don't interfere with the stream itself if you don't know what you're doing. (e.g. close, seeks etc might cause infinite loops or the video might be closed already) Instead queue those events and apply those actions in the next frame.
    */
    struct IVideoplayerEventListener
    {
        /**
        * @brief Start event (video plays first frame)
        */
        virtual void OnStart() = 0;

        /**
        * @brief A frame is played (called for each frame)
        */
        virtual void OnFrame() = 0;

        /**
        * @brief A seek occurred (manual, automatic because of loop or bad performance > sync with timesource)
        */
        virtual void OnSeek() = 0;

        /**
        * @brief The end was reached (video plays last frame)
        */
        virtual void OnEnd() = 0;
    };

    /**
    * @ingroup vp_interface
    * @brief Time source interface for video and audio player classes and also for synchronization targets.
    */
    struct IMediaTimesource
    {
        /**
        * @brief Optional start position (0 if not set)
        * @return Start position in seconds
        */
        virtual float GetStart() = 0;

        /**
        * @brief stream position (-1 if unavailable)
        * @return position in seconds
        */
        virtual float GetPosition() = 0;

        /**
        * @brief stream duration (-1 if unavailable)
        * @return duration in seconds
        */
        virtual float GetDuration() = 0;

        /**
        * @brief stream end position (-1 if unavailable)
        * @return end position in seconds (can be less then duration)
        */
        virtual float GetEnd() = 0;
    };

    /**
    * @ingroup vp_interface
    * @brief Time source interface for video and audio player classes and also for synchronization targets.
    */
    struct IMediaPlayback : public IMediaTimesource
    {
        /**
        * @brief Set playback speed
        * This parameter is only available if the video is not synchronized to sound time sources,
        * the reason for this is because the sound playback speed can not be set in a always working manner in CE3.
        * @param fSpeed Speed as factor (1 means normal 0.5 means half speed and 2 means double speed)
        */
        virtual void SetSpeed( float fSpeed = 1.0f ) = 0;

        /**
        * @brief Skip to the end of chapter/video/scene
        * This function will be called automatically by the videoplayer system when the user triggers the skip action.
        * The skip action can be defined in the standard cryengine xml file.
        * @param bForce Will force the media to be skipped even if it is otherwise parameterized (overrides init parameters)
        */
        virtual void Skip( bool bForce = false ) = 0;

        /**
        * @brief Opens the media with the exact same parameters as the last open call,
        * @todo sometimes not yet implemented
        */
        virtual bool ReOpen() = 0;

        /**
        * @brief Get the playback status
        * @return true if playing, false if paused or closed
        */
        virtual bool IsPlaying() = 0;

        /**
        * @brief Get the media status
        * @return true if open, false if closed/error
        */
        virtual bool IsActive() = 0;

        /**
        * @brief Closes the currently opened media
        */
        virtual void Close() = 0;

        /**
        * @brief Resumes playback if the media is currently paused or not yet started
        */
        virtual void Resume() = 0;

        /**
        * @brief Pauses the playback of the currently opened media.
        */
        virtual void Pause() = 0;

        /**
        * @brief Seeks the media to a position
        * @param absolute position in seconds
        */
        virtual bool Seek( float fPos ) = 0;
    };

    /**
    * @ingroup vp_interface
    * @brief Sound playback specific interface
    * This is an wrapper to provide a more reliable way to play a sound
    * that can be synchronized to a video.
    */
    struct ISoundplayer : public IMediaPlayback
    {
        /**
        * @brief Opens a sound file/event.
        * This function wont directly open the sound. The sound will only be opened if the sound player is in playing state and at least one
        * sound proxy is present or the 2D sound is set. On each frame there is a retry to open/resume the sound.
        * @param sSoundOrEvent Path to sound in file system/pak or the fmod sound event.
        * @param pSyncTimesource pointer to a video this sound should be synchronized to. (needed for duration, start/end position)
        * @param bLoop Should the sound be automatically repeated after its end is reached.
        */
        virtual bool Open( const char* sSoundOrEvent, IMediaTimesource* pSyncTimesource, bool bLoop = false ) = 0;

        /**
        * @brief Add one sound proxy entity that should also play this sound.
        * Much of those parameters heavily depend on the way the sound is set up in fmod.
        * @param pEntity The entity that should act as sound proxy.
        * @param vOffset Relative offset from that entity. Default is 0,0,0.
        * @param vDirection Direction of the sound. Default is forward.
        * @param nSoundFlags Flags for Fmod, the flags might be modified by the sound player. Default is 3D,
        * @param fVolume Volume the sound should be played at. Default is 1
        * @param fMinRadius Minimal radius the sound should be heard from. Default is 0.
        * @param fMaxRadius Maximum radius the sound should be heard at. Default is 100.
        * @param eSemantic Semantic flag the sound should be played with. Default is living entity.
        */
        virtual void AddSoundProxy( IEntity* pEntity, const Vec3 vOffset = Vec3( ZERO ), const Vec3 vDirection = FORWARD_DIRECTION, uint32 nSoundFlags = FLAG_SOUND_3D | FLAG_SOUND_DEFAULT_3D, float fVolume = 1, float fMinRadius = 0, float fMaxRadius = 100, ESoundSemantic eSemantic = eSoundSemantic_Living_Entity ) = 0;

        /**
        * @brief Removes the Sound proxy of the specific entity
        */
        virtual void RemoveSoundProxy( IEntity* pEntity ) = 0;

        /**
        * @brief Set the 2d Mode of the sound
        * 2D Mode doesn't rely on sound proxies
        * @param bActivate Activates the 2D Sound.
        * @param fVolume Volume the sound should be played at. Default is 1
        * @param nSoundFlags Flags for Fmod. Default is Movie 2D.
        */
        virtual void Set2DSound( bool bActivate = false, float fVolume = 1, uint32 nSoundFlags = FLAG_SOUND_MOVIE | FLAG_SOUND_2D ) = 0;

        /**
        * @brief Is the 2D Mode currently activated.
        * @return true if 2D Mode active
        */
        virtual bool Is2DSoundActive() = 0;
    };

    struct S2DVideo_;

    /**
    * @ingroup vp_interface
    * @brief Video playback specific interface
    * This is an wrapper to provide a more reliable way to play a sound
    * that can be synchronized to a video.
    */
    struct IVideoplayer : public IMediaPlayback
    {
        /**
        * @brief Get the video identifier
        * @return unique video identifier
        */
        virtual int GetId() = 0;

        /**
        * @brief Open a video stream
        * Use in combination with Resume to start playing a video.
        * @return success
        * @param sFile Relative Path inside the Game folder (e.g. inside pak file or extracted)
        * @param sSoundOrEvent Path to sound in file system/pak or the fmod sound event.
        * @param bLoop Should the media be automatically repeated after its end is reached.
        * @param bSkippable Should the media be skippable by the user.
        * @param bBlockGame Should the game be paused during playback of the media
        * @param eTS Time source mode
        * @param fStartAt Start playback/loop at specific position in seconds, Default 0
        * @param fEndAfter End playback/loop at specific position in seconds, Default 0
        * @param nCustomWidth Custom Width for render target (might not be used depending on renderer), Default -1
        * @param nCustomHeight Custom Height for render target (might not be used depending on renderer), Default -1
        */
        virtual bool Open( const char* sFile, const char* sSoundOrEvent, bool bLoop = false, bool bSkippable = true, bool bBlockGame = false, eTimeSource eTS = VTS_Default, eDropMode eDM = VDM_Default, float fStartAt = 0, float fEndAfter = 0, int nCustomWidth = -1, int nCustomHeight = -1 ) = 0;

        /**
        * @brief Set time source for media
        * @param eTS time source mode
        */
        virtual void SetTimesource( eTimeSource eTS = VTS_Default ) = 0;

        /**
        * @brief Advances the position and renders the video frame
        * @param deltaTime Delta in Seconds (time passed since last frame)
        * @warning This function is normally called automatically
        */
        virtual void Advance( float deltaTime ) = 0;

        /**
        * @brief Get Sound player interface
        * @return Interface to current sound player interface, NULL if not present
        */
        virtual ISoundplayer* GetSoundplayer() = 0;

        /**
        * @brief Get frames per seconds
        * @return frames per seconds
        */
        virtual float GetFPS() = 0;

        /**
        * @brief Get height in pixels
        * @return video height in pixels
        */
        virtual unsigned GetHeight() = 0;

        /**
        * @brief Get width in pixels
        * @return video width in pixels
        */
        virtual unsigned GetWidth() = 0;

        /**
        * @brief Get render target interface
        * @return Interface to current render target/texture interface, NULL if not present
        */
        virtual ITexture* GetTexture() = 0;

        /**
        * @brief Draw the video
        * @param info Parameters for the 2D draw action
        */
        virtual void Draw2D( S2DVideo_& info ) = 0;

        /**
        * @brief Register listener to this videoplayer event dispatcher
        * @param item Interface pointer of listener
        */
        virtual void RegisterListener( IVideoplayerEventListener* item ) = 0;

        /**
        * @brief Unregisters listener from this videoplayer event dispatcher
        * @param item Interface pointer of listener
        */
        virtual void UnregisterListener( IVideoplayerEventListener* item ) = 0;
    };

    /**
    * @ingroup vp_interface
    * @brief 2D Drawing specific structure
    * Structure for all 2D specific settings to be used together with the videoplayer interface.
    * Use the videoplayer system to instantiate this object.
    * If you manually create this object you have to handle the drawing yourself. (not recommended)
    * @warning Changes require recompilation of the plugin link library. (ABI incompatibilities)
    */
    typedef struct S2DVideo_
    {
        public:
            IVideoplayer* pVideo; //!< interface pointer of the associated media player
            eResizeMode nResizeMode; //!< resize mode
            eZPos nZPos; //!< Z-Position for drawing
            float fCustomAR; //!< custom aspect ratio to be used
            float fRelTop; //!< top screen position of the media frame (relative between 0.0 and 1.0)
            float fRelLeft; //!< left screen position of the media frame (relative between 0.0 and 1.0)
            float fRelWidth; //!< screen width of the media frame (relative between 0.0 invisible and 1.0 fullscreen)
            float fRelHeight; //!< screen height of the media frame (relative between 0.0 invisible and 1.0 fullscreen)
            float fAngle; //!< rotate media frame (angle 0 - 360)
            ColorF cRGBA; //!< Foreground Color of the media frame, Default 1,1,1,1 (white opaque)
            ColorF cBG_RGBA; //!< Background Color behind the media frame, Default is 0,0,0,0 (black invisible)

        private:
            bool bSoundSource; //!< 2D output requires 2D sound output

        public:
            /**
            * @brief Default constructor resets parameters
            */
            S2DVideo_()
            {
                Reset();
            };

            /**
            * @brief Destructor disables sound if this 2D output did require sound.
            */
            ~S2DVideo_()
            {
                if ( bSoundSource )
                {
                    SetSoundsource( false );
                }
            };

            /**
            * @brief Resets all parameters to defaults
            */
            void Reset()
            {
                pVideo = NULL;
                bSoundSource = false;
                nResizeMode = VRM_Default;
                nZPos = VZP_Default;
                fCustomAR = 0;
                fRelTop = 0;
                fRelLeft = 0;
                fRelWidth = 1;
                fRelHeight = 1;
                fAngle = 0;
                cRGBA = ColorF( 1, 1, 1, 1 );
                cBG_RGBA = ColorF( 0, 0, 0, 0 );
            };

            /**
            * @brief Operator only compares the pointers.
            */
            bool operator== ( const struct S2DVideo_* r ) const
            {
                return this == r;
            };

            /**
            * @brief Set the associated video
            * @param _pVideo Interfacepointer of the media.
            */
            void SetVideo( IVideoplayer* _pVideo )
            {
                if ( bSoundSource && pVideo )
                {
                    pVideo->GetSoundplayer()->Set2DSound( false );
                }

                pVideo = _pVideo;

                if ( bSoundSource && pVideo && !pVideo->GetSoundplayer()->Is2DSoundActive() )
                {
                    pVideo->GetSoundplayer()->Set2DSound( true );
                }
            }

            /**
            * @brief Set this 2D parameters as 2D sound source
            * @param _bSoundSource Activate/disable this parameters as 2D sound source.
            */
            void SetSoundsource( bool _bSoundSource )
            {
                if ( pVideo )
                {
                    if ( bSoundSource && !_bSoundSource )
                    {
                        pVideo->GetSoundplayer()->Set2DSound( false );
                    }

                    else if ( !bSoundSource && _bSoundSource && !pVideo->GetSoundplayer()->Is2DSoundActive() )
                    {
                        pVideo->GetSoundplayer()->Set2DSound( true );
                    }
                }

                bSoundSource = _bSoundSource;
            }

            /**
            * @brief Draw the associated video with the current parameters.
            */
            void Draw()
            {
                if ( pVideo )
                {
                    pVideo->Draw2D( *this );
                }
            };
    } S2DVideo;

    struct IVideoplayerPlaylist;

    /**
    * @ingroup vp_interface
    * @brief plugin Videoplayer concrete interface
    * Structure for all 2D specific settings to be used together with the videoplayer interface.
    */
    struct IPluginVideoplayer
    {
        /**
        * @brief Get Plugin base interface
        */
        virtual PluginManager::IPluginBase* GetBase() = 0;

        /**
        * @brief Create a videoplayer class
        * @param sType Type currently only WebM supported
        * @return Videoplayer interface of the class
        */
        virtual IVideoplayer* CreateVideoplayer( const char* sType = "WebM" ) = 0;

        /**
        * @brief Get videoplayer interface from an unique video identifier.
        * @param nVideoID unique video identifier
        * @return Videoplayer interface of the class, NULL if not found
        */
        virtual IVideoplayer* GetVideoplayerById( int nVideoID = -1 ) = 0;

        /**
        * @brief Delete/Close a videoplayer class
        * @param pVideoplayer interface pointer of the videoplayer class to be deleted.
        */
        virtual void DeleteVideoplayer( IVideoplayer* pVideoplayer ) = 0;

        /**
        * @brief Create 2D Output object
        * 2D Outputs created this way are managed by the videoplayer system and don't need to be drawn manually.
        * Delete them only using Delete2DVideo.
        * @return Pointer to the 2D Output object
        */
        virtual S2DVideo* Create2DVideo() = 0;

        /**
        * @brief Delete 2D Output object
        * @param p2DVideo Pointer to the object to be deleted.
        */
        virtual void Delete2DVideo( S2DVideo* p2DVideo ) = 0;

        /**
        * @brief Create a media playlist
        * @param bShowMenuOnEndDefault Show the menu when the playlist ends defaultvalue
        * @return Pointer to a media playlist interface
        */
        virtual IVideoplayerPlaylist* CreatePlaylist( bool bShowMenuOnEndDefault = false ) = 0;

        /**
        * @brief Delete a media playlist
        * @param pPlaylist Pointer to the object to be deleted.
        */
        virtual void DeletePlaylist( IVideoplayerPlaylist* pPlaylist ) = 0;

        /**
        * @brief Remember material inside internal material cache
        * This is required for restoring materials to their original state.
        * @param mat Material to be remembered
        */
        virtual void RememberMaterial( IMaterial* mat ) = 0;

        /**
        * @brief Restore a cached material to their original state.
        * @return success
        * @param mat Material to be restored
        * @param bResetOverride also remove the modified material
        * @see ResetMaterial
        * @attention Resets all overrides in this material (e.g. videos on different texture slots need to be restored manually)
        */
        virtual bool RestoreMaterial( IMaterial* mat, bool bResetOverride = false ) = 0;

        /**
        * @brief Create a new material showing a video
        * @return created material or NULL
        * @param pVideo Video to be shown
        * @param sMaterial Material name
        * @param nMtlFlags Material flags (can be override by the videoplayer plugin)
        */
        virtual IMaterial* CreateMaterial( IVideoplayer* pVideo, const char* sMaterial, int nMtlFlags = 0 ) = 0;

        /**
        * @brief Override material with a video
        * @return success
        * @param pVideo Video to be shown
        * @param pMaterial Material to be overridden
        * @param nSubmat Sub material slot to be overridden
        * @param nTextureslot Texture slot to be overridden
        * @param bRecommendedSettings Sets shader to illum and set parameters (best practice is to optimize the shader and parameters manually depending on the tod/scene)
        */
        virtual bool OverrideMaterial( IVideoplayer* pVideo, IMaterial* pMaterial, int nSubmat = 0, int nTextureslot = EFTT_DIFFUSE, bool bRecommendedSettings = true ) = 0;

        /**
        * @brief Restore Material
        * @return success
        * @param pMaterial Material to be restored
        * @param nSubmat Sub material slot to be restored
        * @param bResetOverride also remove the modified material
        * @see RestoreMaterial
        * @attention Resets all overrides in this material (e.g. videos on different texture slots need to be restored manually)
        */
        virtual bool ResetMaterial( IMaterial* pMaterial, int nSubmat = 0, bool bResetOverride = false ) = 0;

        /**
        * @brief Restore all cached materials overridden by the video to their original state.
        * @param pVideo video that should be removed from the materials
        * @param bResetOverride also remove the modified material
        * @attention Resets all overrides in this material (e.g. videos on different texture slots need to be restored manually)
        */
        virtual void RestoreMaterials( IVideoplayer* pVideo, bool bResetOverride = false ) = 0;

        /**
        * @brief Override all cached materials associated to the video player (e.g. after reset)
        * @param pVideo video associated to the materials
        */
        virtual void OverrideMaterials( IVideoplayer* pVideo ) = 0;
    };
};
