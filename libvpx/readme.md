Building libvpx with Visual Studio 2010
=======================================

* Download vsyasm for VS 2010 http://yasm.tortall.net/Download.html
* Download x64 configuration http://code.google.com/p/webm/downloads/list
* Download win32 configuration http://code.google.com/p/webm/downloads/list

For each Configuration
----------------------
Do the same thing for x64 and x32 build just keep the 32 bit libs combined with the x64 code. This way both versions can be used by the Videoplayer plugin.
* Convert Solution build/vpx.sln to vs2010
* Fix the Error ```vsyasm: FATAL: unable to open include file 'vpx_ports/x86_abi_support.asm'```
  * Open the file "src\build\x86-msvs\yasm.xml"
  * Replace occurences of ```Separator=";"``` with nothing (Delete them) 
    (see http://code.google.com/p/webm/issues/detail?id=389)
* Configurate project properties
  * Select Release
  * Enable Full Optimatization
  * Enable Intrinsics
  * Favor Speed
  * C++/Code Generation/Runtime Library = "Multi-threaded DLL"
* Build
* Copy Release/lib to lib