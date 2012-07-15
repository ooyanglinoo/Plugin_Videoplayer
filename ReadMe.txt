To get started please read this document.
For redistribution please see LICENSE.txt and PATENTS.txt.

I would appreciate it if you would drop me a line if you plan on
using it in a specific project.

====================================================================
Release Notes
====================================================================
Stable Release:
- CryEngine 3 FreeSDK Version 3.4
- DirectX 11 and 9
- 32/64 Bit

Features:
- Create Video Splashscreens / Menubackgrounds
- Replace any ingame 3D Entity/Material/Texture with a video (from the filesystem or in pak file)
- Looping / Custom resolution / Seeking / Pause / Speed
- Play multiple video streams and reuse them on multiple targets
- 2D/Fullscreen Videosupport (multiple simultaneous 2D videos possible)
  supports scaling, aspect ratio, placement, rotation,
  black border (or colored), transparency...
- Synchronize to multiple timesources (e.g. sound, supporting bullet time or playback during pause)
- Flowgraphnodes
- C++ Interface
- 2D Sound / 3D Sounds on Entity
- Playlists

Roadmap:
- Sourcecode release on github
- More Documentation/Videotutorials
- Caching for small videos
- Multithreaded video decoding

Feature requests/latest version can be found here:
http://www.crydev.net/viewtopic.php?f=308&t=87555

The offical versions can be downloaded here:
http://www.crydev.net/project_db.php?action=project_profile&team_id=51&project_id=1919

====================================================================
Howto videos
====================================================================
Part 1: Introduction and Howto play video on ingame objects
http://www.youtube.com/watch?v=g0feGWMsSCE
(Version 1.0.1 so expect some differences)

====================================================================
Extraction
====================================================================
Extract the Files to your cryengine SDK Folder
so that the Code and BinXX directory match up.
(Select the correct FreeSDK version)

Also extract the Game/Videos folder to match up
if you want the sample logos/backgrounds.

If you have a custom GameDll then you will need
to recompile it. See "Using the Plugin in C++"

====================================================================
Preparing a video with audio
====================================================================
Download a *.webm video from the internet and place it into the Game folder.
e.g. http://www.webmfiles.org/demo-files/

or Encode your video to VP8/WebM format
e.g. using free software: XMedia Recode Portable (http://download.cnet.com/XMedia-Recode-Portable/3000-2194_4-75706636.html)
     recommended encoding settings: http://www.webmproject.org/tools/encoder-parameters/

The file should have a width and height divisible by 4 or better 16.
 (the excess pixel will be cropped for performance and compatibility reasons)

The audiotrack needs to be in a seperate file:
- Download MKVToolnix (mkvmergeui and MKVExtractGUI2)
- Rename your .webm file to .mkv
- Extract the audio track (MKVExtractGUI2)
- Extract the video track (MKVExtractGUI2)
- Create a webm file containing only the video track (mkvmergeui)
  Make sure you enable the WebM compatibility checkbox under "Global"
- Remove the orginal webm/mkv file

Final Video result:
You can now directly use the webm file containing the VP8/webm video.

Final Audio result:
You can now directly use the ogg file (sometimes doesn't play)
or if you want optimal engine compatibility encode the audiotrack to mp2
or create an fmod project containing the audio tracks.

====================================================================
Use Plugin in Flowgraph
====================================================================
- Startup the editor and load forest map.
- Open Flownode editor and open Video_Flowgraph.xml. (see Video_Flowgraph.png)
- Select the video and audiofile in the flownodes.
- Start game, one fisherhut and some tables should now show your video.

====================================================================
Playlist Auto Events
====================================================================
- Splash Screen / Game Start
File: Videos/Auto_Start.xml

- Start Menu (No Level loaded)
File: Videos/Auto_Menu.xml

- Pause Menu (While Level is loaded)
File: Videos/Auto_Menu_InGame.xml

- Level Start
File: Levels/<YourLevel>/Auto_Video.xml
Alternative: Use Flownodes

====================================================================
Playlist XML Format
====================================================================
- Node hierarchy is Videoplayer[1] > Scene[n] > Input[n] > Output[n]
- All Input nodes in one Scene play in parallel
- For Samples see Playlist_Sample.xml (for 2d usage)
  and Playlist_Material.xml (for usage with playlist flownode)

Scene Node Attributes
====================================================================
Skippable="1"
	Can the scene be skipped by skip event (default is space)

Input Node Attributes
====================================================================
Class="InputWebM"
	only this is supported atm

Video="Videos\test.webm"
	Path to a webm file

Sound="Videos\test.ogg"
	Path to Soundevent or file

Loop="0"
	Should the video be looped

Skippable="1"
	Can the video be skipped by skip event (default is space)

StartAt="0"
	Start video at position

EndAfter="0"
	End video after x seconds

TimeSource="3"
	GameTime = 1
	Sound = 2
	SystemTime = 4
	SoundOrGameTime = 3
	SoundOrSystemTime = 6

Output Node Attributes
====================================================================
Class="Output2D"
	only this is supported atm

ResizeMode="2"
	Original = 0
	Stretch = 1
	TouchInside = 2
	TouchOutside = 3

CustomAR="0"
	Custom Aspect Ratio (4:3=1.33 / 16:9=1.77)
	to keep the orginal one use 0

Top="0"
	postion 0-1 (relative)

Left="0"
	postion 0-1 (relative)

Width="1"
	size 0-1 (relative)

Height="1"
	size 0-1 (relative)

Angle="0"
	rotates the video and background by x degrees

RGBA="255,255,255,255"
	Colortint in RGBA format. (Use for all 255 to not tint the video)

BackgroundRGBA="0,0,0,255"
	Background Color in RGBA format.

ZOrder="1"
	BehindMenu = 0
	AboveMenu = 1

====================================================================
Use Plugin in C++
====================================================================

Compiler Settings
====================================================================
Open the Solution CryEngine_GameCodeOnly.sln

Open CryGame Properties
Set the Dropdowns to All Configurations, All Platforms

Add to C/C++ -> General -> Additional Include Directories:
  $(SolutionDir)..\Plugin_D3D;$(SolutionDir)..\Plugin_Videoplayer

Apply those properties and close the dialog

Edit Source Files
====================================================================
CryGame Project (Solution Explorer)
Open the following File: Startup Files / GameStartup.cpp

Add behind the existing includes the following:
  #include <ID3DSystem.h>
  #include <ID3DSystem_impl.h>
  #include <IVideoplayerSystem.h>
  #include <IVideoplayerSystem_impl.h>

Find the function CGameStartup::Init
Add at the end of the function (before return pOut) the following:
  InitD3DSystem(startupParams);
  InitVideoplayerSystem(startupParams, *gD3DSystem);

The initialization can only happen once because its a singleton.
