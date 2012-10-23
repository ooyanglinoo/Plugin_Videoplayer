/* Videoplayer_Plugin - for licensing and copyright see license.txt */

VideoplayerPlugin::IVideoplayerSystemPtr gVideoplayerSystem; //!< Global videoplayer system pointer inside game dll

namespace VideoplayerPlugin
{
    /**
    * @brief Initialize the Videoplayer Plugin
    * This code must be called once in the game dll GameStartup after initializing the D3D Plugin.,
    * @return success
    * @param startupParams CryEngine Startup Params
    * @param d3dsys Pointer to the initialized D3D Plugin.
    */
    bool InitVideoplayerSystem( SSystemInitParams& startupParams, ID3DSystem& d3dsys )
    {
        typedef boost::shared_ptr<IEngineModule> IEngineModulePtr;
        IEngineModulePtr pModule;

        HINSTANCE hModule = CryLoadLibrary( EXTENSION_VIDEOPLAYER_SYSTEM_FILE );

        if ( hModule )
        {
            // plugin link library found

            // Initialize use of CryExtension in module
            typedef void ( *fInitSystem )( ISystem*, const char* );
            fInitSystem InitModuleFunc = ( fInitSystem )CryGetProcAddress( hModule, "ModuleInitISystem" );

            if ( InitModuleFunc )
            {
                // Entry point of plugin dll found.
                InitModuleFunc( gEnv->pSystem, EXTENSION_VIDEOPLAYER_SYSTEM_MODULE ); // initialize cryextension

                // Create/Initialize Module
                if ( CryCreateClassInstance( EXTENSION_VIDEOPLAYER_SYSTEM_MODULE, pModule ) )
                {
                    pModule->Initialize( *gEnv, startupParams ); // initialize the plugin module

                    // Create/Initialize Module
                    if ( CryCreateClassInstance( EXTENSION_VIDEOPLAYER_SYSTEM, gVideoplayerSystem ) )
                    {
                        gVideoplayerSystem->Initialize( d3dsys ); // supply d3d system for videoplayer plugin

                        return true; // Success
                    }
                }
            }
        }

        return false; // Failure
    }
}
