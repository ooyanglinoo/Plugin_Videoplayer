/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <CVideoplayerSystem.h>
#include <CPluginVideoplayer.h>
#include <WebM/CWebMWrapper.h>
#include <Playlist/CVideoplayerPlaylist.h>

VideoplayerPlugin::CVideoplayerSystem* gVideoplayerSystem = NULL;

namespace VideoplayerPlugin
{
    CVideoplayerSystem::CVideoplayerSystem()
    {
        // Reset data

        m_pAutoPlaylists = NULL;
        m_pOnlySkipFilter = NULL;
        gVideoplayerSystem = this;
        m_nFreeVideoId = 1;
        m_bEditing = false;
        m_nGameLoopActive = 5;
        m_nD3DActive = 0;
        m_fFrameTime = 0;

        SetScreenState( eSS_Initialize );
        m_bBlocked = false;

        // cvar
        vp_playbackmode = VPM_Default;

#if defined(VP_DISABLE_SYSTEM)
        return;
#endif
    }

    CVideoplayerSystem::~CVideoplayerSystem()
    {
        // Should be called while Game is still active otherwise there maybe leaks/problems

        gVideoplayerSystem = NULL;

        m_pVideos.clear();
        m_p2DVideos.clear();

        if ( gEnv && gEnv->pGameFramework && gEnv->pSystem )
        {
            // Game still active
            if ( m_pAutoPlaylists )
            {
                // Playlists avaible -> Cleanup
                delete m_pAutoPlaylists;
                m_pAutoPlaylists = NULL;
            }

            ReleaseMaterials(); // Reset/Release also the Engine Materials

            // Remove Listeners so that there are no calls into deleted system
            gEnv->pGameFramework->UnregisterListener( this );

            ISystemEventDispatcher* pDispatcher = gEnv->pSystem->GetISystemEventDispatcher();

            if ( pDispatcher )
            {
                pDispatcher->RemoveListener( this );
            }

            ILevelSystem* pLevelSystem = gEnv->pGameFramework->GetILevelSystem();

            if ( pLevelSystem )
            {
                pLevelSystem->RemoveListener( this );
            }

            IActionMapManager* pAMManager = gEnv->pGameFramework->GetIActionMapManager();

            if ( pAMManager )
            {
                pAMManager->RemoveExtraActionListener( this );
            }

            // unregister cvars
            if ( gEnv->pConsole )
            {
                gEnv->pConsole->UnregisterVariable( "vp_playbackmode", true );
                gEnv->pConsole->UnregisterVariable( "vp_seekthreshold", true );
                gEnv->pConsole->UnregisterVariable( "vp_dropthreshold", true );
                gEnv->pConsole->UnregisterVariable( "vp_dropmaxduration", true );
            }
        }
    }

    PluginManager::IPluginBase* CVideoplayerSystem::GetBase()
    {
        return static_cast<PluginManager::IPluginBase*>( gPlugin );
    };

    void CVideoplayerSystem::OnPreReset()
    {
        // Release resources because of device reset.
        for ( tVideoIDMap::const_iterator iter = m_pVideos.begin(); iter != m_pVideos.end(); ++iter )
        {
            ( ( CWebMWrapper* )( *iter ).second )->ReleaseResources();
        }

        cleanupVideoResources();
    }

    void CVideoplayerSystem::OnPostReset()
    {
        // Recreate the required resources (after device reset)
        for ( tVideoIDMap::const_iterator iter = m_pVideos.begin(); iter != m_pVideos.end(); ++iter )
        {
            ( ( CWebMWrapper* )( *iter ).second )->CreateResources();
        }
    }

