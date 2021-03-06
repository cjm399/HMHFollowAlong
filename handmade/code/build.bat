echo off

if not exist ..\..\build mkdir h:\build
pushd ..\..\build

REM -MT : Creates static entrance to our main program
REM -nologo : Does not print VS copywrite at top of build
REM -EHa- : Removes ability to do try-catch style exceptions
REM -Gm : Minimal rebuild (Deprecated)
REM -GR : Enables Runtime Type Info 
REM -WX : Treat warnings as errors
REM -W4 : Warning level 4
REM -wd([0-9]*) : Disables warning of type $1
REM -Oi : Use CPU intrinsics over CRL standard functions where possible.
REM -Od : Do no Optimizations
REM -FC : Display full path of source files
REM -Z7 : Generates Debugging info, safer than -Zi
 
cl -MT -nologo -EHa- -Gm- -GR- -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -Oi -Od -FC -Zi -Fmwin32_handmade.map -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1   ..\handmade\code\win32_handmade.cpp /link -opt:ref -subsystem:windows,5.10 user32.lib Gdi32.lib Winmm.lib

popd
