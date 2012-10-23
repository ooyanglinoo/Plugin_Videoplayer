/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <StdAfx.h>
#include <Nodes/G2FlowBaseNode.h>
#include <CPluginVideoplayer.h>
#include <IVideoplayerSystem.h>
#include <ICryAnimation.h>
#include <CVideoplayerSystem.h>

namespace VideoplayerPlugin
{
    class CFlowVideoOutputEntityNode : public CFlowBaseNode<eNCT_Instanced>
    {
        private:
            IVideoplayer* m_pVideo;
            IEntity* m_pEntity;
            bool m_bSoundSource;

            enum EInputPorts
            {
                EIP_OVERRIDE = 0,
                EIP_RESET,
                EIP_SOUNDSOURCE,
                EIP_VIDEOID,
                EIP_SLOT,
                EIP_SUBMAT,
                EIP_TEXSLOT,
                EIP_RECOMMENDED,
            };

        public:
            CFlowVideoOutputEntityNode( SActivationInfo* pActInfo )
            {
                m_pVideo = NULL;
                m_pEntity = NULL;
                m_bSoundSource = false;
            }

            virtual ~CFlowVideoOutputEntityNode()
            {
                // TODO not very safe, we need to check if entity is still existing ;(
                if ( m_bSoundSource && m_pVideo )
                {
                    m_pVideo->GetSoundplayer()->RemoveSoundProxy( m_pEntity );
                }
            }

            virtual IFlowNodePtr Clone( SActivationInfo* pActInfo )
            {
                return new CFlowVideoOutputEntityNode( pActInfo );
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
                    InputPortConfig<bool>( "SoundSource",            false,      _HELP( "Set this entity as only 3D soundsource of the video" ),           "bSoundSource" ),
                    InputPortConfig<int>( "VideoID",                 -1,         _HELP( "id" ),                                            "nVideoID" ),
                    InputPortConfig<int>( "Slot",                    0,          _HELP( "slot of the entity to be modified" ),             "nSlot",        _UICONFIG( "" ) ),
                    InputPortConfig<int>( "SubMaterial",             0,          _HELP( "submaterial to be modified" ),                    "nSubMat",      _UICONFIG( "" ) ),
                    InputPortConfig<int>( "TextureSlot",             0,          _HELP( "textureslot to be modified" ),                    "nTexSlot",     _UICONFIG( "enum_int:00_DIFFUSE=0,01_BUMP=1,02_GLOSS=2,03_ENV=3,04_DETAIL_OVERLAY=4,05_BUMP_DIFFUSE=5,06_BUMP_HEIGHT=6,07_DECAL_OVERLAY=7,08_SUBSURFACE=8,09_CUSTOM=9,10_CUSTOM_SECONDARY=10,11_OPACITY=11" ) ),
                    InputPortConfig<bool>( "RecommendedSettings",    true,       _HELP( "modify shader and lightning for optimal colors" ), "bRecommendedSettings" ),
                    {0},
                };

                config.pInputPorts = inputs;
                config.pOutputPorts = NULL;//outputs;
                config.sDescription = _HELP( PLUGIN_CONSOLE_PREFIX "Videodestination/entity material (also optional Sounddestination)" );

                config.nFlags |= EFLN_TARGET_ENTITY;
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

                    case eFE_SetEntityId:
                        m_pEntity = pActInfo->pEntity;
                        break;

