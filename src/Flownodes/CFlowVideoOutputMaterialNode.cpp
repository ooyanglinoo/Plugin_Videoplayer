/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Nodes/G2FlowBaseNode.h>
#include <CPluginVideoplayer.h>
#include <IPluginVideoplayer.h>
#include <CVideoplayerSystem.h>

namespace VideoplayerPlugin
{
    class CFlowVideoOutputMaterialNode : public CFlowBaseNode<eNCT_Instanced>
    {
        private:
            IVideoplayer* m_pVideo;

            enum EInputPorts
            {
                EIP_OVERRIDE = 0,
                EIP_RESET,
                EIP_VIDEOID,
                EIP_MATERIAL,
                EIP_SUBMAT,
                EIP_TEXSLOT,
                EIP_RECOMMENDED,
            };

        public:

            CFlowVideoOutputMaterialNode( SActivationInfo* pActInfo )
            {
                m_pVideo = NULL;
            }

            virtual ~CFlowVideoOutputMaterialNode()
            {

            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowVideoOutputMaterialNode( pActInfo );
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
                    InputPortConfig_Void( "Override",                            _HELP( "override material now" ) ),
                    InputPortConfig_Void( "Reset",                               _HELP( "reset to previous material" ) ),
                    InputPortConfig<int>( "VideoID",                 -1,         _HELP( "id" ),                                            "nVideoID" ),
                    InputPortConfig<string>( "mat_Material",         "",         _HELP( "material to be modified" ),                       "sMaterial",    _UICONFIG( "" ) ),
                    InputPortConfig<int>( "SubMaterial",             0,          _HELP( "submaterial to be modified" ),                    "nSubMat",      _UICONFIG( "" ) ),
                    InputPortConfig<int>( "TextureSlot",             0,          _HELP( "textureslot to be modified" ),                    "nTexSlot",     _UICONFIG( "enum_int:00_DIFFUSE=0,01_BUMP=1,02_GLOSS=2,03_ENV=3,04_DETAIL_OVERLAY=4,05_BUMP_DIFFUSE=5,06_BUMP_HEIGHT=6,07_DECAL_OVERLAY=7,08_SUBSURFACE=8,09_CUSTOM=9,10_CUSTOM_SECONDARY=10,11_OPACITY=11" ) ),
                    InputPortConfig<bool>( "RecommendedSettings",    true,       _HELP( "modify shader and lightning for optimal colors" ), "bRecommendedSettings" ),
                    {0},
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = NULL;//outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX "Videodestination/material" );

                //config.nFlags |= EFLN_TARGET_ENTITY;
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
                        m_pVideo = NULL;
                        break;

                    case eFE_Activate:
                        if ( IsPortActive( pActInfo, EIP_OVERRIDE ) )
                        {
                            m_pVideo = gVideoplayerSystem->GetVideoplayerById( GetPortInt( pActInfo, EIP_VIDEOID ) );

                            if ( m_pVideo )
                            {
                                gVideoplayerSystem->OverrideMaterial( m_pVideo, gEnv->p3DEngine->GetMaterialManager()->FindMaterial( GetPortString( pActInfo, EIP_MATERIAL ) ), GetPortInt( pActInfo, EIP_SUBMAT ), GetPortInt( pActInfo, EIP_TEXSLOT ), GetPortBool( pActInfo, EIP_RECOMMENDED ) );
                            }
                        }

                        if ( IsPortActive( pActInfo, EIP_RESET ) )
                        {
                            gVideoplayerSystem->ResetMaterial( gEnv->p3DEngine->GetMaterialManager()->FindMaterial( GetPortString( pActInfo, EIP_MATERIAL ) ), GetPortInt( pActInfo, EIP_SUBMAT ), true );
                        }

                        break;

                    case eFE_Update:
                        break;
                }
            }
    };
}

REGISTER_FLOW_NODE_EX( "Videoplayer_Plugin:OutputMaterial", VideoplayerPlugin::CFlowVideoOutputMaterialNode, CFlowVideoOutputMaterialNode );