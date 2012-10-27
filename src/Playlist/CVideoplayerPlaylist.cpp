/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <CPluginVideoplayer.h>
#include <CVideoplayerSystem.h>
#include <Playlist/CVideoplayerPlaylist.h>
#include <sstream>
#include <ios>

// No NULL pointer instead empty string
#define SSTRING(text) (text?text:"")

namespace VideoplayerPlugin
{

    bool isempty( char* str )
    {
        while ( *str != 0 )
            if ( !isspace( *str++ ) )
            {
                return false;
            }

        return true;
    }

#define STRING_TRUE "true"
#define STRING_FALSE "false"
    bool ToBool( const char* c )
    {
        bool value = false;

        // "1" and "true" / "0" and "false" as false
        string sValue = c;

        if ( sValue == "1" || sValue == STRING_TRUE )
        {
            value = true;
        }

        else if ( sValue == "0" || sValue == STRING_FALSE )
        {
            value = false;
        }

        return value;
    }

    template<typename T>
    T SGetAttr( IXmlNode* node, const char* sAttr, T tDefault )
    {
        T tRet = tDefault;

        if ( node->haveAttr( sAttr ) )
        {
            char* pTest = SSTRING( node->getAttr( sAttr ) );

            if ( !isempty( pTest ) )
            {
                node->getAttr( sAttr, tRet );
            }
        }

        return tRet;
    }

    template<>
    string SGetAttr<string>( IXmlNode* node, const char* sAttr, string sDefault )
    {
        string sRet = sDefault;

        if ( node->haveAttr( sAttr ) )
        {
            char* sTest = SSTRING( node->getAttr( sAttr ) );

            XmlString xmlString;

            if ( !isempty( sTest ) )
            {
                node->getAttr( sAttr, xmlString );
                sRet = xmlString;
            }
        }

        return sRet;
    }

    template<>
    bool SGetAttr( IXmlNode* node, const char* sAttr, bool bDefault )
    {
        bool bRet = bDefault;

        if ( node->haveAttr( sAttr ) )
        {
            char* pTest = SSTRING( node->getAttr( sAttr ) );

            if ( !isempty( pTest ) )
            {
                bRet = ToBool( pTest );
            }
        }

        return bRet;
    }

#define XML_SCENE "scene"
#define XML_INPUT "input"
#define XML_OUTPUT "output"

#define XML_LOOP "loop"
#define XML_SKIPPABLE "skippable"
#define XML_BLOCKGAME "blockgame"
#define XML_CLASS "class"
#define XML_SOUNDSOURCE "soundsource"
#define XML_RESIZEMODE "resizemode"
#define XML_CUSTOMAR "customar"
#define XML_TOP "top"
#define XML_LEFT "left"
#define XML_WIDTH "width"
#define XML_HEIGHT "height"
#define XML_ANGLE "angle"
#define XML_RGBA "rgba"
#define XML_BACKGROUNDRGBA "backgroundrgba"
#define XML_ZORDER "zorder"

#define XML_VIDEO "video"
#define XML_SOUND "sound"

#define XML_SPEED "speed"
#define XML_STARTAT "startat"
#define XML_ENDAFTER "endafter"
#define XML_CUSTOMWIDTH "customwidth"
#define XML_CUSTOMHEIGHT "customheight"
#define XML_TIMESOURCE "timesource"
#define XML_DROPMODE "dropmode"

    CVideoplayerPlaylist::CVideoplayerPlaylist()
    {
        m_xmlPlaylist = NULL;
        Close();
    }

    CVideoplayerPlaylist::~CVideoplayerPlaylist()
    {
        Close();
    }

    void CVideoplayerPlaylist::QueueEvent( SVideoEvent::eVideoEventType type, IVideoplayer*  pVideo )
    {
        m_qVideoEvents.push( SVideoEvent( type, pVideo ) );
    }

    void CVideoplayerPlaylist::OnStartPlaylist()
    {
        for ( std::vector<IVideoplayerPlaylistEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnStartPlaylist( this );
        }

        gPlugin->LogAlways( "Playlist OnStart file(%s) scenes(%d)", m_sFile.c_str(), m_iSceneCount );
    }

