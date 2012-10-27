Videoplayer-Plugin
==================

Videoplayer Plugin for CryEngine featuring WebM, DirectX 9, DirectX 11 Support

Features
--------
* Create Video Splashscreens / Menubackgrounds
* Replace any ingame 3D Entity/Material/Texture with a video (from the filesystem or in pak file)
* Looping / Custom resolution / Seeking / Pause / Speed
* Play multiple video streams and reuse them on multiple targets
* Fullscreen/2D Videosupport (multiple videos also possible)
  supports scaling, aspect ratio, placement, rotation,
  black border (or colored), transparency...
* Synchronize to multiple timesources (e.g. sound, supporting bullet time or playback during pause)
* 2D Sound / 3D Sounds on Entity
* Playlists (for videos and console commands e.g. loading a map as mainmenu background)
* Flowgraphnodes
* C++ Interface

Release Notes (V1.6.2)
==============
Stable Release tested with:
* CryEngine 3 FreeSDK Version 3.4.0
* DirectX 11 and 9
* 32 and 64 Bit
* Plugin SDK Version 1.0

This shouldn't be an issue anymore in the next FreeSDK version
--------------------
* Plugin is partly disabled in Launcher (32 bit) when using DX11 (ingame white texture, splashscreens work)
* Known DX11 CryEngine3 FreeSDK 3.4 Bug (don't reload your map)

Tutorial videos
===============
* [Part 3: 1.6 Demonstration](http://www.youtube.com/watch?v=I0x343yvtsM)
* [Part 2: 1.5 Features and Playlists](http://www.youtube.com/watch?v=AGEEjqRHfTU)
* [Part 1: Introduction and Howto play video on ingame objects](http://www.youtube.com/watch?v=g0feGWMsSCE) (Version 1.0.1 so expect some differences)

Installation / Integration
==========================
This plugin requires the Plugin SDK to be installed.
The plugin manager will automatically load up the plugin when the game/editor is restarted or if you directly load it.

Users
-----
* Extract the files to your CryEngine SDK Folder so that the Code and BinXX/Plugins directory match up.

Designers
---------
* If you want the sample logos/backgrounds and level please download the additional sample package.

Developers
----------
* If you want to use video functions using C++ please clone the latest stable branch and add the "..\Plugin_Videoplayer\inc" path to your include directories
* For the Debug Configuration please download the [DirectX SDK](http://www.microsoft.com/en-us/download/details.aspx?id=6812)

CVars
=====
* ```vp_playbackmode``` Editor Playbackmode
  * 0 Restore materials to their orginal state when exiting game mode (Default)
  * 1 Don't restore materials when exiting the game mode
  * 2 Don't restore materials and keep videos playing when exiting the game mode<br/>
    **This mode is dangerous and resource intensive, so use it wisely**

* ```vp_seekthreshold``` Synchronization Seek Threshold<br/>
  Start seeking when detecting at least x second lag (Default 5 seconds)

* ```vp_dropthreshold``` Synchronization Drop Threshold<br/>
  Start dropping when detecting at least x second lag (Default 0.1 seconds)

* ```vp_dropmaxduration``` Synchronization Maximal Drop Duration<br/>
  Maximal duration in seconds to drop before outputing at least one frame (Default 0.1 seconds)

Flownodes
=========
TODO: Describe the flownodes inside your plugin

