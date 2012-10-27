/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <CVideoplayerSystem.h>
#include <Playlist/CAutoPlaylists.h>

namespace VideoplayerPlugin
{
    CAutoPlaylists::CAutoPlaylists()
    {
        m_pSplashScreen = gVideoplayerSystem->CreatePlaylist( true ); // Only for Playlist the default is to show the menu
        m_pMenu = gVideoplayerSystem->CreatePlaylist();
        m_pMenuIngame = gVideoplayerSystem->CreatePlaylist();
        m_pLevelLoaded = gVideoplayerSystem->CreatePlaylist();
    }

    CAutoPlaylists::~CAutoPlaylists()
    {
        if ( gVideoplayerSystem && gEnv && gEnv->pSystem && !gEnv->pSystem->IsQuitting() )
        {
            if ( m_pSplashScreen )
            {
                gVideoplayerSystem->DeletePlaylist( m_pSplashScreen );
            }

            if ( m_pMenu )
            {
                gVideoplayerSystem->DeletePlaylist( m_pMenu );
            }

            if ( m_pMenuIngame )
            {
                gVideoplayerSystem->DeletePlaylist( m_pMenuIngame );
            }

            if ( m_pLevelLoaded )
            {
                gVideoplayerSystem->DeletePlaylist( m_pLevelLoaded );
            }
        }
    }

    void CAutoPlaylists::OnSkip( bool bForce )
    {
        if ( m_pSplashScreen )
        {
            m_pSplashScreen->Skip( bForce );
        }

        if ( m_pMenu )
        {
            m_pMenu->Skip( bForce );
        }

        if ( m_pMenuIngame )
        {
            m_pMenuIngame->Skip( bForce );
        }

        if ( m_pLevelLoaded )
        {
            m_pLevelLoaded->Skip( bForce );
        }
    }

    void CAutoPlaylists::OnScreenChange()
    {
        if ( m_pSplashScreen )
        {
            //  m_pSplashScreen->UnregisterListener(this);
            m_pSplashScreen->Close();
        }

        if ( m_pMenu )
        {
            m_pMenu->Close();
        }

        if ( m_pMenuIngame )
        {
            m_pMenuIngame->Close();
        }

        if ( m_pLevelLoaded )
        {
            m_pLevelLoaded->Close();
        }
    }

    void CAutoPlaylists::OnStart()
    {
        OnScreenChange();

        if ( m_pSplashScreen )
        {
            if ( m_pSplashScreen->Open( AUTOPLAY_SPLASHSCREEN ) )
            {
                gVideoplayerSystem->ShowMenu( false );
                m_pSplashScreen->Resume();
                m_pSplashScreen->RegisterListener( this );
            }

            else
            {
                OnMenu( false );
            }
        }
    }

    void CAutoPlaylists::OnStartPlaylist( IVideoplayerPlaylist* pPlaylist )
    {}

    void CAutoPlaylists::OnEndPlaylist( IVideoplayerPlaylist* pPlaylist )
    {
        if ( pPlaylist == m_pSplashScreen && gVideoplayerSystem->GetScreenState() == eSS_StartScreen )
        {
            OnMenu( false );
        }
    }

    void CAutoPlaylists::OnMenu( bool bInGame )
    {
        OnScreenChange();

        if ( m_pMenu && !bInGame )
        {
            m_pMenu->Open( AUTOPLAY_MENU );
            m_pMenu->Resume();
        }

        else if ( m_pMenuIngame && bInGame )
        {
            m_pMenuIngame->Open( AUTOPLAY_MENU_INGAME );
            m_pMenuIngame->Resume();
        }
    }

    void CAutoPlaylists::OnLevelLoaded( const char* path )
    {
        OnScreenChange();

        if ( m_pLevelLoaded )
        {
            string sPath = path;
            sPath += "/";
            sPath += AUTOPLAY_LEVEL;

            m_pLevelLoaded->Open( sPath.c_str() );
            m_pLevelLoaded->Resume();
        }
    }
}