    void CVideoplayerSystem::OnAction( const ActionId& action, int activationMode, float value )
    {
        // Handle the Skip actions (key defined in standard cryengine profile xml file)
        if ( action == "ui_skip_video" && activationMode == eAAM_OnPress )
        {
            // The videoplayer and playlists decide themselves if they really skip or not
            for ( tVideoPlaylistMap::const_iterator iter = m_pPlaylists.begin(); iter != m_pPlaylists.end(); ++iter )
            {
                // Skip Playlists
                ( *iter ).second->Skip( false );
            }

            for ( tVideoIDMap::const_iterator iter = m_pVideos.begin(); iter != m_pVideos.end(); ++iter )
            {
                // Skip Videos
                ( *iter ).second->Skip( false );
            }
        }
    }

    // The next functions handle parts of the screenstate
    void CVideoplayerSystem::OnActionEvent( const SActionEvent& event )
    {
        if ( event.m_event == eAE_inGame )
        {
            SetScreenState( eSS_InGameScreen );
        }

        //CryLogAlways("OActionE %d = %d (%s)", (int)event.m_event, event.m_value, event.m_description ? event.m_description : "");
    }

    void CVideoplayerSystem::OnSystemEvent( ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam )
    {
        switch ( event )
        {
            case ESYSTEM_EVENT_LEVEL_UNLOAD:
                SetScreenState( eSS_LoadingScreen );
                break;

            case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
                SetScreenState( eSS_LoadingScreen );
                break;

            case ESYSTEM_EVENT_LEVEL_LOAD_END:
                SetScreenState( eSS_InGameScreen );
                break;
        }

        //CryLogAlways("OSystemE %d w%u l(%u)", (int)event, (unsigned)wparam, (unsigned)lparam);
    }

    void CVideoplayerSystem::OnLoadingComplete( ILevel* pLevel )
    {
        if ( m_pAutoPlaylists && pLevel && !gEnv->IsEditor() )
        {
            SetScreenState( eSS_BlockedScreen );
            m_pAutoPlaylists->OnLevelLoaded( pLevel->GetLevelInfo()->GetPath() ); // trigger playlist
        }
    }

    void CVideoplayerSystem::OnLoadingProgress( ILevelInfo* pLevel, int progressAmount )
    {

    }

    void CVideoplayerSystem::OnLoadingStart( ILevelInfo* pLevel )
    {
        SetScreenState( eSS_LoadingScreen );
    }

    void CVideoplayerSystem::OnLoadGame( ILoadGame* pLoadGame )
    {
        SetScreenState( eSS_LoadingScreen );
    }

    void CVideoplayerSystem::OnLevelEnd( const char* nextLevel )
    {
        // Also cleanup materials after level end (because all materials are bound to level, there are no global materials)
        ReleaseMaterials();
    }

    bool CVideoplayerSystem::IsGameLoopActive()
    {
        return m_nGameLoopActive > 0;
    }

    bool CVideoplayerSystem::IsD3DActive()
    {
        return gD3DSystem && m_nD3DActive > 0;
    }

    void CVideoplayerSystem::AdvanceAll( float fDeltaTime )
    {
        // handle editor mode changes (and vp_playbackmode)
        if ( !m_bEditing && gEnv->IsEditor() && gEnv->IsEditing() )
        {
            // Editor is returning to Editing

            if ( vp_playbackmode == VPM_Restore )
            {
                // Reset Materials and clean up videos

                for ( tVideoPlaylistMap::const_iterator iter = m_pPlaylists.begin(); iter != m_pPlaylists.end(); ++iter )
                {
                    // Stop playlists
                    ( *iter ).second->Close();
                }

                for ( tVideoIDMap::const_iterator iter = m_pVideos.begin(); iter != m_pVideos.end(); ++iter )
                {
                    // Stop videos
                    ( *iter ).second->Close();
                }

                ReleaseMaterials();
            }

            if ( vp_playbackmode == VPM_DontRestore )
            {
                // Pause videos keep materials

                for ( tVideoPlaylistMap::const_iterator iter = m_pPlaylists.begin(); iter != m_pPlaylists.end(); ++iter )
                {
                    // Pause playlist
                    ( *iter ).second->Pause();
                }

                for ( tVideoIDMap::const_iterator iter = m_pVideos.begin(); iter != m_pVideos.end(); ++iter )
                {
                    // Pause videos
                    ( *iter ).second->Pause();
                }
            }

            m_bEditing = true;

            if ( vp_playbackmode != VPM_KeepPlaying )
            {
                return;
            }
        }

        else if ( gEnv->IsEditor() && !gEnv->IsEditing() )
        {
            // Entering Game mode in editor mode
            m_bEditing = false;
        }


        for ( tVideoIDMap::const_iterator iter = m_pVideos.begin(); iter != m_pVideos.end(); ++iter )
        {
            // Advance videos
            ( *iter ).second->Advance( fDeltaTime );
        }

        for ( tVideoPlaylistMap::const_iterator iter = m_pPlaylists.begin(); iter != m_pPlaylists.end(); ++iter )
        {
            // Advance playlists
            ( *iter ).second->Advance( fDeltaTime );
        }
    }

