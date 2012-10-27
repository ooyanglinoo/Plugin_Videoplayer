/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Nodes/G2FlowBaseNode.h>
#include <CPluginVideoplayer.h>
#include <IPluginVideoplayer.h>
#include <CVideoplayerSystem.h>

namespace VideoplayerPlugin
{
    class CFlowVideoOutputTextureNode : public CFlowBaseNode<eNCT_Instanced>
    {
        private:
            IVideoplayer* m_pVideo;
            int m_nID;
            string m_sName;

            enum EInputPorts
            {
                EIP_GET = 0,
                EIP_VIDEOID,
            };

            enum EOutputPorts
            {
                EOP_CHANGED = 0,
                EOP_TEXID,
                EOP_TEXNAME,
            };

        public:
            CFlowVideoOutputTextureNode( SActivationInfo* pActInfo )
            {
                m_pVideo = NULL;
                m_nID = -1;
                m_sName = "";
            }

            virtual ~CFlowVideoOutputTextureNode()
            {
                m_pVideo = NULL;
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowVideoOutputTextureNode( pActInfo );
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
                    InputPortConfig_Void( "Get", _HELP( "Get texture info" ) ),
                    InputPortConfig<int>( "VideoID", -1, _HELP( "id" ), "nVideoID" ),
                    {0},
                };

                static const SOutputPortConfig outputs[] =
                {
                    OutputPortConfig_Void( "OnChanged", _HELP( "texture name/id changed" ) ),
                    OutputPortConfig<int>( "TextureID", _HELP( "texture id" ), "nTextureID" ),
                    OutputPortConfig<string>( "TextureName", _HELP( "texture name" ), "sTextureName" ),
                    {0},
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX "Videodestination/Texture for custom use" );

                config.SetCategory( EFLN_APPROVED );
            }

            virtual void ProcessEvent( EFlowEvent evt, SActivationInfo* pActInfo )
            {
                switch ( evt )
                {
                    case eFE_Suspend:
                        pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
                        break;

                    case eFE_Resume:
                        pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
                        break;

                    case eFE_Initialize:
                        break;

                    case eFE_Activate:

                        if ( IsPortActive( pActInfo, EIP_GET ) && !m_pVideo )
                        {
                            m_pVideo = gVideoplayerSystem->GetVideoplayerById( GetPortInt( pActInfo, EIP_VIDEOID ) );
                            pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, true );
                        }

                        else if ( m_pVideo )
                        {
                            // Set changed properties
                            if ( IsPortActive( pActInfo, EIP_VIDEOID ) )
                            {
                                m_pVideo = gVideoplayerSystem->GetVideoplayerById( GetPortInt( pActInfo, EIP_VIDEOID ) );
                            }
                        }

                        break;

                    case eFE_Update:
                        if ( m_pVideo )
                        {
                            ITexture* tex = m_pVideo->GetTexture();

                            if ( tex && m_nID != tex->GetTextureID() )
                            {
                                m_nID = tex->GetTextureID();
                                m_sName = tex->GetName();

                                ActivateOutput( pActInfo, EOP_TEXID, m_nID );
                                ActivateOutput( pActInfo, EOP_TEXNAME, m_sName );
                                ActivateOutput( pActInfo, EOP_CHANGED, true );
                            }
                        }

                        if ( !m_pVideo && m_nID != -1 )
                        {
                            m_nID = -1;
                            m_sName = "";

                            ActivateOutput( pActInfo, EOP_TEXID, m_nID );
                            ActivateOutput( pActInfo, EOP_TEXNAME, m_sName );
                            ActivateOutput( pActInfo, EOP_CHANGED, true );

                            pActInfo->pGraph->SetRegularlyUpdated( pActInfo->myID, false );
                        }

                        break;
                }
            }
    };
}

REGISTER_FLOW_NODE_EX( "Videoplayer_Plugin:OutputTexture", VideoplayerPlugin::CFlowVideoOutputTextureNode, CFlowVideoOutputTextureNode );