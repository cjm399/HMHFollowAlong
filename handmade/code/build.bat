@echo off

if not exist mkdir h:\build
pushd h:\build
cl -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -FC -Zi h:\handmade\code\win32_handmade.cpp user32.lib Gdi32.lib dsound.lib
popd