    void CVideoplayerSystem::DrawAll()
    {
        // cleanup is safe here
        cleanupVideoResources();

        // draw the 2d videos
        for ( t2DVideos::iterator iter = m_p2DVideos.begin(); iter != m_p2DVideos.end(); ++iter )
        {
            ( *iter ).Draw();
        }
    }

    void CVideoplayerSystem::OnPostBeginScene()
    {
        // decrement activity detectors
        if ( --m_nGameLoopActive < 0 )
        {
            m_nGameLoopActive = 0;
        }

        // This is dangerous stuff (game functions will be called from renderer thread)
        // so this is a special cvar for people who take this risk
        if ( vp_playbackmode == VPM_KeepPlaying && m_bEditing && !IsGameLoopActive() )
        {
            // fall back way to do rendering while game loop not active

            m_nD3DActive = 3;

            if ( m_fFrameTime > VIDEO_EPSILON )
            {
                AdvanceAll( m_fFrameTime );
                m_fFrameTime = 0;
            }

            else
            {
                AdvanceAll( 0 );
            }
        }

        // now thread safe (there could be some tearing but for now I don't do additional locking which would hurt performance)
        updateVideoResources( VRT_CE3 );
        updateVideoResources( VRT_DX11 );
    }

    void CVideoplayerSystem::OnPreRender()
    {
        if ( gD3DSystem )
        {
            gD3DSystem->ActivateEventDispatcher( true );    // reinstall hooks when removed
        }

        // decrement activity detectors
        if ( --m_nD3DActive < 0 )
        {
            m_nD3DActive = 0;
        }

        if ( --m_nGameLoopActive < 0 )
        {
            m_nGameLoopActive = 0;
        }

        if ( !IsGameLoopActive() )
        {
            // if game loop isn't active
            if ( !IsD3DActive() )
            {
                // and D3D also not active then also render here (fall back)
                AdvanceAll( m_fFrameTime );
                m_fFrameTime = 0;
            }

            DrawAll(); // make the draw calls (fall back)
        }
    }

