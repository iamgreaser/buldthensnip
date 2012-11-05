/*
    This file is part of Buld Then Snip.

    Buld Then Snip is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Buld Then Snip is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Buld Then Snip.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "common.h"
#include "config.h"

camera_t tcam;
map_t *clmap = NULL;
map_t *svmap = NULL;

SDL_Surface *screen = NULL;
int screen_width = 800;
int screen_height = 600;

char *fnmap = "mesa.vxl";

int error_sdl(char *msg)
{
	fprintf(stderr, "%s: %s\n", msg, SDL_GetError());
	return 1;
}

int error_perror(char *msg)
{
	perror(msg);
	return 1;
}

int platform_init(void)
{
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE))
		return error_sdl("SDL_Init");
	
	return 0;
}

int video_init(void)
{
	SDL_WM_SetCaption("buld then snip",NULL);
	
	screen = SDL_SetVideoMode(screen_width, screen_height, 32, 0);
	
	if(screen == NULL)
		return error_sdl("SDL_SetVideoMode");
	
	return 0;
}

void video_deinit(void)
{
	// don't do anything
}

void platform_deinit(void)
{
	SDL_Quit();
}

int64_t platform_get_time_usec(void)
{
	/*
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	int64_t usec = tv.tv_usec;
	int64_t sec = tv.tv_sec;
	usec += sec;
	
	return usec;
	*/
	int64_t msec = SDL_GetTicks();
	return msec*1000;
}

void run_game(void)
{
	clmap = map_load_aos(fnmap);
	
	tcam.mpx = 256.5f;
	tcam.mpz = 256.5f;
	tcam.mpy = clmap->pillars[((int)tcam.mpz)*clmap->xlen+((int)tcam.mpy)][4+1]-2.0f;
	
	tcam.mxx = 1.0f;
	tcam.mxy = 0.0f;
	tcam.mxz = 0.0f;
	tcam.myx = 0.0f;
	tcam.myy = 1.0f;
	tcam.myz = 0.0f;
	tcam.mzx = 0.0f;
	tcam.mzy = 0.0f;
	tcam.mzz = 1.0f;
	
	int i;
	
	render_vxl_redraw(&tcam, clmap);
	
	int quitflag = 0;
	
	int frame_prev = 0;
	int frame_now = 0;
	int fps = 0;
	
	float sec_curtime = 0.0f;
	float sec_lasttime = 0.0f;
	float sec_wait = 0.0f;
	int64_t usec_basetime = platform_get_time_usec();
	
	float ompx = -M_PI, ompy = -M_PI, ompz = -M_PI;
	
	while(!quitflag)
	{
		// update Lua client
		lua_getglobal(lstate_client, "client");
		lua_getfield(lstate_client, -1, "hook_tick");
		lua_remove(lstate_client, -2);
		if(lua_isnil(lstate_client, -1))
		{
			lua_pop(lstate_client, 1);
			quitflag = 1;
			break;
		}
		sec_lasttime = sec_curtime;
		int64_t usec_curtime = platform_get_time_usec() - usec_basetime;
		sec_curtime = ((float)usec_curtime)/1000000.0f;
		lua_pushnumber(lstate_client, sec_curtime);
		lua_pushnumber(lstate_client, sec_curtime - sec_lasttime);
		if(lua_pcall(lstate_client, 2, 1, 0) != 0)
		{
			printf("Lua Client Error (tick): %s\n", lua_tostring(lstate_client, -1));
			lua_pop(lstate_client, 1);
			quitflag = 1;
			break;
		}
		sec_wait += lua_tonumber(lstate_client, -1);
		lua_pop(lstate_client, 1);
		
		if(tcam.mpx != ompx || tcam.mpy != ompy || tcam.mpz != ompz)
		{
			render_vxl_redraw(&tcam, clmap);
			ompx = tcam.mpx;
			ompy = tcam.mpy;
			ompz = tcam.mpz;
		}
		
		frame_now = SDL_GetTicks();
		fps++;
		
		if(frame_now - frame_prev > 1000)
		{
			char buf[64]; // topo how the hell did this not crash at 16 --GM
			sprintf(buf, "buld then snip | FPS: %d", fps);
			SDL_WM_SetCaption(buf, NULL);
			fps = 0;
			frame_prev = SDL_GetTicks();
		}
		
		//printf("%.2f",);
		SDL_LockSurface(screen);
		//memset(screen->pixels, 0x51, screen->h*screen->pitch);
		render_cubemap(screen->pixels,
			screen->w, screen->h, screen->pitch/4,
			&tcam, clmap);
		SDL_UnlockSurface(screen);
		SDL_Flip(screen);
		
		int msec_wait = 10*(int)(sec_wait*100.0f+0.5f);
		if(msec_wait > 0)
		{
			sec_wait -= msec_wait;
			SDL_Delay(msec_wait);
		}
		
		SDL_Event ev;
		while(SDL_PollEvent(&ev))
		switch(ev.type)
		{
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				// inform Lua client
				lua_getglobal(lstate_client, "client");
				lua_getfield(lstate_client, -1, "hook_key");
				lua_remove(lstate_client, -2);
				if(lua_isnil(lstate_client, -1))
				{
					// not hooked? ignore!
					lua_pop(lstate_client, 1);
					break;
				}
				lua_pushinteger(lstate_client, ev.key.keysym.sym);
				lua_pushboolean(lstate_client, (ev.type == SDL_KEYDOWN));
				if(lua_pcall(lstate_client, 2, 0, 0) != 0)
				{
					printf("Lua Client Error (key): %s\n", lua_tostring(lstate_client, -1));
					lua_pop(lstate_client, 1);
					quitflag = 1;
					break;
				}
				break;
			case SDL_QUIT:
				quitflag = 1;
				break;
			default:
				break;
		}
	}
	map_free(clmap);
	clmap = NULL;
}

int main(int argc, char *argv[])
{
	if(!platform_init()) {
	if(!btslua_init()) {
	if(!net_init()) {
	if(!video_init()) {
	if(!render_init(screen->w, screen->h)) {
		if(argc > 1)
			fnmap = argv[1];
		
		run_game();
		render_deinit();
	} video_deinit();
	} net_deinit();
	} btslua_deinit();
	} platform_deinit();
	}
	
	return 0;
}
