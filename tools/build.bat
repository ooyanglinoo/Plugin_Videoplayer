call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" x86

MSBuild "../project/Videoplayer.vcxproj" /t:Rebuild /p:Configuration=Release

call "%VS100COMNTOOLS%..\..\VC\vcvarsall.bat" x64

MSBuild "../project/Videoplayer.vcxproj" /t:Rebuild /p:Configuration=Release