    void CVideoplayerSystem::OnPostUpdate( float fDeltaTime )
    {
        bool isGamePaused = gEnv->pGameFramework->IsGamePaused();

        // Handle some additional screen states
        if ( m_bBlocked )
        {
            SetScreenState( eSS_BlockedScreen );
        }

        else if ( m_currentScreenState == eSS_Initialize && IsGameLoopActive() )
        {
            SetScreenState( eSS_StartScreen );
        }

        else if ( m_currentScreenState != eSS_Initialize && m_currentScreenState != eSS_StartScreen && !gEnv->pGameFramework->IsGameStarted() )
        {
            SetScreenState( eSS_Menu );
        }

        else if ( m_currentScreenState == eSS_InGameScreen && isGamePaused )
        {
            SetScreenState( eSS_PausedScreen );
        }

        else if ( m_currentScreenState == eSS_PausedScreen && !isGamePaused )
        {
            SetScreenState( eSS_InGameScreen );
        }

        // mark game loop as active (dx11 mode needs more since its called multiple times per frame atm)
        if ( gD3DSystem )
        {
            m_nGameLoopActive = gD3DSystem->GetType() == D3DPlugin::D3D_DX9 ? 5 : 200;
        }

        else
        {
            m_nGameLoopActive = 5;
        }

        if ( IsGameLoopActive() )
        {
            m_fFrameTime += fDeltaTime; // remember frame time for d3d

            // do the rendering here (normal way)
            AdvanceAll( m_fFrameTime );
            m_fFrameTime = 0;

            if ( !gD3DSystem )
            {
                // This is the fallback without D3D Plugin
                updateVideoResources( VRT_CE3 );
            }

            DrawAll(); // do the draw call (preferred place)
        }
    }

    void CVideoplayerSystem::OnPrePresent()
    {
        updateVideoResources( VRT_DX9 );
    }

    void CVideoplayerSystem::SetScreenState( const EScreenState state )
    {
        if ( gEnv->IsEditor() )
        {
            // In Editor only editor screen state is valid (there is no main menu/pause)
            m_currentScreenState = eSS_EditorScreen;
            return;
        }

        if ( m_currentScreenState != state )
        {
            // if the screen state changed
            EScreenState oldstate = m_currentScreenState;
            m_currentScreenState = state;

            if ( m_pAutoPlaylists )
            {
                // if auto playlist event present then trigger them according to the state change
                if ( oldstate == eSS_Initialize && state == eSS_StartScreen )
                {
                    m_pAutoPlaylists->OnStart();
                }

                else if ( state == eSS_PausedScreen )
                {
                    m_pAutoPlaylists->OnMenu( true );
                }

                else if ( oldstate == eSS_PausedScreen && state == eSS_InGameScreen )
                {
                    m_pAutoPlaylists->OnScreenChange();
                }

                else if ( state == eSS_Menu )
                {
                    m_pAutoPlaylists->OnMenu( false );
                }
            }
        }
    }

    EScreenState CVideoplayerSystem::GetScreenState() const
    {
        return m_currentScreenState;
    }
}
#include "HUD/UIMenuEvents.h"

namespace VideoplayerPlugin
{
    void CVideoplayerSystem::ShowMenu( bool bShow )
    {
        if ( gEnv && gEnv->pFlashUI && gEnv->pGameFramework )
        {
            // Scaleform UI active
            string sSystem = "";
            string sEvent = "";

            // Set the currently required flash call (for the cryengine standard menu)
            // if the menu doesn't implement those method then change those function names
            // (otherwise splash screen won't show or the whole menu might stay hidden)
            if ( !bShow )
            {
                sSystem = "MenuEvents";
                sEvent = "OnStopIngameMenu";
            }

            else
            {
                if ( gEnv->pGameFramework->IsGameStarted() )
                {
                    sSystem = "MenuEvents";
                    sEvent = "OnStartIngameMenu";
                    SetScreenState( eSS_PausedScreen );
                }

                else
                {
                    sSystem = "System";
                    sEvent = "OnSystemStarted";
                    SetScreenState( eSS_Menu );
                }
            }

            IUIEventSystem* system = gEnv->pFlashUI->GetEventSystem( sSystem, IUIEventSystem::eEST_SYSTEM_TO_UI );

            // Make the call if event system present
            if ( system )
            {
                system->SendEvent( SUIEvent( system->GetEventId( sEvent ) ) );
            }
        }
    }

