/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#include <IPluginBase.h>

#pragma once

/**
* @brief Videoplayer Plugin Namespace
*/
namespace VideoplayerPlugin
{
    /**
    * @brief plugin Videoplayer concrete interface
    */
    struct IPluginVideoplayer
    {
        /**
        * @brief Get Plugin base interface
        */
        virtual PluginManager::IPluginBase* GetBase() = 0;

        // TODO: Add your concrete interface declaration here
    };
};