                    case eFE_Activate:
                        if ( IsPortActive( pActInfo, EIP_OVERRIDE ) && m_pEntity )
                        {
                            m_pVideo = gVideoplayerSystem->GetVideoplayerById( GetPortInt( pActInfo, EIP_VIDEOID ) );

                            if ( m_pVideo )
                            {
                                if ( GetPortBool( pActInfo, EIP_SOUNDSOURCE ) )
                                {
                                    m_bSoundSource = true;
                                    m_pVideo->GetSoundplayer()->AddSoundProxy( m_pEntity );
                                }

                                else if ( m_bSoundSource )
                                {
                                    m_pVideo->GetSoundplayer()->RemoveSoundProxy( m_pEntity );
                                    m_bSoundSource = false;
                                }

                                SEntitySlotInfo slotInfo;
                                int nSlot   = CLAMP( GetPortInt( pActInfo, EIP_SLOT ), -1, m_pEntity->GetSlotCount() - 1 );

                                // -1 means all slots
                                int iMin    = nSlot >= 0 ? nSlot : 0;
                                int iMax    = nSlot >= 0 ? nSlot : m_pEntity->GetSlotCount() - 1;

                                // iterate over all selected slots (e.g. archtype entities)
                                for ( int i = iMin; i <= iMax; ++i )
                                {
                                    memset( &slotInfo, 0, sizeof( slotInfo ) );

                                    if ( m_pEntity->GetSlotInfo( i, slotInfo ) )
                                    {
                                        if ( slotInfo.pCharacter )
                                        {
                                            // TODO maybe move later to character node
                                            gVideoplayerSystem->OverrideMaterial( m_pVideo, slotInfo.pCharacter->GetMaterial(), GetPortInt( pActInfo, EIP_SUBMAT ), GetPortInt( pActInfo, EIP_TEXSLOT ), GetPortBool( pActInfo, EIP_RECOMMENDED ) );
                                        }

                                        if ( slotInfo.pStatObj )
                                        {
                                            gVideoplayerSystem->OverrideMaterial( m_pVideo, slotInfo.pStatObj->GetMaterial(), GetPortInt( pActInfo, EIP_SUBMAT ), GetPortInt( pActInfo, EIP_TEXSLOT ), GetPortBool( pActInfo, EIP_RECOMMENDED ) );
                                        }

                                        if ( slotInfo.pChildRenderNode )
                                        {
                                            // TODO this provides instance based overrides
                                            gVideoplayerSystem->OverrideMaterial( m_pVideo, slotInfo.pChildRenderNode->GetMaterialOverride(), GetPortInt( pActInfo, EIP_SUBMAT ), GetPortInt( pActInfo, EIP_TEXSLOT ), GetPortBool( pActInfo, EIP_RECOMMENDED ) );
                                        }

                                        if ( slotInfo.pMaterial )
                                        {
                                            gVideoplayerSystem->OverrideMaterial( m_pVideo, slotInfo.pMaterial, GetPortInt( pActInfo, EIP_SUBMAT ), GetPortInt( pActInfo, EIP_TEXSLOT ), GetPortBool( pActInfo, EIP_RECOMMENDED ) );
                                        }
                                    }
                                }

                                // e.g. for normal entities
                                if ( nSlot <= 0 || m_pEntity->GetSlotCount() <= 0 )
                                {
                                    gVideoplayerSystem->OverrideMaterial( m_pVideo, m_pEntity->GetMaterial(), GetPortInt( pActInfo, EIP_SUBMAT ), GetPortInt( pActInfo, EIP_TEXSLOT ), GetPortBool( pActInfo, EIP_RECOMMENDED ) );
                                }
                            }
                        }

                        if ( IsPortActive( pActInfo, EIP_RESET ) && m_pEntity )
                        {
                            if ( m_bSoundSource && m_pVideo )
                            {
                                m_pVideo->GetSoundplayer()->RemoveSoundProxy( m_pEntity );
                            }

                            SEntitySlotInfo slotInfo;
                            int nSlot   = CLAMP( GetPortInt( pActInfo, EIP_SLOT ), -1, m_pEntity->GetSlotCount() - 1 );

                            // -1 means all slots
                            int iMin    = nSlot >= 0 ? nSlot : 0;
                            int iMax    = nSlot >= 0 ? nSlot : m_pEntity->GetSlotCount() - 1;

                            // iterate over all selected slots
                            for ( int i = iMin; i <= iMax; ++i )
                            {
                                memset( &slotInfo, 0, sizeof( slotInfo ) );

                                if ( m_pEntity->GetSlotInfo( i, slotInfo ) )
                                {
                                    if ( slotInfo.pCharacter )
                                    {
                                        // TODO maybe move later to character node
                                        gVideoplayerSystem->ResetMaterial( slotInfo.pCharacter->GetMaterial(), GetPortInt( pActInfo, EIP_SUBMAT ), true );
                                    }

                                    if ( slotInfo.pStatObj )
                                    {
                                        gVideoplayerSystem->ResetMaterial( slotInfo.pStatObj->GetMaterial(), GetPortInt( pActInfo, EIP_SUBMAT ), true );
                                    }

                                    if ( slotInfo.pChildRenderNode )
                                    {
                                        // TODO this provides instance based overrides
                                        gVideoplayerSystem->ResetMaterial( slotInfo.pChildRenderNode->GetMaterialOverride(), GetPortInt( pActInfo, EIP_SUBMAT ), true );
                                    }

                                    if ( slotInfo.pMaterial )
                                    {
                                        gVideoplayerSystem->ResetMaterial( slotInfo.pMaterial, GetPortInt( pActInfo, EIP_SUBMAT ), true );
                                    }
                                }
                            }

                            // e.g. for normal entities
                            if ( nSlot <= 0 || m_pEntity->GetSlotCount() <= 0 )
                            {
                                gVideoplayerSystem->ResetMaterial( m_pEntity->GetMaterial(), GetPortInt( pActInfo, EIP_SUBMAT ), true );
                            }
                        }

                        break;

                    case eFE_Update:
                        break;
                }
            }
    };
}

REGISTER_FLOW_NODE_EX( "Videoplayer_Plugin:OutputEntity", VideoplayerPlugin::CFlowVideoOutputEntityNode, CFlowVideoOutputEntityNode );