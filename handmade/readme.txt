Welcome to Handmade Hero!

If you are new to the series, the royal we strongly recommend watching
at least the first episode of Handmade Hero before trying to build the
source code, just so that you are familiar with the build
methodology.  You can find it here:

https://forums.handmadehero.org/jace/videos/win32-platform/day001.html

If you'd rather go commando, you can try it without the video:

1) The way the directory structure is normally set up is (with w:
standing in for whatever drive you wish):

w:\handmade\code - the code is in here
w:\handmade\data - the contents of the test_assets zip gets unpacked here
w:\handmade\misc - other stuff goes here (emacs config, shell scripts)
w:\build - where the build results go

2) To build the game, you have to have Visual Studio 2013 installed.
You can get it for free from Microsoft here:

https://www.visualstudio.com/en-us/products/visual-studio-community-vs.aspx

Once you have installed it, you will need to start a command shell and
run "vcvarsall.bat x64" to set up the paths and environment variables
for building C programs from the command line.

You can then build Handmade Hero at any time by going to
w:\handmade\code and running the build.bat there.  This will build
the game into w:\build.

3) To run the game, you must be in the w:\handmade\data directory, and
you should run the w:\build\win32_handmade.exe executable.  If you
would like to run it from the Visual Studio debugger, you must load
win32_handmade.exe in the debugger and then use the Solution Explorer
to set the working directory to w:\handmade\data.

You MUST have unzipped the asset ZIP into the data directory in order
for the game to work.  If you haven't downloaded the asset ZIP, you
can get it from the same link you used to download the source code.

4) If you have problems or questions, head over to the forums and
post them:

https://forums.handmadehero.org/index.php/forum?view=category&catid=4

Happy coding!
- Casey
