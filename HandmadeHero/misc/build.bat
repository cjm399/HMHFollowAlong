@echo off

mkdir ..\..\Builds
pushd ..\..\Builds
cl -Zi ..\HandmadeHero\code\win32_handmade.cpp User32.lib Gdi32.lib
popd
