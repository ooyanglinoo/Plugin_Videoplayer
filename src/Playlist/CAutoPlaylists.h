/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <Playlist/IVideoplayerPlaylist.h>

#pragma once

namespace VideoplayerPlugin
{
    class CAutoPlaylists :
        public IVideoplayerPlaylistEventListener
    {
        private:
            IVideoplayerPlaylist* m_pSplashScreen;
            IVideoplayerPlaylist* m_pMenu;
            IVideoplayerPlaylist* m_pMenuIngame;
            IVideoplayerPlaylist* m_pLevelLoaded;

        public:
            CAutoPlaylists();
            ~CAutoPlaylists();

            // Events
            void OnScreenChange();
            void OnSkip( bool bForce = false );
            void OnStart();
            void OnMenu( bool bInGame = false );
            void OnLevelLoaded( const char* path );

            // IVideoplayerPlaylistEventListener
            virtual void OnStartPlaylist( IVideoplayerPlaylist* pPlaylist );
            virtual void OnBeginScene( IVideoplayerPlaylist* pPlaylist, int nIndex ) {};
            virtual void OnVideoStart( IVideoplayerPlaylist* pPlaylist, IVideoplayer* pVideo ) {};
            virtual void OnVideoEnd( IVideoplayerPlaylist* pPlaylist, IVideoplayer* pVideo ) {};
            virtual void OnEndScene( IVideoplayerPlaylist* pPlaylist, int nIndex ) {};
            virtual void OnEndPlaylist( IVideoplayerPlaylist* pPlaylist );
    };
}