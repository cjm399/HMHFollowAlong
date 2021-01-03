@echo off

mkdir h:\build
pushd h:\build
cl -FC -Zi h:\handmade\code\win32_handmade.cpp user32.lib Gdi32.lib dsound.lib
popd
