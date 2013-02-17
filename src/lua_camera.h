/*
    This file is part of Iceball.

    Iceball is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Iceball is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Iceball.  If not, see <http://www.gnu.org/licenses/>.
*/

// client functions
int icelua_fn_client_camera_point(lua_State *L)
{
	int top = icelua_assert_stack(L, 3, 5);
	float dx, dy, dz;
	float zoom = 1.0f, roll = 0.0f;
	
	dx = lua_tonumber(L, 1);
	dy = lua_tonumber(L, 2);
	dz = lua_tonumber(L, 3);
	if(top >= 4)
		zoom = lua_tonumber(L, 4);
	if(top >= 5)
		roll = lua_tonumber(L, 5);
	
	//printf("%f\n", zoom);
	cam_point_dir(&tcam, dx, dy, dz, zoom, roll);
	
	return 0;
}

int icelua_fn_client_camera_point_sky(lua_State *L)
{
	int top = icelua_assert_stack(L, 3, 7);
	float dx, dy, dz;
	float zoom = 1.0f;
	float sx, sy, sz;
	
	dx = lua_tonumber(L, 1);
	dy = lua_tonumber(L, 2);
	dz = lua_tonumber(L, 3);
	if(top >= 4)
		zoom = lua_tonumber(L, 4);
	sx = (top >= 5 ? lua_tonumber(L, 5) :  0.0f);
	sy = (top >= 6 ? lua_tonumber(L, 6) : -1.0f);
	sz = (top >= 7 ? lua_tonumber(L, 7) :  0.0f);
	
	//printf("%f\n", zoom);
	cam_point_dir_sky(&tcam, dx, dy, dz, sx, sy, sz, zoom);
	
	return 0;
}

int icelua_fn_client_camera_move_local(lua_State *L)
{
	int top = icelua_assert_stack(L, 3, 3);
	float dx, dy, dz;
	
	dx = lua_tonumber(L, 1);
	dy = lua_tonumber(L, 2);
	dz = lua_tonumber(L, 3);
	
	tcam.mpx += dx*tcam.mxx+dy*tcam.myx+dz*tcam.mzx;
	tcam.mpy += dx*tcam.mxy+dy*tcam.myy+dz*tcam.mzy;
	tcam.mpz += dx*tcam.mxz+dy*tcam.myz+dz*tcam.mzz;

	return 0;
}

int icelua_fn_client_camera_move_global(lua_State *L)
{
	int top = icelua_assert_stack(L, 3, 3);
	float dx, dy, dz;
	
	dx = lua_tonumber(L, 1);
	dy = lua_tonumber(L, 2);
	dz = lua_tonumber(L, 3);
	
	tcam.mpx += dx;
	tcam.mpy += dy;
	tcam.mpz += dz;

	return 0;
}

int icelua_fn_client_camera_move_to(lua_State *L)
{
	int top = icelua_assert_stack(L, 3, 3);
	float px, py, pz;
	
	px = lua_tonumber(L, 1);
	py = lua_tonumber(L, 2);
	pz = lua_tonumber(L, 3);
	
	tcam.mpx = px;
	tcam.mpy = py;
	tcam.mpz = pz;

	return 0;
}

int icelua_fn_client_camera_get_pos(lua_State *L)
{
	int top = icelua_assert_stack(L, 0, 0);
	
	lua_pushnumber(L, tcam.mpx);
	lua_pushnumber(L, tcam.mpy);
	lua_pushnumber(L, tcam.mpz);
	
	return 3;
}

int icelua_fn_client_camera_get_forward(lua_State *L)
{
	int top = icelua_assert_stack(L, 0, 0);
	
	lua_pushnumber(L, tcam.mzx);
	lua_pushnumber(L, tcam.mzy);
	lua_pushnumber(L, tcam.mzz);
	
	return 3;
}

int icelua_fn_client_screen_get_dims(lua_State *L)
{
	int top = icelua_assert_stack(L, 0, 0);
	
#ifdef DEDI
	return luaL_error(L, "EDOOFUS: why the hell is this being called in the dedi version?");
#else
	lua_pushinteger(L, screen->w);
	lua_pushinteger(L, screen->h);
#endif
	
	return 2;
}

int icelua_fn_client_camera_shading_set(lua_State *L)
{
    /* sides are: east, bottom, south, west, top, north */
	int top = icelua_assert_stack(L, 6, 6);

#ifdef DEDI
	return luaL_error(L, "lm: why the hell is this being called in the dedi version?");
#else
	int i;
	for(i = 0; i < 6; i++)
	{
		float v = lua_tonumber(L, i+1);
		if(v < 0.0f) v = 0.0f;
		if(v > 1.0f) v = 1.0f;
		uint32_t s = v*255;
		if(s >= 128) s++;
		cam_shading[i] = s;
	}
#endif

	return 0;
}

