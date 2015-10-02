--[[
    This file is part of Ice Lua Components.

    Ice Lua Components is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ice Lua Components is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ice Lua Components.  If not, see <http://www.gnu.org/licenses/>.
]]

-- Dear melchips: Please don't use this list to tell people what's NOW SUPPORTED in a version.
-- Use it to tell people what's BROKEN or POTENTIALLY broken.
-- Thanks. --GM

VERSION_ENGINE = {
	cmp={0,2,1,0,30},
	num=8421376+30,
	str="0.2.1-30",
}

--error(""..common.version.num)

-- 0.1: 4194304
-- 0.1.1: 4227072
-- 0.1.2: 4259840
-- 0.2: 8388608
-- 0.2a: 8389632
-- 0.2.1: 8421376

VERSION_BUGS = {
{intro=nil, fix=1, msg="PMF models have the wrong Z value when close to the screen edges, and can be seen through walls"},
{intro=nil, fix=1, msg="PMF models are sometimes saved with garbage following the name"},
{intro=nil, fix=1, msg="Client does pathname security checks for non-clsave files"},
{intro=nil, fix=3, msg="common.img_fill not implemented (this wrapper will be somewhat slow)"},
{intro=nil, fix=4, msg="Sound is not supported"},
{intro=4, fix=5, msg="Dedicated server build was broken"},
{intro=5, fix=6, msg="CRASHES ON CHANNEL WRAPAROUND - PLEASE UPDATE TO 0.0-6!"},
{intro=nil, fix=7, msg="Camera roll / camera_point_sky not implemented - drunken cam will not roll properly"},
{intro=nil, fix=8, msg="Renderer uses double-rect approximation of cube instead of using trapezia"},
{intro=nil, fix=9, msg="Mouse warping not implemented"},
{intro=nil, fix=10, msg="Immediate ceiling isn't drawn"},
{intro=nil, fix=11, msg="Blocks appear inverted in common cases"},
{intro=nil, fix=12, msg="TGA loader prone to crashing on unsanitised data"},
{intro=nil, fix=13, msg="No per-face shading"},
{intro=nil, fix=14, msg="Color conversion functions are using hacky Lua code"},
{intro=nil, fix=16, msg="Per-face shading is only preliminary"},
{intro=nil, fix=17, msg="OpenMP not supported"},
{intro=nil, fix=19, msg="OpenGL not supported"},
{intro=19, fix=22, msg="Preliminary OpenGL support (fog not supported yet)"},
{intro=19, fix=21, msg="OpenGL renderer ignores islocal flag when rendering PMFs"},
{intro=21, fix=22, msg="PMF renderer does not update bones when redefined"},
{renderer="gl", intro=19, fix=23, msg="[OpenGL] texture rendering is slow"},
{renderer="gl", intro=19, fix=24, msg="[OpenGL] VBOs not supported"},
{intro=nil, fix=26, msg="TODO: give changelog for -25/-26 (which I think are the same version more or less)"},
{intro=nil, fix=27, msg="Altered the international keyboard thing to be more backwards compatible"},
{intro=25, fix=27, msg="THIS VERSION IS INCOMPATIBLE. PLEASE UPGRADE TO 0.0-27 AT LEAST."},
{intro=nil, fix=28, msg="Sound loader only loads first half of 16-bit samples correctly"},
{renderer="gl", intro=19, fix=29, msg="[OpenGL] Crashes on map creation (as opposed to map loading)"},
{intro=nil, fix=30, msg="clsave/config.json not supported"},
{intro=30, fix=31, msg="broke dedicated server build... again"},
{intro=nil, fix=32, msg=".it module music not supported"},
--{intro=32, fix=nil, msg=".it module music might have stability issues. If it crashes, please tell us :)"},
{intro=nil, fix=34, msg="Server must be manually seeded"},
{intro=33, fix=34, msg="A few compilation warnings that shouldn't be there"},
{renderer="gl", intro=22, fix=35, msg="[OpenGL] Smooth lighting not supported"},
--{renderer="gl", intro=35, fix=nil, msg="[OpenGL] Smooth lighting of PMF models not supported"},
{renderer="softgm", intro=37, fix=39, msg="[softgm] Preliminary smooth lighting (WIP)"},
{renderer="gl", intro=22, fix=38, msg="[OpenGL] Rendering tends to stutter on some cards"},
{renderer="gl", intro=38, fix=4227072+2, msg="[OpenGL] Preliminary stutter-reduced rendering (WIP)"},
{renderer="gl", intro=22, fix=40, msg="[OpenGL] Chunks rendering options not supported in game engine config file"},
{renderer="gl", intro=22, fix=41, msg="[OpenGL] option to disable VBOs not available"},
{renderer="gl", intro=22, fix=42, msg="[OpenGL] lack of support for cards w/o non-power-of-2 texture support"},
{intro=nil, fix=43, msg="File transfer cancellation not supported"},
{intro=nil, fix=44, msg="[Windows] MessageBox added to 0.0-44 telling people not to double-click the .exe files"},
{intro=nil, fix=44, msg="[Windows binary build] stdout/stderr is now moved to the commandline in 0.0-44 - upgrade!"},
{intro=nil, fix=45, msg="Inbuilt tutorial not available in this version"},
{intro=nil, fix=45, msg="libsackit is out of date and does not support IT 2.14p3 resonant filters"},
{renderer="gl", intro=22, fix=46, msg="[OpenGL] PMF models tend to z-fight"},
{intro=nil, fix=47, msg="Binary file loading/saving not supported"},
{renderer="gl", intro=nil, fix=48, msg="[OpenGL] Chunk count is static and does not adapt to different fog values"},
{intro=nil, fix=48, msg="No way to determine from the Lua end what renderer a client is using"},
{renderer="gl", intro=nil, fix=48, msg="[OpenGL] Chunk generation pattern kinda sucks"},
{intro=nil, fix=49, msg="Kick not handled gracefully"},
{intro=49, fix=nil, msg="Kick message isn't communicated properly"},
{intro=nil, fix=50, msg="Network handle architecture changed. If it breaks, upgrade. If your mods break, FIX THEM."},
{intro=nil, fix=51, msg="ENet protocol not supported"},
{intro=51, fix=52, msg="Server tends to crash when a TCP connection loads and there's at least one other client still connected"},
{intro=51, fix=53, msg="Local mode (-s) broken and causes a crash"},
{intro=nil, fix=53, msg="Timing accuracy somewhat bad (uses a float instead of a double, mostly an issue for sec_current)"},
{intro=nil, fix=53, msg="There are some weird network stability issues"},
{intro=nil, fix=4194304+0, msg="Binary files don't have a type name"},
{intro=nil, fix=4194304+0, msg="JSON files cannot be remotely sent to clients"},
{intro=nil, fix=4194304+1, msg="Arbitrary TCP connections not supported"},
{intro=4194304+1, fix=4194304+2, msg="This build doesn't actually compile on not-windows because itoa isn't a real function."},
{intro=4194304+1, fix=4194304+2, msg="Raw TCP connection throws an error on failure"},
{intro=4194304+1, fix=4194304+9, msg="Raw TCP appears to ignore the whitelist on the client side"},
{intro=nil, fix=nil, msg="Occasional crash in sackit_module_free on common.mus_free"},
{intro=nil, fix=nil, msg="Sound distance attenuation affected by zoom (workaround implemented)"},
{renderer="gl", intro=nil, fix=4194304+3, msg="[OpenGL] Frustum culling not supported"},
{renderer="gl", intro=nil, fix=4194304+4, msg="[OpenGL] Ambient occlusion on sides not rendered equally"},
{renderer="gl", intro=4194304+4, fix=4194304+5, msg="[OpenGL] Ambient occlusion on sides rendered very unequally on very rare GPUs such as the Intel HD 3000"},
{renderer="gl", intro=4194304+3, fix=4194304+7, msg="[OpenGL] Frustum culling improved in later versions"},
{renderer="gl", intro=4194304+3, fix=nil, msg="[OpenGL] Frustum culling still screws up on occasion"},
{intro=nil, fix=4194304+8, msg="Occasional crash when music is stopped"},
{intro=4194304+1, fix=4194304+9, msg="Raw TCP still throws a lua error if it can't connect"},
{intro=nil, fix=4227072+1, msg="Arbitrary UDP connections not supported"},
--{intro=4227072+1, fix=nil, msg="Raw UDP support might be a bit flaky - if you find bugs, please tell us!"},
{renderer="gl", intro=nil, fix=4227072+2, msg="[OpenGL] Breaking blocks around the edges does not update the chunks properly"},
{intro=nil, fix=4227072+3, msg="common.net_pack() reads an integer before it converts it to floating point"},
{intro=nil, fix=4227072+5, msg="Image scaling not supported"},
{renderer="softgm", intro=4227072+5, fix=4227072+7, msg="[softgm] Image scaling not supported"},
{intro=4227072+5, fix=4227072+6, msg="Image scaling accidentally only supported integers for scale parameters"},
{intro=4227072+5, fix=4227072+7, msg="Incompatible semantics for image scaling"},
{intro=nil, fix=4227072+8, msg="iceball:// URL scheme not supported"},
{intro=4227072+8, fix=4227072+9, msg="[Windows] iceball:// handler doesn't set current directory correctly"},
{intro=nil, fix=4259840, msg="Sound broken wrt stereo (only the last sound played is in stereo; the rest uses the left for both channels)"},
{renderer="softgm", intro=nil, fix=4259840+1, msg="[OSX][softgm] Colours are incorrect (32-bit endian swap)"},
{intro=nil, fix=4259840+2, msg="PNG not supported"},
{intro=4259840+2, fix=nil, msg="Preliminary PNG support - more support to come when we can be bothered"},
{intro=4259840+2, fix=4259840+4, msg="PNG reader lacks support for greyscale/indexed images"},
{intro=4259840+2, fix=4259840+4, msg="PNG reader lacks support for tRNS-block transparency"},
{renderer="gl", intro=nil, fix=4259840+3, msg="[OpenGL] Low-quality mode not supported"},
{intro=nil, fix=4259840+5, msg="Server stability is a bit crap (this bug fixed in 0.1.2-5)"},
{intro=nil, fix=4259840+6, msg="JSON writing not supported"},
{intro=nil, fix=4259840+7, msg="General JSON support is broken"},
{intro=nil, fix=4259840+9, msg="UDP sending can crash if DNS lookup fails"},
{intro=nil, fix=4259840+10, msg="Garbage collection not supported"},
{intro=4259840+10, fix=nil, msg="Garbage collection incomplete (only pmf/img/wav supported)"},
{intro=4259840+10, fix=4259840+11, msg="GARBAGE COLLECTION CRASHY AND UNSTABLE, DO NOT USE THIS VERSION"},
{intro=nil, fix=4259840+12, msg="Frame delay in client.hook_tick doesn't work properly - Frame limiter will not work"},
{intro=nil, fix=4259840+14, msg="Network serialisation broken on ARM"},
{intro=nil, fix=8388608+1, msg="Local code cannot write to clsave/pub"},
{intro=4259840+6, fix=8388608+2, msg="JSON writer crashes on 64-bit builds"},
{intro=nil, fix=8388608+2, msg="tcp_connect crashes on address failure"},
{intro=nil, fix=8388608+2, msg="argb_spit_to_merged broken on ARM"},
{intro=4259840+6, fix=8389632, msg="JSON writer not sandboxed - UPGRADE"},
{intro=nil, fix=8389632+1, msg="Lua vertex array (VA) rendering not supported"},
{intro=8389632+1, fix=8389632+2, msg="VA API rendering broken on non-VBO mode"},
{intro=8389632+1, fix=8389632+2, msg="Memory leak when reusing a VA in va_make"},
{intro=8389632+1, fix=8389632+2, msg="VA API lacks support for textures"},
{intro=8389632+2, fix=8389632+3, msg="Compat breakage with va_render_global and textures"},
{intro=4259840+11, fix=8389632+7, msg="[Windows] Launcher crashes when joining server if path contains spaces and not run from command line"},
{intro=4259840+1, fix=8421376+1, msg="VA API lacks support for blending"},
{intro=nil, fix=8421376+2, msg="Lack of depth / stencil buffer mode selection support"},
{intro=8421376+4, fix=8421376+5, msg="Crash on map_free after map_new"},
{renderer="gl", intro=8421376+2, fix=8421376+6, msg="[OpenGL] Stencil bits not set properly, resulting in red screen on some drivers"},
{renderer="gl", intro=nil, fix=8421376+7, msg="[OpenGL] Multitexturing not supported"},
{renderer="gl", intro=nil, fix=8421376+8, msg="[OpenGL] GLSL shaders not supported"},
{renderer="gl", intro=8421376+8, fix=8421376+9, msg="[OpenGL] Map and PMF normals not emitted for shaders"},
{renderer="gl", intro=8421376+7, fix=8421376+9, msg="[OpenGL] Normal information for VAs overrides colour information by mistake"},
{intro=nil, fix=8421376+10, msg="Segfault when blitting without a screen, even image-to-image"},
{renderer="gl", intro=nil, fix=8421376+12, msg="[OpenGL] VAs broken when VBOs disabled"},
{renderer="gl", intro=nil, fix=8421376+14, msg="[OpenGL] Vertex attributes not supported in shaders"},
{intro=nil, fix=8421376+15, msg="map_render not supported"},
{intro=8421376+15, fix=8421376+20, msg="map_render support incomplete"},
{intro=nil, fix=8421376+16, msg="FBOs not supported"},
{intro=8421376+16, fix=nil, msg="FBO support incomplete"},
{intro=nil, fix=8421376+21, msg="gl_flip_quads incomplete"},
{intro=8421376+22, fix=8421376+23, msg="img_dump not sandboxed - UPGRADE"},
{intro=nil, fix=8421376+26, msg="IMA ADPCM samples not supported"},
{intro=nil, fix=8421376+27, msg=".it module music plays at wrong frequency when mixing freq not 44100Hz"},
{intro=nil, fix=8421376+28, msg="Potential infinite loop due to zero time delta"},
{intro=nil, fix=8421376+29, msg="B-format audio system not supported"},
{intro=8421376+29, fix=nil, msg="B-format audio system incomplete"},
{intro=nil, fix=8421376+30, msg="net_pack/unpack_array not supported"},
{intro=nil, fix=8421376+30, msg="string mode bugged in net_unpack"},
{intro=nil, fix=nil, msg="string mode bugged in net_pack, net_*_array"},
}

