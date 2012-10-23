/* Videoplayer_Plugin - for licensing and copyright see license.txt */

#pragma once

// Insert your headers here
#include <platform.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <list>
#include <functional>
#include <limits>
#include <smartptr.h>
#include <CryThread.h>
#include <Cry_Math.h>
#include <ISystem.h>
#include <I3DEngine.h>
#include <IInput.h>
#include <IConsole.h>
#include <ITimer.h>
#include <ILog.h>
#include <IGameplayRecorder.h>
#include <ISerialize.h>

#ifndef _FORCEDLL
#define _FORCEDLL
#endif

#ifndef VIDEOPLAYERPLUGIN_EXPORTS
#define VIDEOPLAYERPLUGIN_EXPORTS
#endif

#pragma warning(disable: 4018)  // conditional expression is constant

// can be used to partially disable the system (used to find crash sources)
/*
#define VP_DISABLE_OVERRIDE // least disabled, no material manipulation
#define VP_DISABLE_RENDER // no gpu texture updates
#define VP_DISABLE_RESOURCE // no gpu resource creation
#define VP_DISABLE_DECODE // no decoding
#define VP_DISABLE_SYSTEM // most disabled, don't initialize system
*/

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