    void CVideoplayerPlaylist::OnBeginScene( int nIndex )
    {
        for ( std::vector<IVideoplayerPlaylistEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnBeginScene( this, nIndex );
        }

#if defined(_DEBUG)
        gPlugin->LogAlways( "Playlist OnBeginScene file(%s) scenes(%d) scene(%d)", m_sFile.c_str(), m_iSceneCount, nIndex );
#endif
    }

    void CVideoplayerPlaylist::OnVideoStart( IVideoplayer* pVideo )
    {
        if ( m_bStart )
        {
            OnStartPlaylist();
            m_bStart = false;
        }

        if ( m_bSceneStart )
        {
            OnBeginScene( m_iScene );
            m_bSceneStart = false;
        }

        for ( std::vector<IVideoplayerPlaylistEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnVideoStart( this, pVideo );
        }

#if defined(_DEBUG)
        gPlugin->LogAlways( "Playlist OnVideoStart file(%s) scenes(%d) scene(%d) id(%d)", m_sFile.c_str(), m_iSceneCount, m_iScene, pVideo->GetId() );
#endif
    }

    void CVideoplayerPlaylist::OnVideoEnd( IVideoplayer* pVideo )
    {
        for ( std::vector<IVideoplayerPlaylistEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnVideoEnd( this, pVideo );
        }

#if defined(_DEBUG)
        gPlugin->LogAlways( "Playlist OnVideoEnd file(%s) scenes(%d) scene(%d) id(%d)", m_sFile.c_str(), m_iSceneCount, m_iScene, pVideo->GetId() );
#endif
    }

    void CVideoplayerPlaylist::OnEndScene( int nIndex )
    {
        for ( std::vector<IVideoplayerPlaylistEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnEndScene( this, nIndex );
        }

#if defined(_DEBUG)
        gPlugin->LogAlways( "Playlist OnEndScene file(%s) scenes(%d) scene(%d)", m_sFile.c_str(), m_iSceneCount, nIndex );
#endif

        if ( m_CurrentScene.bLoop )
        {
            m_iScene = m_iScene > 0 ? --m_iScene : 0;
        }

        if ( !readNextScene() )
        {
            OnEndPlaylist();
        }
    }

