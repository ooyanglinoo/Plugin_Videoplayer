/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Nodes/G2FlowBaseNode.h>
#include <CPluginVideoplayer.h>
#include <IPluginVideoplayer.h>
#include <CVideoplayerSystem.h>

namespace VideoplayerPlugin
{
    class CFlowVideoOutput2DNode : public CFlowBaseNode<eNCT_Instanced>
    {
        private:
            S2DVideo* m_p2DVideo;

            enum EInputPorts
            {
                EIP_SHOW = 0,
                EIP_HIDE,
                EIP_VIDEOID,
                EIP_SOUNDSOURCE,
                EIP_RESIZEMODE,
                EIP_CUSTOMAR,
                EIP_REL_TOP,
                EIP_REL_LEFT,
                EIP_REL_WIDTH,
                EIP_REL_HEIGHT,
                EIP_ANGLE,
                EIP_RGB,
                EIP_ALPHA,
                EIP_BG_RGB,
                EIP_BG_ALPHA,
                EIP_ZORDER,
            };

        public:
            CFlowVideoOutput2DNode( SActivationInfo* pActInfo )
            {
                m_p2DVideo = NULL;
            }

            virtual ~CFlowVideoOutput2DNode()
            {
                if ( m_p2DVideo )
                {
                    gVideoplayerSystem->Delete2DVideo( m_p2DVideo );
                }
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowVideoOutput2DNode( pActInfo );
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
                    InputPortConfig_Void( "Show",                              _HELP( "Activate fullscreen" ) ),
                    InputPortConfig_Void( "Hide",                              _HELP( "Hide fullscreen" ) ),
                    InputPortConfig<int>( "VideoID",         -1,               _HELP( "id" ), "nVideoID" ),
                    InputPortConfig<bool>( "SoundSource",    false,            _HELP( "Output 2D Sound for this video" ), "bSoundSource" ),
                    InputPortConfig<int>( "ResizeMode",      int( VRM_Default ), _HELP( "How should the video be resized to fit the screen" ), "nResizeMode", _UICONFIG( "enum_int:Original=0,Stretch=1,TouchInside=2,TouchOutside=3" ) ),
                    InputPortConfig<float>( "CustomAR",      0.0f,             _HELP( "Custom Aspect Ratio (4:3=1.33 / 16:9=1.77)" ), "fCustomAR" ),
                    InputPortConfig<float>( "Top",           0.0f,             _HELP( "Screen relative top" ),  "fTop" ),
                    InputPortConfig<float>( "Left",          0.0f,             _HELP( "Screen relative left" ), "fLeft" ),
                    InputPortConfig<float>( "Width",         1,                _HELP( "Screen relative width" ), "fWidth" ),
                    InputPortConfig<float>( "Height",        1,                _HELP( "Screen relative height" ), "fHeight" ),
                    InputPortConfig<float>( "Angle",         0,                _HELP( "Angle" ), "fAngle" ),
                    InputPortConfig<Vec3>( "color_RGB",      Vec3( 1, 1, 1 ),  _HELP( "RGB " ), "fRGB", _UICONFIG( "" ) ),
                    InputPortConfig<float>( "Alpha",         1,                _HELP( "Alpha" ), "fAlpha" ),
                    InputPortConfig<Vec3>( "color_BGRGB",    Vec3( 0, 0, 0 ),  _HELP( "Background RGB color" ), "fBG_RGB", _UICONFIG( "" ) ),
                    InputPortConfig<float>( "BGAlpha",       0,                _HELP( "Alpha when set displays background to fill background" ), "fBG_Alpha" ),
                    InputPortConfig<int>( "ZOrder",          int( VZP_Default ), _HELP( "When should the video be drawn" ), "nZOrder", _UICONFIG( "enum_int:BehindMenu=0,AboveMenu=1" ) ),
                    {0},
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = NULL;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX "Videodestination/2D & Fullscreen" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Suspend:
                        break;

                    case eFE_Resume:
                        break;

                    case eFE_Initialize:
                        break;

                    case eFE_Activate:
                        if ( IsPortActive( pActInfo, EIP_HIDE ) )
                        {
                            gVideoplayerSystem->Delete2DVideo( m_p2DVideo );
                            m_p2DVideo = NULL;
                        }

                        else if ( IsPortActive( pActInfo, EIP_SHOW ) )
                        {
                            if ( !m_p2DVideo )
                            {
                                m_p2DVideo = gVideoplayerSystem->Create2DVideo();
                            }

                            if ( m_p2DVideo )
                            {
                                // Set all properties
                                m_p2DVideo->SetSoundsource( GetPortBool( pActInfo, EIP_SOUNDSOURCE ) );
                                m_p2DVideo->SetVideo( gVideoplayerSystem->GetVideoplayerById( GetPortInt( pActInfo, EIP_VIDEOID ) ) );
                                m_p2DVideo->nResizeMode = eResizeMode( GetPortInt( pActInfo, EIP_RESIZEMODE ) );
                                m_p2DVideo->fCustomAR = GetPortFloat( pActInfo, EIP_CUSTOMAR );
                                m_p2DVideo->fRelTop = GetPortFloat( pActInfo, EIP_REL_TOP );
                                m_p2DVideo->fRelLeft = GetPortFloat( pActInfo, EIP_REL_LEFT );
                                m_p2DVideo->fRelWidth = GetPortFloat( pActInfo, EIP_REL_WIDTH );
                                m_p2DVideo->fRelHeight = GetPortFloat( pActInfo, EIP_REL_HEIGHT );
                                m_p2DVideo->fAngle = GetPortFloat( pActInfo, EIP_ANGLE );
                                m_p2DVideo->cRGBA = ColorF( GetPortVec3( pActInfo, EIP_RGB ), GetPortFloat( pActInfo, EIP_ALPHA ) );
                                m_p2DVideo->cBG_RGBA = ColorF( GetPortVec3( pActInfo, EIP_BG_RGB ), GetPortFloat( pActInfo, EIP_BG_ALPHA ) );
                                m_p2DVideo->nZPos = eZPos( GetPortInt( pActInfo, EIP_ZORDER ) );
                            }
                        }

                        else if ( m_p2DVideo )
                        {
                            // Set changed properties
                            if ( IsPortActive( pActInfo, EIP_SOUNDSOURCE ) || IsPortActive( pActInfo, EIP_SHOW ) )
                            {
                                m_p2DVideo->SetSoundsource( GetPortBool( pActInfo, EIP_SOUNDSOURCE ) );
                            }

                            if ( IsPortActive( pActInfo, EIP_VIDEOID ) || IsPortActive( pActInfo, EIP_SHOW ) )
                            {
                                m_p2DVideo->SetVideo( gVideoplayerSystem->GetVideoplayerById( GetPortInt( pActInfo, EIP_VIDEOID ) ) );
                            }

                            if ( IsPortActive( pActInfo, EIP_RESIZEMODE ) )
                            {
                                m_p2DVideo->nResizeMode = eResizeMode( GetPortInt( pActInfo, EIP_RESIZEMODE ) );
                            }

                            if ( IsPortActive( pActInfo, EIP_CUSTOMAR ) )
                            {
                                m_p2DVideo->fCustomAR = GetPortFloat( pActInfo, EIP_CUSTOMAR );
                            }

                            if ( IsPortActive( pActInfo, EIP_REL_TOP ) )
                            {
                                m_p2DVideo->fRelTop = GetPortFloat( pActInfo, EIP_REL_TOP );
                            }

                            if ( IsPortActive( pActInfo, EIP_REL_LEFT ) )
                            {
                                m_p2DVideo->fRelLeft = GetPortFloat( pActInfo, EIP_REL_LEFT );
                            }

                            if ( IsPortActive( pActInfo, EIP_REL_WIDTH ) )
                            {
                                m_p2DVideo->fRelWidth = GetPortFloat( pActInfo, EIP_REL_WIDTH );
                            }

                            if ( IsPortActive( pActInfo, EIP_REL_HEIGHT ) )
                            {
                                m_p2DVideo->fRelHeight = GetPortFloat( pActInfo, EIP_REL_HEIGHT );
                            }

                            if ( IsPortActive( pActInfo, EIP_ANGLE ) )
                            {
                                m_p2DVideo->fAngle = GetPortFloat( pActInfo, EIP_ANGLE );
                            }

                            if ( IsPortActive( pActInfo, EIP_RGB ) )
                            {
                                m_p2DVideo->cRGBA = ColorF( GetPortVec3( pActInfo, EIP_RGB ), GetPortFloat( pActInfo, EIP_ALPHA ) );
                            }

                            if ( IsPortActive( pActInfo, EIP_ALPHA ) )
                            {
                                m_p2DVideo->cRGBA.a = GetPortFloat( pActInfo, EIP_ALPHA );
                            }

                            if ( IsPortActive( pActInfo, EIP_BG_RGB ) )
                            {
                                m_p2DVideo->cBG_RGBA = ColorF( GetPortVec3( pActInfo, EIP_BG_RGB ), GetPortFloat( pActInfo, EIP_BG_ALPHA ) );
                            }

                            if ( IsPortActive( pActInfo, EIP_BG_ALPHA ) )
                            {
                                m_p2DVideo->cBG_RGBA.a = GetPortFloat( pActInfo, EIP_BG_ALPHA );
                            }

                            if ( IsPortActive( pActInfo, EIP_BG_ALPHA ) )
                            {
                                m_p2DVideo->nZPos = eZPos( GetPortInt( pActInfo, EIP_ZORDER ) );
                            }
                        }

                        break;

                    case eFE_Update:
                        break;
                }
            }
    };
}

REGISTER_FLOW_NODE_EX( "Videoplayer_Plugin:Output2D", VideoplayerPlugin::CFlowVideoOutput2DNode, CFlowVideoOutput2DNode );