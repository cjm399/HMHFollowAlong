@echo off

mkdir h:\build
pushd h:\build
cl -Zi h:\handmade\code\win32_handmade.cpp user32.lib Gdi32.lib
popd
