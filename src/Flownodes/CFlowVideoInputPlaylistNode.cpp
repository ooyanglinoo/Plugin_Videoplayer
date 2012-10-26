/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Nodes/G2FlowBaseNode.h>
#include <CPluginVideoplayer.h>
#include <IPluginVideoplayer.h>
#include <CVideoplayerSystem.h>
#include <Playlist/CVideoplayerPlaylist.h>

namespace VideoplayerPlugin
{
    class CFlowVideoInputPlaylistNode :
        public CFlowBaseNode<eNCT_Instanced>,
        private IVideoplayerPlaylistEventListener
    {

            typedef struct SVideoInformation_
            {
                bool m_bVideoStart;
                bool m_bVideoEnd;
                IVideoplayer* m_pVideo;

                SVideoInformation_()
                {
                    m_bVideoStart = false;
                    m_bVideoEnd = false;
                    m_pVideo = NULL;
                }
            } SVideoInformation;

        private:
            IVideoplayerPlaylist* m_pPlaylist;
            bool m_bStart;
            bool m_bEnd;

            bool m_bSceneStart;
            bool m_bSceneEnd;

            SVideoInformation m_VideoInfo[3];

            bool m_bWasPlaying;

            virtual void OnBeginScene( IVideoplayerPlaylist* pPlaylist, int nIndex )
            {
                m_bSceneStart = true;

                for ( int i = 0; i < 3; ++i )
                {
                    m_VideoInfo[i].m_pVideo = pPlaylist->GetSceneVideoplayer( i );
                }
            };

            virtual void OnVideoStart( IVideoplayerPlaylist* pPlaylist, IVideoplayer* pVideo )
            {
                for ( int i = 0; i < 3; ++i )
                {
                    if ( m_VideoInfo[i].m_pVideo == pVideo )
                    {
                        m_VideoInfo[i].m_bVideoStart = true;
                    }
                }
            };

            virtual void OnVideoEnd( IVideoplayerPlaylist* pPlaylist, IVideoplayer* pVideo )
            {
                for ( int i = 0; i < 3; ++i )
                {
                    if ( m_VideoInfo[i].m_pVideo == pVideo )
                    {
                        m_VideoInfo[i].m_bVideoEnd = true;
                    }
                }
            };

            virtual void OnEndScene( IVideoplayerPlaylist* pPlaylist, int nIndex )
            {
                m_bSceneEnd = true;

                for ( int i = 0; i < 3; ++i )
                {
                    m_VideoInfo[i].m_pVideo = NULL;
                }
            };

            virtual void OnStartPlaylist( IVideoplayerPlaylist* pPlaylist )
            {
                m_bStart = true;
            }

            virtual void OnEndPlaylist( IVideoplayerPlaylist* pPlaylist )
            {
                m_bEnd = true;
            }

            enum EInputPorts
            {
                EIP_OPEN = 0,
                EIP_CLOSE,
                EIP_FILE,
                EIP_LOOP,
                EIP_SKIPPABLE,
                EIP_BLOCKGAME,
                EIP_STARTAT,
                EIP_ENDAFTER,
                EIP_RESUME,
                EIP_PAUSE,
            };

            enum EOutputPorts
            {
                EOP_VIDEOID = 0,
                EOP_VIDEOID2,
                EOP_VIDEOID3,
                EOP_PLAYING,
                EOP_ONSTART,
                EOP_ONEND,
                EOP_ONSCENESTART,
                EOP_ONSCENEEND,
                EOP_ONVIDEOSTART,
                EOP_ONVIDEOEND,
                EOP_ONVIDEOSTART2,
                EOP_ONVIDEOEND2,
                EOP_ONVIDEOSTART3,
                EOP_ONVIDEOEND3,
            };

#define INITIALIZE_OUTPUTS(x) \
    ActivateOutput<int>(x, EOP_VIDEOID, -1);\
    ActivateOutput<int>(x, EOP_VIDEOID2, -1);\
    ActivateOutput<int>(x, EOP_VIDEOID3, -1);\
    ActivateOutput<bool>(x, EOP_PLAYING, false);

        public:

            CFlowVideoInputPlaylistNode( SActivationInfo* pActInfo )
            {
                m_pPlaylist = NULL;
                m_bStart    = false;
                m_bEnd      = false;

                m_bSceneStart = false;
                m_bSceneEnd = false;

                m_bWasPlaying = false;
            }

            virtual ~CFlowVideoInputPlaylistNode()
            {
                if ( m_pPlaylist )
                {
                    m_pPlaylist->UnregisterListener( this );
                    gVideoplayerSystem->DeletePlaylist( m_pPlaylist );
                }
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowVideoInputPlaylistNode( pActInfo );
            }

            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            void Serialize( SActivationInfo* pActInfo, TSerialize ser )
            {
            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig_Void( "Open",                                _HELP( "Open / Start paused (call Resume)" ) ),
                    InputPortConfig_Void( "Close",                               _HELP( "Close / Stop" ) ),

                    InputPortConfig<string>( "file_File",    "",                 _HELP( "playlist xml file" ),                 "sFile",                        _UICONFIG( "" ) ),
                    InputPortConfig<bool>( "Loop",           true,               _HELP( "loops the playlist (set on Open)" ),              "bLoop" ),
                    InputPortConfig<bool>( "Skippable",  true,       _HELP( "Can this playlist be skipped by the user" ),      "bSkippable" ),
                    InputPortConfig<bool>( "BlockGame",  false,      _HELP( "Block Game while this playlist is playing" ),     "bBlockGame" ),

                    InputPortConfig<int>( "StartAt",         0.0,                _HELP( "start [scene]" ),                             "nStartAt" ),
                    InputPortConfig<int>( "EndAfter",        0.0,                _HELP( "end/loop [scene]" ),                          "nEndAfter" ),

                    InputPortConfig_Void( "Resume",                              _HELP( "Resume" ) ),
                    InputPortConfig_Void( "Pause",                               _HELP( "Pause" ) ),
                    {0},
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<int>( "VideoID",                            _HELP( "id for further use" ),                        "nVideoID" ),
                    OutputPortConfig<int>( "VideoID2",                           _HELP( "id2 for further use" ),                       "nVideoID2" ),
                    OutputPortConfig<int>( "VideoID3",                           _HELP( "id3 for further use" ),                       "nVideoID3" ),
                    OutputPortConfig<bool>( "Playing",                           _HELP( "currently playing" ),                         "bPlaying" ),

                    OutputPortConfig_Void( "OnStart",                            _HELP( "Start reached" ) ),
                    OutputPortConfig_Void( "OnEnd",                              _HELP( "End reached" ) ),
                    OutputPortConfig_Void( "OnSceneStart",                       _HELP( "Scene start reached" ) ),
                    OutputPortConfig_Void( "OnSceneEnd",                         _HELP( "Scene end reached" ) ),
                    OutputPortConfig_Void( "OnVideoStart",                       _HELP( "Video1 start reached" ) ),
                    OutputPortConfig_Void( "OnVideoEnd",                         _HELP( "Video1 end reached" ) ),
                    OutputPortConfig_Void( "OnVideoStart2",                      _HELP( "Video2 start reached" ) ),
                    OutputPortConfig_Void( "OnVideoEnd2",                        _HELP( "Video2 end reached" ) ),
                    OutputPortConfig_Void( "OnVideoStart3",                      _HELP( "Video3 start reached" ) ),
                    OutputPortConfig_Void( "OnVideoEnd3",                        _HELP( "Video3 end reached" ) ),
                    {0},
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX "Videosource Playlist" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Suspend:
                        if ( m_pPlaylist && m_pPlaylist->IsPlaying() )
                        {
                            m_pPlaylist->Pause();
                            m_bWasPlaying = true;
                        }

                        pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
                        break;

                    case eFE_Resume:
                        if ( m_pPlaylist && m_bWasPlaying )
                        {
                            m_pPlaylist->Resume();
                            m_bWasPlaying = false;
                        }

                        pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
                        break;

                    case eFE_Initialize:
                        INITIALIZE_OUTPUTS( pActInfo );
                        break;

                    case eFE_Activate:
                        if ( !m_pPlaylist )
                        {
                            m_pPlaylist = gVideoplayerSystem->CreatePlaylist();

                            if ( m_pPlaylist )
                            {
                                m_pPlaylist->RegisterListener( this );
                                pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
                            }
                        }

                        if ( !m_pPlaylist )
                        {
                            return;
                        }

                        if ( IsPortActive( pActInfo, EIP_CLOSE ) )
                        {
                            m_pPlaylist->Close();
                            INITIALIZE_OUTPUTS( pActInfo );
                        }

                        if ( IsPortActive( pActInfo, EIP_PAUSE ) )
                        {
                            m_pPlaylist->Pause();
                            ActivateOutput<bool>( pActInfo, EOP_PLAYING, m_pPlaylist->IsPlaying() );
                        }

                        if ( IsPortActive( pActInfo, EIP_OPEN ) )
                        {
                            if ( m_pPlaylist->Open(
                                        GetPortString( pActInfo, EIP_FILE ).c_str(),
                                        GetPortBool( pActInfo, EIP_LOOP ),
                                        GetPortBool( pActInfo, EIP_SKIPPABLE ),
                                        GetPortBool( pActInfo, EIP_BLOCKGAME ),
                                        GetPortInt( pActInfo, EIP_STARTAT ),
                                        GetPortInt( pActInfo, EIP_ENDAFTER ) ) )
                            {
                                ActivateOutput<bool>( pActInfo, EOP_PLAYING, m_pPlaylist->IsPlaying() );
                            }

                            else
                            {
                                INITIALIZE_OUTPUTS( pActInfo );
                            }
                        }

                        if ( IsPortActive( pActInfo, EIP_RESUME ) )
                        {
                            m_pPlaylist->Resume();
                            ActivateOutput<bool>( pActInfo, EOP_PLAYING, m_pPlaylist->IsPlaying() );
                        }

                        //if(IsPortActive(pActInfo, EIP_SEEK))
                        //{
                        //  m_pVideo->Seek(GetPortFloat(pActInfo, EIP_POSTION));
                        //  ActivateOutput<float>(pActInfo, EOP_POSITION, m_pVideo->GetPosition());
                        //}

                        break;

                    case eFE_Update:
                        if ( m_bEnd )
                        {
                            m_bEnd = false;
                            ActivateOutput( pActInfo, EOP_ONEND, true );
                        }

                        else if ( m_bStart )
                        {
                            m_bStart = false;
                            ActivateOutput( pActInfo, EOP_ONSTART, true );
                        }

                        if ( m_bSceneEnd )
                        {
                            m_bSceneEnd = false;
                            ActivateOutput( pActInfo, EOP_ONSCENEEND, true );
                        }

                        else if ( m_bSceneStart )
                        {
                            m_bSceneStart = false;
                            ActivateOutput( pActInfo, EOP_ONSCENESTART, true );
                        }

                        int nVideo = 0;

                        if ( m_VideoInfo[nVideo].m_bVideoEnd )
                        {
                            m_VideoInfo[nVideo].m_bVideoEnd = false;
                            ActivateOutput( pActInfo, EOP_ONVIDEOEND, true );
                        }

                        else if ( m_VideoInfo[nVideo].m_bVideoStart )
                        {
                            m_VideoInfo[nVideo].m_bVideoStart = false;

                            if ( m_VideoInfo[nVideo].m_pVideo )
                            {
                                ActivateOutput<int>( pActInfo, EOP_VIDEOID, m_VideoInfo[nVideo].m_pVideo->GetId() );
                            }

                            ActivateOutput( pActInfo, EOP_ONVIDEOSTART, true );
                        }

                        nVideo = 1;

                        if ( m_VideoInfo[nVideo].m_bVideoEnd )
                        {
                            m_VideoInfo[nVideo].m_bVideoEnd = false;
                            ActivateOutput( pActInfo, EOP_ONVIDEOEND2, true );
                        }

                        else if ( m_VideoInfo[nVideo].m_bVideoStart )
                        {
                            m_VideoInfo[nVideo].m_bVideoStart = false;

                            if ( m_VideoInfo[nVideo].m_pVideo )
                            {
                                ActivateOutput<int>( pActInfo, EOP_VIDEOID2, m_VideoInfo[nVideo].m_pVideo->GetId() );
                            }

                            ActivateOutput( pActInfo, EOP_ONVIDEOSTART2, true );
                        }

                        nVideo = 2;

                        if ( m_VideoInfo[nVideo].m_bVideoEnd )
                        {
                            m_VideoInfo[nVideo].m_bVideoEnd = false;
                            ActivateOutput( pActInfo, EOP_ONVIDEOEND3, true );
                        }

                        else if ( m_VideoInfo[nVideo].m_bVideoStart )
                        {
                            m_VideoInfo[nVideo].m_bVideoStart = false;

                            if ( m_VideoInfo[nVideo].m_pVideo )
                            {
                                ActivateOutput<int>( pActInfo, EOP_VIDEOID3, m_VideoInfo[nVideo].m_pVideo->GetId() );
                            }

                            ActivateOutput( pActInfo, EOP_ONVIDEOSTART3, true );
                        }

                        break;
                }
            }
    };
}

REGISTER_FLOW_NODE_EX( "Videoplayer_Plugin:InputPlaylist", VideoplayerPlugin::CFlowVideoInputPlaylistNode, CFlowVideoInputPlaylistNode );