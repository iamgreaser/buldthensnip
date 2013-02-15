GUYS GUYS GUYS
FIRST THING YOU NEED TO READ:

Read the manual in MANUAL/, otherwise you will not be able to play this!
If the MANUAL/ folder is missing, run genmanual.py with Python 2.7.

 ------------------------------------------------------------------------------

This is FINALLY a game, BUT is not ready to be released yet.

NOTE:
By Stack's request, this project is now known as "Iceball".

mesa.vxl is by Triplefox, and is currently being used to test load/render.

LICENSING NOTES:
Iceball is licensed under the regular GNU GPL version 3.
Ice Lua Components is licensed under the LGPL version 3.
All assets are released under Creative Commons 3.0 BY-SA:
  http://creativecommons.org/licenses/by-sa/3.0/

These are, unless otherwise marked:
  Copyright (C) 2012-2013, Iceball contributors.
See MANUAL/main/credits.html (source: msrc/main/credits.py) for the list,
and my apologies if I've forgotten to update it.

Ice Lua Components contains some content from libSDL,
  which is licensed under the LGPL version 2.1.
It is marked accordingly.

The manual is in the public domain, except where otherwise specified.

REQUIREMENTS:
- a C compiler that isn't crap (read: not MSVC++)
  - specifically, GCC
  - MinGW is a port of GCC for Windows: http://mingw.org/
  - if you use something else we might consider compatibility with it
- SDL 1.2 (not 1.3) - http://libsdl.org/
- Lua 5.1 (not 5.2) - http://lua.org/
- zlib - http://zlib.net/
- GNU make
  - if someone has BSD make, please tell us :)

STUFF TO DO BEFORE 0.1 CAN BE RELEASED:
- DOCS!!! (ones which aren't crap)
- clsave/config.json
- make net_pack more solid
- JSON writer
- make kicking not suck

MSVC readme (wip):
- create a folder 'winlibs' in the buldenthesnip dir
  dump all dll's + lib's in this folder (opengl,lua,zlib, sdl, glew)
  dump all includes in submaps (glew in glew submap, and so on)
  /buldenthesnip/
    /winlibs/
	  /glew/
	  /lua/
	  /SDL/
	  /zlib/
	  glew32.lib
	  glew32.dll
	  lua5.1.lib
	  lua5.1.dll
	  and so on..

- right mouse on project -> properties.
  Working directory (without quotes): '$(SolutionDir)/../'
  Command Arguments (without quotes):
	'-c iceballga.me 20737'  (connect to srv)
	'-s 0 pkg/base' (make local srv)
- edit clsave/pub/user.json
- now run it from vs.net debugger :)