    void CVideoplayerSystem::BlockGameForVideo( bool bBlock )
    {
        // Trigger a pause without going to menu to play menu in the foreground
        if ( gEnv && gEnv->pGameFramework )
        {
            if ( !gEnv->bMultiplayer && gEnv->pGameFramework->IsGameStarted() )
            {
                gEnv->pGameFramework->PauseGame( bBlock, true );
                m_bBlocked = bBlock;

                if ( m_bBlocked )
                {
                    SetScreenState( eSS_BlockedScreen );
                }

                else
                {
                    SetScreenState( eSS_InGameScreen );
                }
            }
        }
    }

    bool CVideoplayerSystem::IsGameBlocked()
    {
        return m_bBlocked;
    }

    bool CVideoplayerSystem::Initialize()
    {
#if defined(VP_DISABLE_SYSTEM)
        return false;
#endif

        // initialize the singleton system that require already initialized D3DSystem and GameFramework
        if ( gD3DSystem )
        {
            gD3DSystem->GetDevice(); // start search if isnt already found

            // register listener
            gD3DSystem->RegisterListener( this );
        }

        else
        {
            gPlugin->LogWarning( "D3D Plugin not found, using fallbacks, some features might not work or be slower" );
        }

        if ( gEnv )
        {
            // register listeners
            if ( gEnv->pGameFramework )
            {
                gEnv->pGameFramework->RegisterListener( this, "Video", eFLPriority_Menu );

                ISystemEventDispatcher* pDispatcher = gEnv->pSystem->GetISystemEventDispatcher();

                if ( pDispatcher )
                {
                    pDispatcher->RegisterListener( this );
                }

                else
                {
                    gPlugin->LogError( "ISystemEventDispatcher == NULL" );
                }

                ILevelSystem* pLevelSystem = gEnv->pGameFramework->GetILevelSystem();

                if ( pLevelSystem )
                {
                    pLevelSystem->AddListener( this );
                }

                else
                {
                    gPlugin->LogError( "ILevelSystem == NULL" );
                }

                IActionMapManager* pAMManager = gEnv->pGameFramework->GetIActionMapManager();

                if ( pAMManager )
                {
                    pAMManager->AddExtraActionListener( this );

                    // register filters //TODO
                    m_pOnlySkipFilter = gEnv->pGameFramework->GetIActionMapManager()->CreateActionFilter( "video_onlyskip", eAFT_ActionPass );

                    //m_pOnlySkipFilter->Filter("ui_skip_video");
                    //m_pOnlySkipFilter->Enable(true);
                }

                else
                {
                    gPlugin->LogError( "IActionMapManager == NULL" );
                }
            }

            else
            {
                gPlugin->LogError( "IGameFramework == NULL" );
            }

            if ( gEnv->pConsole )
            {
                // register cvars
                REGISTER_CVAR( vp_playbackmode, VPM_Default, VF_NULL, "don't restore material in editor mode (0=restore,1=pause,2=play)" );
                REGISTER_CVAR( vp_seekthreshold, SEEK_THRESHOLD, VF_NULL, "threshold in seconds after which seeks will be triggered" );
                REGISTER_CVAR( vp_dropthreshold, DROP_THRESHOLD, VF_NULL, "threshold in seconds after which drops will be triggered" );
                REGISTER_CVAR( vp_dropmaxduration, DROP_MAXDURATION, VF_NULL, "maximal duration to drop at one time before outputting a frame again" );
            }

            else
            {
                gPlugin->LogError( "IConsole == NULL" );
            }
        }

        else
        {
            gPlugin->LogError( "gEnv == NULL" );
        }

        // Auto Playlist
        m_pAutoPlaylists = new CAutoPlaylists();

        return true;
    }

    IVideoplayer* CVideoplayerSystem::CreateVideoplayer( const char* sType )
    {
        // Return a unique video id for now only WebM class
#if !defined(VP_DISABLE_DECODE)
        CWebMWrapper* pVideo = new CWebMWrapper( m_nFreeVideoId );
        m_pVideos[m_nFreeVideoId] = ( IVideoplayer* )pVideo;
        ++m_nFreeVideoId;
        return pVideo;
#endif
        return NULL;
    }

