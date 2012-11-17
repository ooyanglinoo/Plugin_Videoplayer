Videoplayer Plugin 1.7 (19.11.2012)
==============
Stable Release for CryEngine 3.4.3 (32/64 bit, DX9 & DX11), Plugin SDK 1.1

New:
* Support for CryEngine 3.4.3
* Support for playlist lua commands and conditions
* Texture filter now configurable (Nearest or Linear), Linear is the new default
* Installer

Fixes:
* Now handles entity deletion if sound was playing on entity (possible crash)
* Don't report empty sounds as errors

Known Limitations:
* Plugin is partly disabled in 32bit launcher when using DX11

Videoplayer Plugin 1.6.2 (02.11.2012)
==============
Stable Release for 32/64 bit, FreeSDK 3.4.0 (DX9 & DX11), Plugin SDK 1.0

Known Limitations:
* Plugin is partly disabled in 32bit launcher when using DX11
* Known DX11 CryEngine3 FreeSDK 3.4 Bug (don't reload your map)

New:
* Now using the Plugin SDK
* Support for playlist commands like loading a map as menu background
  (they can also be delayed/canceled to create cvar animations, like time of the day or subtitles)
* Support for showing default menu on playlist end
* Full sourcecode avaible under BSD-2 clause license

Changes:
* Improved SSE2 YUV conversion (Performance: 2x1080p + 2x720p in parallel on i7-920 DX9 ~50fps DX11 ~25fps)
* Less Console logging
* New samples

Fixes:
* DirectX 9 mode can resize/fullscreen the renderer again without restarting
* Speed attribute now works also in playlist

Videoplayer Plugin 1.6.1 (07.09.2012)
==============
Stable Release for 32/64 bit FreeSDK 3.4 (DX9 & DX11)

Known Limitations:
* Plugin is partly disabled in 32bit Launcher when using DX11
* Known DX11 CryEngine3 FreeSDK 3.4 Bug (don't reload your map)

New:
* Documentation of SoundSource for Playlist XML Output2D in readme

Changes:
* Now under BSD-2 clause license (previously CC-BY 3.0)

Fixes:
* 2D Sound sometimes not starting immediately fixed, now retry until sound plays (Preload issue)

Videoplayer Plugin 1.6.0 (19.08.2012)
==============
Stable Release for 32/64 bit FreeSDK 3.4 (DX9 & DX11)

Known Limitations:
* Plugin is partly disabled in 32bit Launcher when using DX11
* Known DX11 CryEngine3 FreeSDK 3.4 Bug (don't reload your map)

New:
* Additional drop modes
* CVar to keep Editor in playback mode instead of resetting material (vp_playbackmode=0-2)
* CVars to fine tune synchronization (vp_seekthreshold, vp_dropthreshold, vp_dropmaxduration)
* Basic sample monitor assets by kimba23
* Basic sample level showcasing the flownodes
* Sample Video: Tears of Steel Teaser and Sample FMOD project

Changes:
* Using libvpx 1.1 (Eider) to play WebM/RAW/IVF files
* Everything moved into namespace VideoplayerPlugin
* Internal refactoring and doxygen documentation in preparation of sourcecode release
* Using astyle to format the sourcecode
* Using aligned memory for improved performance
* Better synchronization for systems with low performance

Fixes:
* D3DPlugin 1.6 fixed a sporadic stackoverflow and increased thread safety

Videoplayer Plugin 1.5.0 (13.06.2012)
==============
Stable Release for 32/64 bit FreeSDK 3.4 (DX9 & DX11)

New:
* DirectX 11 Renderer
* Playlists
* Events to trigger Splashscreens / Menubackground playlists
  (Game Start, Start Menu, Ingame Menu, Level Start)
* 3D Sound on multiple entities
* Added Flownode Videoplayer_Plugin:OutputTexture for custom usage of textures
* Added Flownode Videoplayer_Plugin:InputPlaylist for playlist support
* Editor Support (Everything is closed/restored when quitting gamemode)
* YUV conversion fallback: GFX accelerated or CPU (SSE2 or normal)
* Fallback Software Renderer (if D3DPlugin finds unknown D3D version)
* Sample Videos: Background, CryEngine3 logo, Autodesk Gameware (Scaleform)
* Sample Playlists: for all auto events

Changes:
* Refactored internal Rendererhandling
* Refactored internal Soundhandling
* Improved Soundsynchronization
  (Frame dropping and/or automatic seeking)
* Renamed "Input" Flownode to "InputWebM"

Fixes:
* Sounds sometimes not starting
* Video sometimes waiting after triggering seek

Bugfixes from 1.5 beta:
* YUV conversion fallback not activating
  now also working on onboard intel graphic cards
* Removal of deprecated "Input" Flownode
* DirectX 9 mode not working

Thanks to:
* Nils L. Corneliusen for his SSE2 based YUV conversion
   visit his website at http://ignorantus.com
* fractal.design and kimba23 for beta testing

Videoplayer Plugin 1.1.0 (02.05.2012)
==============
Stable Release for FreeSDK 3.3.9 and 3.4 (DX9)

New:
* 2D/Fullscreen Videosupport  (multiple simultaneous 2D videos possible)
  supports scaling, aspect ratio, placement, rotation,
  black border (or colored), transparency...
* Added Flownode Videoplayer_Plugin:Output2D
* Added Flownode Videoplayer_Plugin:OutputEntity
* Selectable time synchronization source: Sound, GameTime, SystemTime
* Support for bullet time when using the GameTime timesource

Changes:
* Moved/Renamed all Flownodes to "Videoplayer_Plugin"
  so that the names reflect the functionality better.
  (This is a breaking change but will be the last for the Flownodes)

Fixes:
* Crash if DX11 is used while not yet supported
* Sporadic crash on quit while video was playing

Thanks to:
* VABG, Lex4art for reporting and testing the YUV conversion bug
* Hamers for being a fair rival ;)

Notes:
* Changing the speed of a file is not working together with sound.
  (because fmod pitch and pitchshift isn't working completly)
* The 3.3.9 version doesn't support borders in 2D mode
  (because of renderer features only avaible in 3.4 version)

Videoplayer Plugin 1.0.1 (19.04.2012)
==============
Initial Stable Release for FreeSDK 3.3.9 and 3.4 (DX9)

New:
* Added Support to restore orginal material
* FreeSDK 3.4 (DX9) Support

Changes:
* C++ Videomaterial Interface changed
* Readme improved
* Video_Flowgraph.xml improved
* Using libvpx 1.0 (Duclair) to play WebM/RAW/IVF files

Fixes:
* Fixed Shutdown / Mapchange / Resolution change / Fullscreen Crashes
* Fixed YUV Colorconversion for video resolutions not divisible by 4 (now automatic cropping)
* Fixed StartAt in combination with Seek or Videosource changes
* Fixed Flownode OnEnd so that it only triggers when the End is reached

Videoplayer Plugin 0.9 (06.04.2012)
==============
Initial Public Beta Release for FreeSDK 3.3.9 (DX9)

New:
* Play Videos on Materials
* Flownode to select inputfile and output material
* Using libvpx 0.97 (Cayuga) to play WebM/RAW/IVF files
