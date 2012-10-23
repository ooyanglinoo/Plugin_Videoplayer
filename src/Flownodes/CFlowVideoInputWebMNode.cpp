/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Nodes/G2FlowBaseNode.h>
#include <CPluginVideoplayer.h>
#include <IVideoplayerSystem.h>
#include <CVideoplayerSystem.h>

namespace VideoplayerPlugin
{
    class CFlowVideoInputWebMNode :
        public CFlowBaseNode<eNCT_Instanced>,
        private IVideoplayerEventListener
    {
        private:
            IVideoplayer* m_pVideo;
            bool m_bStart;
            bool m_bEnd;
            bool m_bFrame;
            bool m_bWasPlaying;

            virtual void OnStart()
            {
                m_bStart = true;
            }

            virtual void OnSeek() { }

            virtual void OnFrame()
            {
                m_bFrame = true;
            }

            virtual void OnEnd()
            {
                m_bEnd = true;
            }

            enum EInputPorts
            {
                EIP_OPEN = 0,
                EIP_CLOSE,
                EIP_FILE,
                EIP_SOUND,
                EIP_LOOP,
                EIP_SKIPPABLE,
                EIP_BLOCKGAME,
                EIP_STARTAT,
                EIP_ENDAFTER,
                EIP_CUSTOMWIDTH,
                EIP_CUSTOMHEIGHT,
                EIP_TIMESOURCE,
                EIP_DROPMODE,
                EIP_SPEED,
                EIP_RESUME,
                EIP_PAUSE,
                EIP_SEEK,
                EIP_POSTION,
            };

            enum EOutputPorts
            {
                EOP_VIDEOID = 0,
                EOP_PLAYING,
                EOP_POSITION,
                EOP_ONSTART,
                EOP_ONEND,
                EOP_DURATION,
                EOP_FPS,
                EOP_WIDTH,
                EOP_HEIGHT
            };

#define INITIALIZE_OUTPUTS(x) \
    ActivateOutput<int>(x, EOP_VIDEOID, -1);\
    ActivateOutput<float>(x, EOP_POSITION, 0.0);\
    ActivateOutput<float>(x, EOP_DURATION, 0.0);\
    ActivateOutput<float>(x, EOP_FPS, 0.0);\
    ActivateOutput<int>(x, EOP_WIDTH, 0);\
    ActivateOutput<int>(x, EOP_HEIGHT, 0);\
    ActivateOutput<bool>(x, EOP_PLAYING, false);

        public:

            CFlowVideoInputWebMNode( SActivationInfo* pActInfo )
            {
                m_pVideo    = NULL;
                m_bStart    = false;
                m_bEnd      = false;
                m_bFrame    = false;
                m_bWasPlaying = false;
            }

            virtual ~CFlowVideoInputWebMNode()
            {
                if ( m_pVideo )
                {
                    m_pVideo->UnregisterListener( this );
                    gVideoplayerSystem->DeleteVideoplayer( m_pVideo );
                }
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowVideoInputWebMNode( pActInfo );
            }

            virtual void GetMemoryUsage( ICrySizer* s ) const
            {
                s->Add( *this );
            }

            void Serialize( SActivationInfo* pActInfo, TSerialize ser )
            {
                /*
                if (ser.IsReading())
                {
                    int counter = m_iCursorCounter;

                    while(counter > 0)
                    {
                        gEnv->pHardwareMouse->DecrementCounter();
                        --counter;
                    }
                }

                ser.Value("m_bEnabled", m_bEnabled);
                ser.Value("m_iCursorCounter", m_iCursorCounter);
                ser.Value("m_bKeyPressed", m_bKeyPressed);
                ser.Value("m_bOnEnter", m_bOnEnter);
                ser.Value("m_bOnLeave", m_bOnLeave);
                ser.Value("m_EntityID", m_EntityID);
                ser.Value("m_LastEntityID", m_LastEntityID);
                ser.Value("m_sKey", m_sKey);
                ser.Value("m_sRaySelection", m_sRaySelection);

                if (ser.IsReading())
                {
                    m_actInfo = *pActInfo;

                    if(gEnv->pHardwareMouse)
                        gEnv->pHardwareMouse->AddListener(this);

                    if(GetISystem() && GetISystem()->GetIInput())
                        GetISystem()->GetIInput()->AddEventListener(this);

                    if(GetPortBool(&m_actInfo, EIP_EnableModalMode))
                    {
                        m_bKeyPressed = true;
                        m_bNoInput = true;

                        if(GetPortBool(&m_actInfo, EIP_DisableMovement))
                        {
                            Movement(false);
                        }

                        MouseCursor(true);
                    }
                    else
                    {
                        Movement(true);
                    }

                    if (m_bEnabled && m_bKeyPressed)
                    {
                        int counter = m_iCursorCounter;

                        while (counter > 0)
                        {
                            gEnv->pHardwareMouse->IncrementCounter();
                            --counter;
                        }

                        Movement(false);
                    }
                } */
            }

            virtual void GetConfiguration( SFlowNodeConfig& config )
            {
                static const SInputPortConfig inputs[] =
                {
                    InputPortConfig_Void( "Open",                                _HELP( "Open / Start paused (call Resume)" ) ),
                    InputPortConfig_Void( "Close",                               _HELP( "Close / Stop" ) ),

                    InputPortConfig<string>( "file_File",    "",                 _HELP( "videofile (set on Open)" ),                   "sFile",                        _UICONFIG( "" ) ),
                    InputPortConfig<string>( "sound_Sound",  "",                 _HELP( "soundfile/event (set on Open)" ),             "sSound",                       _UICONFIG( "" ) ),
                    InputPortConfig<bool>( "Loop",           true,               _HELP( "loops the video (set on Open)" ),             "bLoop" ),
                    InputPortConfig<bool>( "Skippable",      false,              _HELP( "Can this video be skipped by the user" ),     "bSkippable" ),
                    InputPortConfig<bool>( "BlockGame",      false,              _HELP( "Block Game while this video is playing" ),    "bBlockGame" ),

                    InputPortConfig<float>( "StartAt",       0.0,                _HELP( "start [sec]" ),                               "fStartAt" ),
                    InputPortConfig<float>( "EndAfter",      0.0,                _HELP( "end/loop [sec]" ),                            "fEndAfter" ),
                    InputPortConfig<int>( "CustomWidth",     -1,                 _HELP( "custom render width [px]" ),                  "nCustomWidth" ),
                    InputPortConfig<int>( "CustomHeight",    -1,                 _HELP( "custom render height [px]" ),                 "nCustomHeight" ),
                    InputPortConfig<int>( "TimeSource",      int( VTS_Default ),   _HELP( "timesource to sync to" ),                     "nTimeSource",                  _UICONFIG( "enum_int:Game=1,Sound=2,System=4,SoundOrGame=3,SoundOrSystem=6" ) ),
                    InputPortConfig<int>( "DropMode",        int( VDM_Default ),   _HELP( "dropmode to use for sync" ),                  "nDropMode",                    _UICONFIG( "enum_int:None=0,Drop=1,Seek=2,DropOutput=4,DropOrSeek=3,DropOutputOrSeek=6" ) ),
                    InputPortConfig<float>( "Speed",         1.0,                _HELP( "play speed" ),                                "fSpeed" ),

                    InputPortConfig_Void( "Resume",                              _HELP( "Resume" ) ),
                    InputPortConfig_Void( "Pause",                               _HELP( "Pause" ) ),

                    InputPortConfig_Void( "Seek",                                _HELP( "seeks to position" ) ),
                    InputPortConfig<float>( "PositionI",     0.0,                _HELP( "position for seek [sec]" ),                   "fPosition" ),
                    {0},
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig<int>( "VideoID",                            _HELP( "id for further use" ),                        "nVideoID" ),
                    OutputPortConfig<bool>( "Playing",                           _HELP( "currently playing" ),                         "bPlaying" ),
                    OutputPortConfig<float>( "PositionO",                        _HELP( "position [sec]" ),                            "fPosition" ),
                    OutputPortConfig_Void( "OnStart",                            _HELP( "start/loop begin reached" ) ),
                    OutputPortConfig_Void( "OnEnd",                              _HELP( "End reached" ) ),
                    OutputPortConfig<float>( "Duration",                         _HELP( "duration [sec]" ),                            "fDuration" ),
                    OutputPortConfig<float>( "FPS",                              _HELP( "frames per second" ),                         "fFPS" ),
                    OutputPortConfig<int>( "Width",                              _HELP( "decoder width [px]" ),                        "nWidth" ),
                    OutputPortConfig<int>( "Height",                             _HELP( "decoder height [px]" ),                       "nHeight" ),
                    {0},
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX "Videosource WebM" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Suspend:
                        if ( m_pVideo && m_pVideo->IsPlaying() )
                        {
                            m_pVideo->Pause();
                            m_bWasPlaying = true;
                        }

                        pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
                        break;

                    case eFE_Resume:
                        if ( m_pVideo && m_bWasPlaying )
                        {
                            m_pVideo->Resume();
                            m_bWasPlaying = false;
                        }

                        pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
                        break;

                    case eFE_Initialize:
                        INITIALIZE_OUTPUTS( pActInfo );
                        break;

                    case eFE_Activate:
                        if ( !m_pVideo )
                        {
                            m_pVideo = gVideoplayerSystem->CreateVideoplayer();

                            if ( m_pVideo )
                            {
                                m_pVideo->RegisterListener( this );
                                pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
                            }
                        }

                        if ( !m_pVideo )
                        {
                            return;
                        }

                        if ( IsPortActive( pActInfo, EIP_CLOSE ) )
                        {
                            m_pVideo->Close();
                            INITIALIZE_OUTPUTS( pActInfo );
                        }

                        if ( IsPortActive( pActInfo, EIP_PAUSE ) )
                        {
                            m_pVideo->Pause();
                            ActivateOutput<float>( pActInfo, EOP_POSITION, m_pVideo->GetPosition() );
                            ActivateOutput<bool>( pActInfo, EOP_PLAYING, m_pVideo->IsPlaying() );
                        }

                        if ( IsPortActive( pActInfo, EIP_OPEN ) )
                        {
                            if ( m_pVideo->Open(
                                        GetPortString( pActInfo, EIP_FILE ).c_str(),
                                        GetPortString( pActInfo, EIP_SOUND ).c_str(),
                                        GetPortBool( pActInfo, EIP_LOOP ),
                                        GetPortBool( pActInfo, EIP_SKIPPABLE ),
                                        GetPortBool( pActInfo, EIP_BLOCKGAME ),
                                        eTimeSource( GetPortInt( pActInfo, EIP_TIMESOURCE ) ),
                                        eDropMode( GetPortInt( pActInfo, EIP_DROPMODE ) ),
                                        GetPortFloat( pActInfo, EIP_STARTAT ),
                                        GetPortFloat( pActInfo, EIP_ENDAFTER ),
                                        GetPortInt( pActInfo, EIP_CUSTOMWIDTH ),
                                        GetPortInt( pActInfo, EIP_CUSTOMHEIGHT ) ) )
                            {
                                ActivateOutput<int>( pActInfo, EOP_VIDEOID, m_pVideo->GetId() );
                                ActivateOutput<float>( pActInfo, EOP_POSITION, m_pVideo->GetPosition() );
                                ActivateOutput<float>( pActInfo, EOP_DURATION, m_pVideo->GetDuration() );
                                ActivateOutput<float>( pActInfo, EOP_FPS, m_pVideo->GetFPS() );
                                ActivateOutput<int>( pActInfo, EOP_WIDTH, m_pVideo->GetWidth() );
                                ActivateOutput<int>( pActInfo, EOP_HEIGHT, m_pVideo->GetHeight() );
                                ActivateOutput<bool>( pActInfo, EOP_PLAYING, m_pVideo->IsPlaying() );
                                m_pVideo->SetSpeed( GetPortFloat( pActInfo, EIP_SPEED ) );
                            }

                            else
                            {
                                INITIALIZE_OUTPUTS( pActInfo );
                            }
                        }

                        if ( IsPortActive( pActInfo, EIP_RESUME ) )
                        {
                            m_pVideo->SetSpeed( GetPortFloat( pActInfo, EIP_SPEED ) );
                            m_pVideo->Resume();
                            ActivateOutput<float>( pActInfo, EOP_POSITION, m_pVideo->GetPosition() );
                            ActivateOutput<bool>( pActInfo, EOP_PLAYING, m_pVideo->IsPlaying() );
                            m_bStart = m_pVideo->IsPlaying();
                        }

                        if ( IsPortActive( pActInfo, EIP_SEEK ) )
                        {
                            m_pVideo->Seek( GetPortFloat( pActInfo, EIP_POSTION ) );
                            ActivateOutput<float>( pActInfo, EOP_POSITION, m_pVideo->GetPosition() );
                        }

                        if ( IsPortActive( pActInfo, EIP_SPEED ) )
                        {
                            m_pVideo->SetSpeed( GetPortFloat( pActInfo, EIP_SPEED ) );
                        }

                        if ( IsPortActive( pActInfo, EIP_TIMESOURCE ) )
                        {
                            m_pVideo->SetTimesource( eTimeSource( GetPortInt( pActInfo, EIP_TIMESOURCE ) ) );
                        }

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

                        if ( m_bFrame )
                        {
                            m_bFrame = false;
                            ActivateOutput<float>( pActInfo, EOP_POSITION, m_pVideo->GetPosition() );
                        }

                        break;
                }
            }
    };
}

REGISTER_FLOW_NODE_EX( "Videoplayer_Plugin:InputWebM", VideoplayerPlugin::CFlowVideoInputWebMNode, CFlowVideoInputWebMNode );