    IVideoplayer* CVideoplayerSystem::GetVideoplayerById( int nVideoID )
    {
        if ( m_pVideos.find( nVideoID ) != m_pVideos.end() )
        {
            return ( IVideoplayer* )m_pVideos[nVideoID];
        }

        else
        {
            return NULL;
        }
    }

    void CVideoplayerSystem::DeleteVideoplayer( IVideoplayer* pVideoplayer )
    {
        int nVideoID = pVideoplayer ? pVideoplayer->GetId() : -1;

        if ( m_pVideos.find( nVideoID ) != m_pVideos.end() )
        {
            delete ( CWebMWrapper* )( m_pVideos[nVideoID] );
            m_pVideos.erase( nVideoID );
        }
    }

    IVideoplayerPlaylist* CVideoplayerSystem::CreatePlaylist()
    {
        CVideoplayerPlaylist* pPlaylist = new CVideoplayerPlaylist();
        m_pPlaylists[pPlaylist] = ( IVideoplayerPlaylist* )pPlaylist;

        return pPlaylist;
    }

    void CVideoplayerSystem::DeletePlaylist( IVideoplayerPlaylist* pPlaylist )
    {
        if ( m_pPlaylists.find( pPlaylist ) != m_pPlaylists.end() )
        {
            delete ( CVideoplayerPlaylist* )( m_pPlaylists[pPlaylist] );
            m_pPlaylists.erase( pPlaylist );
        }
    }

    S2DVideo* CVideoplayerSystem::Create2DVideo()
    {
        m_p2DVideos.push_back( S2DVideo() );
        return &m_p2DVideos.back();
    }

    void CVideoplayerSystem::Delete2DVideo( S2DVideo* p2DVideo )
    {
        // iterator to vector element:
        t2DVideos::const_iterator it = std::find( m_p2DVideos.begin(), m_p2DVideos.end(), p2DVideo );

        if ( it != m_p2DVideos.end() )
        {
            m_p2DVideos.erase( it );
        }
    }

    void CVideoplayerSystem::RememberMaterial( IMaterial* mat )
    {
        if ( m_pMaterials.find( mat ) == m_pMaterials.end() )
        {
            m_pMaterials[mat] = gEnv->p3DEngine->GetMaterialManager()->CloneMaterial( mat );
            m_pMaterials[mat]->AddRef();
        }
    }

    bool CVideoplayerSystem::RestoreMaterial( IMaterial* mat, bool bResetOverride )
    {
        if ( gEnv && gEnv->pSystem && !gEnv->pSystem->IsQuitting() )
        {
            // don't cleanup if system is quitting

            if ( m_pMaterials.find( mat ) != m_pMaterials.end() )
            {
                gEnv->p3DEngine->GetMaterialManager()->CopyMaterial( m_pMaterials[mat], mat, EMaterialCopyFlags( MTL_COPY_NAME | MTL_COPY_TEXTURES | MTL_COPY_TEMPLATE ) );

                if ( bResetOverride )
                {
                    // reset/cleanup the modified material
                    for ( tOverrideMap::iterator iter = m_Overrides.begin(); iter != m_Overrides.end(); ++iter )
                    {
                        for ( tOverrideSet::iterator iterS = ( *iter ).second.begin(); iterS != ( *iter ).second.end(); ++iterS )
                        {
                            if ( ( *iterS ).pMaterial == mat )
                            {
                                ( *iterS ).Reset();
                            }
                        }
                    }
                }

                return true;
            }
        }

        return false;
    }

    void CVideoplayerSystem::ReleaseMaterials()
    {
        m_pMaterials.clear();
        m_Overrides.clear();
    }

