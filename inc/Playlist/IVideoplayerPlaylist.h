/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#pragma once

#include <IPluginVideoplayer.h>

namespace VideoplayerPlugin
{
    struct IVideoplayerPlaylist;

    /**
    * @ingroup vp_interface
    * @brief Listener Interface for video player playlist events dispatched by a playlist
    * @warning Don't interfere with the stream/playlist itself if you don't know what you're doing. (e.g. close, seeks etc might cause infinite loops or the video might be closed already). Instead queue those events and apply those actions in the next frame.
    */
    struct IVideoplayerPlaylistEventListener
    {
        /**
        * @brief Playlist start event (playlists starts playing first frame of the first video)
        * @param pPlaylist affected playlist
        */
        virtual void OnStartPlaylist( IVideoplayerPlaylist* pPlaylist ) = 0;

        /**
        * @brief Scene start event (playlists starts playing first frame of the first video of the current scene)
        * @param pPlaylist affected playlist
        * @param nIndex affected scene
        */
        virtual void OnBeginScene( IVideoplayerPlaylist* pPlaylist, int nIndex ) = 0;

        /**
        * @brief Video start event (playlists starts playing first frame of the video)
        * @param pPlaylist affected playlist
        * @param pVideo affected video
        */
        virtual void OnVideoStart( IVideoplayerPlaylist* pPlaylist, IVideoplayer* pVideo ) = 0;

        /**
        * @brief Video end event (video plays last frame)
        * @param pPlaylist affected playlist
        * @param pVideo affected video
        */
        virtual void OnVideoEnd( IVideoplayerPlaylist* pPlaylist, IVideoplayer* pVideo ) = 0;

        /**
        * @brief Scene end event (playlist plays last frame of the current scene)
        * @param pPlaylist affected playlist
        * @param nIndex affected scene
        */
        virtual void OnEndScene( IVideoplayerPlaylist* pPlaylist, int nIndex ) = 0;

        /**
        * @brief Playlist end event (playlists reached end of the last video of the last scene)
        * @param pPlaylist affected playlist
        */
        virtual void OnEndPlaylist( IVideoplayerPlaylist* pPlaylist ) = 0;
    };

    /**
    * @ingroup vp_interface
    * @brief Playlist specific interface
    */
    struct IVideoplayerPlaylist : public IMediaPlayback
    {
        /**
        * @brief Open a playlist
        * The playlist must be in xml format.
        * @return success
        * @param sPlaylist Relative Path inside the Game folder (e.g. inside pak file or extracted)
        * @param bLoop Should the playlist be automatically repeated after its end is reached.
        * @param bSkippable Should the scenes be skippable by the user.
        * @param bBlockGame Should the game be paused during playback of the playlist
        * @param nStartAtScene Start playback/loop at specific scene, Default -1
        * @param fEndAtScene End playback/loop at specific scene, Default -1
        */
        virtual bool Open( const char* sPlaylist, bool bLoop = false, bool bSkippable = true, bool bBlockGame = false, int nStartAtScene = -1, int nEndAtScene = -1 ) = 0;

        /**
        * @brief Advances the scenes
        * @param deltaTime Delta in Seconds (time passed since last frame)
        * @warning This function is normally called automatically
        */
        virtual void Advance( float deltaTime ) = 0;

        /**
        * @brief Seeks the playlist to a position and/or scene (multiple videos can be affected)
        * @return success
        * @param scene target scene index
        * @param fPos absolute position in seconds (inside the target scene)
        */
        virtual bool Seek( const int scene, float fPos ) = 0;

        /**
        * @brief Get the number of scenes inside the playlist
        * @return number of scenes inside the playlist
        */
        virtual int GetSceneCount() = 0;

        /**
        * @brief Get the pointer to a videoplayer inside the current scene
        * @return interface pointer to the videoplayer or NULL
        * @param nIndex index of the video inside the current scene
        */
        virtual IVideoplayer* GetSceneVideoplayer( int nIndex ) = 0;

        /**
        * @brief Get the pointer to a videoplayer inside the current scene
        * @return success
        * @param nIndex index of the video inside the current scene
        */
        virtual int GetSceneVideoplayerCount() = 0;

        /**
        * @brief Register listener to this playlist event dispatcher
        * @param item interface pointer of listener
        */
        virtual void RegisterListener( IVideoplayerPlaylistEventListener* item ) = 0;

        /**
        * @brief Unregisters listener from this playlist event dispatcher
        * @param item interface pointer of listener
        */
        virtual void UnregisterListener( IVideoplayerPlaylistEventListener* item ) = 0;
    };
}