    void CVideoplayerPlaylist::OnEndPlaylist()
    {
        m_CurrentScene.reset();

        for ( std::vector<IVideoplayerPlaylistEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); ++iterQueue )
        {
            ( *iterQueue )->OnEndPlaylist( this );
        }

#if defined(_DEBUG)
        gPlugin->LogAlways( "Playlist OnEnd file(%s) scenes(%d)", m_sFile.c_str(), m_iSceneCount );
#endif
    }

    SVideoInput::SVideoInput()
    {
        pVideo = NULL;
        pPlaylist = NULL;
        reset();
    }

    SVideoInput::~SVideoInput()
    {
        reset();
    }

    void SVideoInput::reset()
    {
        sClass = "";
        sVideo = "";
        sSound = "";

        fStartAt = 0.0f;
        fEndAfter = 0.0f;
        fSpeed = 1.0f;

        bLoop = false;
        bSkippable = true;
        bBlockGame = false;

        nCustomWidth = -1;
        nCustomHeight = -1;

        eTS = VTS_DefaultPlaylist;
        eDM = VDM_Default;

        if ( pVideo )
        {
            pVideo->UnregisterListener( this );
            gVideoplayerSystem->DeleteVideoplayer( pVideo );
        }

        pVideo = NULL;
        pPlaylist = NULL;
    }

    SSceneInput::~SSceneInput()
    {
        reset();
    }

    void SSceneInput::reset()
    {
        for ( std::vector<S2DVideo*>::iterator iter = v2DOutputs.begin(); iter != v2DOutputs.end(); ++iter )
        {
            gVideoplayerSystem->Delete2DVideo( *iter );
        }

        v2DOutputs.clear();
        input.reset();
    }

    SScene::~SScene()
    {
        reset();
    }

    void SScene::reset()
    {
        bSkippable = true;
        bLoop = false;
        fDuration = 0;
        vInputs.clear();
    }

    void SVideoInput::OnStart()
    {
        if ( pPlaylist && pVideo )
        {
            pPlaylist->QueueEvent( SVideoEvent::OnStart, pVideo );
        }
    }

    void SVideoInput::OnEnd()
    {
        if ( pPlaylist && pVideo )
        {
            pPlaylist->QueueEvent( SVideoEvent::OnEnd, pVideo );
        }
    }

    bool SVideoInput::init( XmlNodeRef xmlInput, CVideoplayerPlaylist* pPlaylist )
    {
        bool bRet = false;

        reset();

        if ( xmlInput != NULL && xmlInput->isTag( XML_INPUT ) )
        {
            sClass = SGetAttr<string>( xmlInput, XML_CLASS, string( "inputwebm" ) );
            sVideo = SGetAttr<string>( xmlInput, XML_VIDEO, string( "" ) );
            sSound = SGetAttr<string>( xmlInput, XML_SOUND, string( "" ) );

            fStartAt = SGetAttr( xmlInput, XML_STARTAT, 0.0f );
            fEndAfter = SGetAttr( xmlInput, XML_ENDAFTER, 0.0f );
            fSpeed = SGetAttr( xmlInput, XML_SPEED, 1.0f );

            bLoop = SGetAttr( xmlInput, XML_LOOP, false );

            bSkippable = SGetAttr<int>( xmlInput, XML_SKIPPABLE, true );
            bBlockGame = SGetAttr<int>( xmlInput, XML_BLOCKGAME, false );

            nCustomWidth = SGetAttr( xmlInput, XML_WIDTH, -1 );
            nCustomHeight = SGetAttr( xmlInput, XML_HEIGHT, -1 );

            eTS = eTimeSource( SGetAttr<int>( xmlInput, XML_TIMESOURCE, VTS_DefaultPlaylist ) );
            eDM = eDropMode( SGetAttr<int>( xmlInput, XML_DROPMODE, VDM_Default ) );

            this->pPlaylist = pPlaylist;
            pVideo = gVideoplayerSystem->CreateVideoplayer();

            if ( pVideo && pVideo->Open( sVideo, sSound, bLoop, bSkippable, bBlockGame, eTS, eDM, fStartAt, fEndAfter, nCustomWidth, nCustomHeight ) )
            {
                pVideo->RegisterListener( this );
                bRet = true;
            }
        }

        return bRet;
    }

    ColorF convColor( const ColorB& src )
    {
        ColorF ret;

        ret.a = src.a / 255.0f;
        ret.r = src.r / 255.0f;
        ret.g = src.g / 255.0f;
        ret.b = src.b / 255.0f;

        return ret;
    }

    bool SSceneInput::init( XmlNodeRef xmlInput, CVideoplayerPlaylist* pPlaylist )
    {
        bool bRet = false;
        v2DOutputs.clear();

        if ( xmlInput != NULL && xmlInput->isTag( XML_INPUT ) )
        {
            bRet = input.init( xmlInput, pPlaylist );

            if ( bRet && input.pVideo )
            {
                int iOutputCount = xmlInput->getChildCount();

                for ( int iOutput = 0; iOutput < iOutputCount; ++iOutput )
                {
                    XmlNodeRef xmlOutput = xmlInput->getChild( iOutput );

                    if ( xmlOutput != NULL && xmlOutput->isTag( XML_OUTPUT ) )
                    {
                        S2DVideo* pVideo = gVideoplayerSystem->Create2DVideo();

                        if ( pVideo )
                        {
                            pVideo->SetVideo( input.pVideo );
                            pVideo->SetSoundsource( SGetAttr( xmlOutput, XML_SOUNDSOURCE, true ) );
                            pVideo->nResizeMode = eResizeMode( SGetAttr<int>( xmlOutput, XML_RESIZEMODE, VRM_Default ) );
                            pVideo->fCustomAR = SGetAttr( xmlOutput, XML_CUSTOMAR, 0.0f );
                            pVideo->fRelTop = SGetAttr( xmlOutput, XML_TOP, 0.0f );
                            pVideo->fRelLeft = SGetAttr( xmlOutput, XML_LEFT, 0.0f );
                            pVideo->fRelWidth = SGetAttr( xmlOutput, XML_WIDTH, 1.0f );
                            pVideo->fRelHeight = SGetAttr( xmlOutput, XML_HEIGHT, 1.0f );
                            pVideo->fAngle = SGetAttr( xmlOutput, XML_ANGLE, 0.0f );
                            pVideo->cRGBA = convColor( SGetAttr<ColorB>( xmlOutput, XML_RGBA, Col_White ) );
                            pVideo->cBG_RGBA = convColor( SGetAttr<ColorB>( xmlOutput, XML_BACKGROUNDRGBA, Col_Black ) );
                            pVideo->nZPos = eZPos( SGetAttr<int>( xmlOutput, XML_ZORDER, VZP_Default ) );
                            v2DOutputs.push_back( pVideo );
                        }
                    }
                }
            }
        }

        return bRet;
    }

    bool SScene::init( XmlNodeRef xmlScene, CVideoplayerPlaylist* pPlaylist )
    {
        bool bRet = false;
        reset();

        if ( xmlScene != NULL && xmlScene->isTag( XML_SCENE ) )
        {
            bSkippable = SGetAttr( xmlScene, XML_SKIPPABLE, true );
            bLoop = SGetAttr( xmlScene, XML_LOOP, false );

            int iInputCount = xmlScene->getChildCount();

            for ( int iInput = 0; iInput < iInputCount; ++iInput )
            {
                vInputs.push_back( SSceneInput() );
                bRet = vInputs.back().init( xmlScene->getChild( iInput ), pPlaylist );

                if ( !bRet )
                {
                    break;
                }
            }

            fDuration = 0; // TODO: duration
        }

        return bRet;
    }

    void SScene::Skip( bool bForce )
    {
        if ( bForce || bSkippable )
        {
            for ( std::vector<SSceneInput>::iterator iter = vInputs.begin(); iter != vInputs.end(); ++iter )
            {
                if ( iter->input.pVideo )
                {
                    iter->input.pVideo->Skip( bForce );
                }
            }
        }
    }

    void SScene::Resume()
    {
        for ( std::vector<SSceneInput>::iterator iter = vInputs.begin(); iter != vInputs.end(); ++iter )
        {
            if ( iter->input.pVideo )
            {
                iter->input.pVideo->Resume();
            }
        }
    }

    void SScene::Pause()
    {
        for ( std::vector<SSceneInput>::iterator iter = vInputs.begin(); iter != vInputs.end(); ++iter )
        {
            if ( iter->input.pVideo )
            {
                iter->input.pVideo->Pause();
            }
        }
    }

    bool SScene::IsPlaying()
    {
        for ( std::vector<SSceneInput>::iterator iter = vInputs.begin(); iter != vInputs.end(); ++iter )
        {
            if ( iter->input.pVideo && iter->input.pVideo->IsPlaying() )
            {
                return true;
            }
        }

        return false;
    }

    bool SScene::IsActive()
    {
        for ( std::vector<SSceneInput>::iterator iter = vInputs.begin(); iter != vInputs.end(); ++iter )
        {
            if ( iter->input.pVideo && iter->input.pVideo->IsActive() )
            {
                return true;
            }
        }

        return false;
    }

    bool SScene::Seek( float fPos )
    {
        bool bRet = true;

        for ( std::vector<SSceneInput>::iterator iter = vInputs.begin(); iter != vInputs.end(); ++iter )
        {
            if ( iter->input.pVideo )
            {
                bRet &= iter->input.pVideo->Seek( fPos );
            }
        }

        return bRet;
    }

    bool CVideoplayerPlaylist::readNextScene()
    {
        bool bRet = false;

        if ( m_xmlPlaylist != NULL )
        {
            m_qVideoEvents.empty(); // clear events videos will be invalid after init

            if ( m_iScene < m_iSceneCount )
            {
                XmlNodeRef xmlScene = m_xmlPlaylist->getChild( m_iScene++ );
                bRet = m_CurrentScene.init( xmlScene, this );
            }

            if ( !bRet )
            {
                Close();
            }

            else
            {
                m_bSceneStart = true;

                if ( !m_bPaused )
                {
                    Resume();
                }
            }
        }

        return bRet;
    }

    bool CVideoplayerPlaylist::Open( const char* sPlaylist, bool bLoop, bool bSkippable, bool bBlockGame, int nStartAtScene, int nEndAtScene )
    {
        Close();
        m_bLoop = bLoop;
        m_bSkippable = bSkippable;
        m_bBlockGame = bBlockGame;
        m_sFile = sPlaylist;
        m_xmlPlaylist = gEnv->pSystem->LoadXmlFromFile( sPlaylist );

        m_iScene = 0;

        if ( m_xmlPlaylist != NULL )
        {
            m_iSceneCount = m_xmlPlaylist->getChildCount();

            if ( nStartAtScene > 0 )
            {
                m_iScene = CLAMP( nStartAtScene, 0, m_iSceneCount );
            }

            if ( nEndAtScene > 0 )
            {
                m_iSceneCount = CLAMP( nEndAtScene, m_iScene, m_iSceneCount );
            }

            if ( m_iSceneCount > 0 )
            {
                m_bStart = true;
            }

            else
            {
                return false;
            }
        }

        return readNextScene();
    }

    void CVideoplayerPlaylist::Close()
    {
        m_iScene = 0;
        m_iSceneCount = 0;
        m_bLoop = false;
        m_bSkippable = true;
        m_bBlockGame = false;
        m_sFile = "";
        m_bPaused = true;
        m_bStart = false;
        m_bSceneStart = false;
        m_qVideoEvents.empty();
        m_CurrentScene.reset();

        if ( m_xmlPlaylist )
        {
            m_xmlPlaylist = NULL;
            OnEndPlaylist();
        }
    }

    void CVideoplayerPlaylist::Resume()
    {
        m_CurrentScene.Resume();
        m_bPaused = false;
    }

    void CVideoplayerPlaylist::Pause()
    {
        m_CurrentScene.Pause();
        m_bPaused = true;
    }

    void CVideoplayerPlaylist::Advance( float deltaTime )
    {
        bool bVideoEnd = false;

        while ( !m_qVideoEvents.empty() )
        {
            const SVideoEvent& item = m_qVideoEvents.front();

            switch ( item.type )
            {
                case SVideoEvent::OnStart:
                    OnVideoStart( item.pVideo );
                    break;

                case SVideoEvent::OnEnd:
                    OnVideoEnd( item.pVideo );
                    bVideoEnd = true;
                    break;
            }

            m_qVideoEvents.pop();
        }

        if ( bVideoEnd && !IsPlaying() )
        {
            OnEndScene( m_iScene );
        }
    }

    void CVideoplayerPlaylist::Skip( bool bForce )
    {
        if ( bForce || m_bSkippable )
        {
            m_CurrentScene.Skip( bForce );
        }
    }

    bool CVideoplayerPlaylist::ReOpen()
    {
        return true; // TODO
    }

    bool CVideoplayerPlaylist::IsPlaying()
    {
        return m_CurrentScene.IsPlaying();
    }

    bool CVideoplayerPlaylist::IsActive()
    {
        return m_CurrentScene.IsActive();
    }

    bool CVideoplayerPlaylist::Seek( const int scene, float fPos )
    {
        if ( m_iScene != scene )
        {
            m_iScene = scene - 1;
            readNextScene();
        }

        return Seek( fPos );
    }

    bool CVideoplayerPlaylist::Seek( float fPos )
    {
        return m_CurrentScene.Seek( fPos );
    }

    int CVideoplayerPlaylist::GetSceneCount()
    {
        return m_iSceneCount;
    }

    IVideoplayer* CVideoplayerPlaylist::GetSceneVideoplayer( int nIndex )
    {
        if ( nIndex >= 0 && nIndex < GetSceneVideoplayerCount() )
        {
            return m_CurrentScene.vInputs[nIndex].input.pVideo;
        }

        return NULL;
    }

    int CVideoplayerPlaylist::GetSceneVideoplayerCount()
    {
        return m_CurrentScene.vInputs.size();
    }

    void CVideoplayerPlaylist::RegisterListener( IVideoplayerPlaylistEventListener* item )
    {
        vecQueue.push_back( item );
    }

    void CVideoplayerPlaylist::UnregisterListener( IVideoplayerPlaylistEventListener* item )
    {
        for ( std::vector<IVideoplayerPlaylistEventListener*>::const_iterator iterQueue = vecQueue.begin(); iterQueue != vecQueue.end(); iterQueue++ )
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

}