    bool CVideoplayerSystem::ResetMaterial( IMaterial* pMaterial, int nSubmat, bool bResetOverride )
    {
        if ( !pMaterial )
        {
            return false;
        }

        if ( !( nSubmat <= 0 ) && nSubmat >= pMaterial->GetSubMtlCount() )
        {
            return false; // Submaterial invalid
        }

        // Normal or Submaterial
        IMaterial* mat = NULL;

        if ( pMaterial->GetSubMtlCount() <= 0 )
        {
            mat = pMaterial;
        }

        else
        {
            mat = pMaterial->GetSubMtl( nSubmat );
        }

        if ( !mat )
        {
            return false;    // Material not found
        }

        // Restore used material to original state
        return RestoreMaterial( mat, bResetOverride );
    }

    void CVideoplayerSystem::RestoreMaterials( IVideoplayer* pVideo, bool bResetOverride )
    {
        // find all affected materials
        for ( tOverrideMap::const_iterator iter = m_Overrides.begin(); iter != m_Overrides.end(); ++iter )
        {
            for ( tOverrideSet::const_iterator iterS = ( *iter ).second.begin(); iterS != ( *iter ).second.end(); ++iterS )
            {
                if ( ( *iterS ).pVideo == pVideo )
                {
                    RestoreMaterial( ( *iterS ).pMaterial, bResetOverride );
                }
            }
        }
    }

    void CVideoplayerSystem::OverrideMaterials( IVideoplayer* pVideo )
    {
        for ( tOverrideMap::iterator iter = m_Overrides.begin(); iter != m_Overrides.end(); ++iter )
        {
            for ( tOverrideSet::iterator iterS = ( *iter ).second.begin(); iterS != ( *iter ).second.end(); ++iterS )
            {
                if ( ( *iterS ).pVideo == pVideo )
                {
                    ( ( CWebMWrapper* )pVideo )->OverrideMaterial( *iterS );
                }
            }
        }
    }

    bool CVideoplayerSystem::OverrideMaterial( IVideoplayer* pVideo, IMaterial* pMaterial, int nSubmat, int nTextureslot, bool bRecommendedSettings )
    {
        if ( !pMaterial || !pVideo )
        {
            return false;
        }

        if ( nTextureslot < 0 || nTextureslot >= EFTT_MAX )
        {
            return false;    // Wrong textureslot
        }

        if ( !( nSubmat <= 0 ) && nSubmat >= pMaterial->GetSubMtlCount() )
        {
            return false;    // Submaterial invalid
        }

        // Normal or Submaterial
        IMaterial* mat      = NULL;

        if ( pMaterial->GetSubMtlCount() <= 0 )
        {
            mat = pMaterial;
        }

        else
        {
            mat = pMaterial->GetSubMtl( nSubmat );
        }

        if ( !mat )
        {
            return false;    // Material not found
        }

        // Remember orginal material
        RememberMaterial( mat );

        // Remember used materials
        if ( m_Overrides.find( mat ) == m_Overrides.end() )
        {
            m_Overrides.insert( std::make_pair( mat, tOverrideSet( EFTT_MAX ) ) );
        }

        SMaterialOverride& item = m_Overrides[mat][nTextureslot];
        item.Set( pVideo, mat, nTextureslot, bRecommendedSettings );

        return ( ( CWebMWrapper* )pVideo )->OverrideMaterial( item );
    }

    IMaterial* CVideoplayerSystem::CreateMaterial( IVideoplayer* pVideo, const char* sMaterial, int nMtlFlags )
    {
        IMaterial* mat = NULL;

        if ( !sMaterial || !pVideo || !gEnv || !gEnv->p3DEngine || !gEnv->p3DEngine->GetMaterialManager() )
        {
            // not possible to create
            return NULL;
        }

        mat = gEnv->p3DEngine->GetMaterialManager()->CreateMaterial( sMaterial, nMtlFlags );

        if ( mat )
        {
            OverrideMaterial( pVideo, mat ); // put video into texture slot
        }

        return mat;
    }
}