/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#pragma once

#include <Game.h>

#include <IPluginManager.h>
#include <IPluginBase.h>
#include <CPluginBase.hpp>

#include <IPluginD3D.h>
#include <IPluginVideoplayer.h>

#define PLUGIN_NAME "Videoplayer"
#define PLUGIN_CONSOLE_PREFIX "[" PLUGIN_NAME " " PLUGIN_TEXT "] " //!< Prefix for Logentries by this plugin

namespace VideoplayerPlugin
{
    /**
    * @brief Provides information and manages the resources of this plugin.
    */
    class CPluginVideoplayer :
        public PluginManager::CPluginBase
    {
        public:
            CPluginVideoplayer();
            ~CPluginVideoplayer();

            // IPluginBase
            bool Release( bool bForce = false );

            int GetInitializationMode() const
            {
                return int( PluginManager::IM_Default );
            };

            bool Init( SSystemGlobalEnvironment& env, SSystemInitParams& startupParams, IPluginBase* pPluginManager, const char* sPluginDirectory );

            bool InitDependencies();

            const char* GetVersion() const
            {
                return "1.7.0.0";
            };

            const char* GetName() const
            {
                return PLUGIN_NAME;
            };

            const char* GetCategory() const
            {
                return "Visual";
            };

            const char* ListAuthors() const
            {
                return "Hendrik Polczynski <hendrikpolczyn at gmail dot com>";
            };

            const char* ListCVars() const;

            const char* GetStatus() const;

            const char* GetCurrentConcreteInterfaceVersion() const
            {
                return "1.0";
            };

            void* GetConcreteInterface( const char* sInterfaceVersion );

            PluginManager::IPluginBase* GetBase()
            {
                return static_cast<PluginManager::IPluginBase*>( this );
            };
    };

    extern CPluginVideoplayer* gPlugin;
    extern D3DPlugin::IPluginD3D* gD3DSystem;
}

/**
* @brief This function is required to use the Autoregister Flownode without modification.
* Include the file "CPluginVideoplayer.h" in front of flownode.
*/
inline void GameWarning( const char* sFormat, ... ) PRINTF_PARAMS( 1, 2 );
inline void GameWarning( const char* sFormat, ... )
{
    va_list ArgList;
    va_start( ArgList, sFormat );
    VideoplayerPlugin::gPlugin->LogV( ILog::eWarningAlways, sFormat, ArgList );
    va_end( ArgList );
};
