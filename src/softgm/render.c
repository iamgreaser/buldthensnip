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

#include "common.h"

#if 0
#define DEBUG_INVERT_DRAW_DIR
#endif
#if 0
#define DEBUG_SHOW_TOP_BOTTOM
#define DEBUG_HIDE_MAIN
#endif

#define CUBESUX_MARKER 20
#define RAYC_MAX ((int)((FOG_MAX_DISTANCE+1)*(FOG_MAX_DISTANCE+1)*8+10))

#define DF_NX 0x01
#define DF_NY 0x02
#define DF_NZ 0x04
#define DF_PX 0x08
#define DF_PY 0x10
#define DF_PZ 0x20
#define DF_SPREAD 0x3F

enum
{
	CM_NX = 0,
	CM_NY,
	CM_NZ,
	CM_PX,
	CM_PY,
	CM_PZ,
	CM_MAX
};

int cam_shading_map[6][4] = {
	{CM_PZ, CM_NZ, CM_PY, CM_NY},
	{CM_NX, CM_PX, CM_NZ, CM_PZ},
	{CM_NX, CM_PX, CM_PY, CM_NY},
	{CM_NZ, CM_PZ, CM_PY, CM_NY},
	{CM_PX, CM_NX, CM_PZ, CM_NZ},
	{CM_PX, CM_NX, CM_PY, CM_NY},
};

uint32_t *cubemap_color[CM_MAX];
float *cubemap_depth[CM_MAX];
int cubemap_size;
int cubemap_shift;

float fog_distance = FOG_INIT_DISTANCE;
uint32_t fog_color = 0xD0E0FF;

uint32_t *rtmp_pixels;
int rtmp_width, rtmp_height, rtmp_pitch;
camera_t *rtmp_camera;
map_t *rtmp_map;

uint32_t cam_shading[6] = {
	 0x000C0, 0x000A0, 0x000D0, 0x000E0, 0x00FF, 0x000D0,
};

typedef struct raydata {
	int16_t x,y,z;
	int8_t gx,gz;
	
	float y1,y2;
	float sx,sy,sz;
} raydata_t;

typedef struct rayblock {
	uint32_t color;
	float x,y,z;
} rayblock_t;

typedef struct edgebit {
	int x1,x2;
	float z1,u1,v1;
	float z2,u2,v2;
} edgebit_t;

int elist_y1;
int elist_y2;
edgebit_t *elist = NULL;
int elist_len = 0;

int rayc_block_len, rayc_block_head;
int rayc_data_len, rayc_data_head;
raydata_t rayc_data[RAYC_MAX];
rayblock_t *rayc_block = NULL;
#ifdef RENDER_CUBES_MULTITHREADED
int *rayc_stack_len = NULL;
int *rayc_stack_ordlen = NULL;
#endif
int *rayc_mark = NULL;
int rayc_block_size = 0;
int rayc_mark_size = 0;

float *dbuf;

#ifdef RENDER_FACE_COUNT
	int render_face_current = 0;
	int render_face_remain = 0;
#endif


/*
 * REFERENCE IMPLEMENTATION
 * 
 */

uint32_t render_fog_apply_new(uint32_t color, float depth)
{
	int b = color&255;
	int g = (color>>8)&255;
	int r = (color>>16)&255;
	int t = (color>>24)&255;
	
	//float fog = (fog_distance*fog_distance/depth)/256.0f;
	float fog = (fog_distance*fog_distance-(depth < 0.001f ? 0.001f : depth))
		/(fog_distance*fog_distance);
	if(fog > 1.0f)
		fog = 1.0f;
	if(fog < 0.0f)
		fog = 0.0f;
	
	r = (r*fog+((fog_color>>16)&0xFF)*(1.0-fog)+0.5f);
	g = (g*fog+((fog_color>>8)&0xFF)*(1.0-fog)+0.5f);
	b = (b*fog+((fog_color)&0xFF)*(1.0-fog)+0.5f);
	
	int fcol = b|(g<<8)|(r<<16);
	return fcol|(t<<24);
}

uint32_t render_fog_apply(uint32_t color, float depth)
{
	int b = color&255;
	int g = (color>>8)&255;
	int r = (color>>16)&255;
	int t = (color>>24)&255;
	
	float fog = (fog_distance-(depth < 0.001f ? 0.001f : depth))/fog_distance;
	if(fog > 1.0f)
		fog = 1.0f;
	if(fog < 0.0f)
		fog = 0.0f;
	
	r = (r*fog+((fog_color>>16)&0xFF)*(1.0-fog)+0.5f);
	g = (g*fog+((fog_color>>8)&0xFF)*(1.0-fog)+0.5f);
	b = (b*fog+((fog_color)&0xFF)*(1.0-fog)+0.5f);
	
	int fcol = b|(g<<8)|(r<<16);
	return fcol|(t<<24);
}

void render_rect_clip(uint32_t *color, int *x1, int *y1, int *x2, int *y2, float depth)
{
	*color = render_fog_apply(*color, depth);
	
	// arrange *1 <= *2
	if(*x1 > *x2)
	{
		int t = *x1;
		*x1 = *x2;
		*x2 = t;
	}
	
	if(*y1 > *y2)
	{
		int t = *y1;
		*y1 = *y2;
		*y2 = t;
	}
	
	// clip
	if(*x1 < 0)
		*x1 = 0;
	if(*y1 < 0)
		*y1 = 0;
	if(*x2 > cubemap_size)
		*x2 = cubemap_size;
	if(*y2 > cubemap_size)
		*y2 = cubemap_size;
}

void render_rect_clip_screen(uint32_t *color, int *x1, int *y1, int *x2, int *y2, float depth)
{
	*color = render_fog_apply(*color, depth);
	
	// arrange *1 <= *2
	if(*x1 > *x2)
	{
		int t = *x1;
		*x1 = *x2;
		*x2 = t;
	}
	
	if(*y1 > *y2)
	{
		int t = *y1;
		*y1 = *y2;
		*y2 = t;
	}
	
	// clip
	if(*x1 < 0)
		*x1 = 0;
	if(*y1 < 0)
		*y1 = 0;
	if(*x2 > rtmp_width)
		*x2 = rtmp_width;
	if(*y2 > rtmp_height)
		*y2 = rtmp_height;
}

void render_rect_zbuf(uint32_t *ccolor, float *cdepth, int x1, int y1, int x2, int y2, uint32_t color, float depth)
{
	int x,y;
	
	// clip
	render_rect_clip_screen(&color, &x1, &y1, &x2, &y2, depth);
	//uint32_t dummy;
	//render_rect_clip_screen(&dummy, &x1, &y1, &x2, &y2, depth);
	
	if(x2 <= 0)
		return;
	if(x1 >= rtmp_width)
		return;
	if(y2 <= 0)
		return;
	if(y1 >= rtmp_height)
		return;
	if(x1 == x2)
		return;
	if(y1 == y2)
		return;
	
	// render
	uint32_t *cptr = &ccolor[y1*rtmp_pitch+x1];
	float *dptr = &cdepth[y1*rtmp_width+x1];
	int stride = x2-x1;
	int pitch = rtmp_pitch - stride;
	int dpitch = rtmp_width - stride;
	
#ifdef __SSE__
	if(x2-x1 >= 16)
	{
		int fpitch = cubemap_size - (((x2-x1)+7)&~7);
		uint32_t *cfptr = cptr;
		float *dfptr = dptr;
		int xs;
		
		for(x = x1; x < x2; x += 8)
		{
			_mm_prefetch(cfptr, _MM_HINT_NTA);
			cfptr += 8;
			_mm_prefetch(dfptr, _MM_HINT_NTA);
			dfptr += 8;
		}
		
		cfptr += fpitch;
		dfptr += fpitch;
		
		for(y = y1; y < y2-1; y++)
		{
			for(x = x1; x < x2-8; x += 8)
			{
				_mm_prefetch(cfptr, _MM_HINT_NTA);
				for(xs = 0; xs < 8; xs++)
				{
					if(*dptr > depth)
					{
						*dptr = depth;
						*cptr = color;
					}
					cptr++; dptr++;
				}
				_mm_prefetch(dfptr, _MM_HINT_NTA);
				cfptr += 8;
				dfptr += 8;
			}
			_mm_prefetch(cfptr, _MM_HINT_NTA);
			cfptr += 8;
			
			for(x = x; x < x2; x++)
			{
				if(*dptr > depth)
				{
					*dptr = depth;
					*cptr = color;
				}
				cptr++; dptr++;
			}
			
			_mm_prefetch(dfptr, _MM_HINT_NTA);
			dfptr += 8;
			
			cfptr += fpitch;
			dfptr += fpitch;
			
			cptr += pitch;
			dptr += pitch;
		}
		
		{
			for(x = x1; x < x2; x++)
			{
				if(*dptr > depth)
				{
					*dptr = depth;
					*cptr = color;
				}
				cptr++; dptr++;
			}
			
			dptr += dpitch;
			cptr += pitch;
		}
	} else {
		for(y = y1; y < y2; y++)
		{
			for(x = x1; x < x2; x++)
			{
				if(*dptr > depth)
				{
					*dptr = depth;
					*cptr = color;
				}
				cptr++; dptr++;
			}
			
			dptr += dpitch;
			cptr += pitch;
		}
	}
#else
	for(y = y1; y < y2; y++)
	{
		for(x = x1; x < x2; x++)
		{
			if(*dptr > depth)
			{
				*dptr = depth;
				*cptr = color;
			}
			cptr++; dptr++;
		}
		
		dptr += dpitch;
		cptr += pitch;
	}
#endif
}

// TODO: fast ver?
void render_vxl_rect_ftb_fast(uint32_t *ccolor, float *cdepth, int x1, int y1, int x2, int y2, uint32_t color, float depth)
//void render_vxl_rect_ftb_slow(uint32_t *ccolor, float *cdepth, int x1, int y1, int x2, int y2, uint32_t color, float depth)
{
	int x,y;
	
	// TODO: stop using this bloody function
	// (alternatively, switch to the fast FTB as used in Doom and Quake)
	//
	// NOTE: this approach seems to be faster than render_vxl_rect_btf.
	
	// clip
	uint32_t dummy;
	render_rect_clip(&dummy, &x1, &y1, &x2, &y2, depth);
	
	if(x2 <= 0)
		return;
	if(x1 >= cubemap_size)
		return;
	if(y2 <= 0)
		return;
	if(y1 >= cubemap_size)
		return;
	if(x1 >= x2)
		return;
	if(y1 >= y2)
		return;
	
	// render
	uint32_t *cptr = &ccolor[(y1<<cubemap_shift)+x1];
	float *dptr = &cdepth[(y1<<cubemap_shift)+x1];
	int pitch = cubemap_size - (x2-x1);
	
#ifdef __SSE__
	// Because SSE was invented just so we could get the prefetch instructions.
	// Anyhow, this makes mesa.vxl go from 41FPS to 42FPS. Totally Worth It
	// (although with some tweaking I think it could go a bit further than this crap)
	if(x2-x1 >= 16)
	{
		int fpitch = cubemap_size - (((x2-x1)+7)&~7);
		uint32_t *cfptr = cptr;
		float *dfptr = dptr;
		int xs;
		for(x = x1; x < x2; x += 8)
		{
			_mm_prefetch(cfptr, _MM_HINT_NTA);
			cfptr += 8;
			_mm_prefetch(dfptr, _MM_HINT_NTA);
			dfptr += 8;
		}
		
		cfptr += fpitch;
		dfptr += fpitch;
		
		for(y = y1; y < y2-1; y++)
		{
			for(x = x1; x < x2-8; x += 8)
			{
				_mm_prefetch(cfptr, _MM_HINT_NTA);
				for(xs = 0; xs < 8; xs++)
				{
					if(*cptr == fog_color)
					{
						*cptr = color;
						*dptr = depth;
					}
					cptr++;
					dptr++;
				}
				_mm_prefetch(dfptr, _MM_HINT_NTA);
				cfptr += 8;
				dfptr += 8;
			}
			_mm_prefetch(cfptr, _MM_HINT_NTA);
			cfptr += 8;
			
			for(x = x; x < x2; x++)
			{
				if(*cptr == fog_color)
				{
					*cptr = color;
					*dptr = depth;
				}
				cptr++;
				dptr++;
			}
			
			_mm_prefetch(dfptr, _MM_HINT_NTA);
			dfptr += 8;
			
			cfptr += fpitch;
			dfptr += fpitch;
			
			cptr += pitch;
			dptr += pitch;
		}
		
		{
			for(x = x1; x < x2; x++)
			{
				if(*cptr == fog_color)
				{
					*cptr = color;
					*dptr = depth;
				}
				cptr++;
				dptr++;
			}
		}
	} else {
		for(y = y1; y < y2; y++)
		{
			for(x = x1; x < x2; x++)
			{
				if(*cptr == fog_color)
				{
					*cptr = color;
					*dptr = depth;
				}
				cptr++;
				dptr++;
			}
			
			cptr += pitch;
			dptr += pitch;
		}
	}
#else
	for(y = y1; y < y2; y++)
	{
		for(x = x1; x < x2; x++)
		{
			if(*cptr == fog_color)
			{
				*cptr = color;
				*dptr = depth;
			}
			cptr++;
			dptr++;
		}
		
		cptr += pitch;
		dptr += pitch;
	}
#endif
}

void render_vxl_cube_htrap(uint32_t *ccolor, float *cdepth,
	int x1a, int x1b, int y1, int x2a, int x2b, int y2, uint32_t color, float depth)
{
	// dropout
	if(x1b <= 0 && x2b <= 0)
		return;
	if(x1a >= cubemap_size && x2a >= cubemap_size)
		return;
	if(y2 <= 0)
		return;
	if(y1 >= cubemap_size)
		return;
	if(x1a >= x1b || x2a >= x2b)
		return;
	if(y1 >= y2)
		return;
	
	// calc gradients
	int m_x1a = (((x2a-x1a)<<16)+0x8000)/(y2-y1);
	int m_x1b = (((x2b-x1b)<<16)+0x8000)/(y2-y1);
	int sub_x1a = 0;
	int sub_x1b = 0;
	
	// Y clamp
	// TODO: clamp y1 properly
	if(y2 >= cubemap_size)
		y2 = cubemap_size;
	
	// render
	//uint32_t *cptr = &ccolor[(y1<<cubemap_shift)+x1a];
	//float *dptr = &cdepth[(y1<<cubemap_shift)+x1a];
	uint32_t *cptr = ccolor;
	float *dptr = cdepth;
	int x,y;
	for(y = y1; y < y2; y++)
	{
		if(y >= 0)
		{
			int rx1a = (x1a < 0 ? 0 : x1a);
			int rx1b = (x1b >= cubemap_size ? cubemap_size : x1b);
			
			cptr = &ccolor[(y<<cubemap_shift)+rx1a];
			dptr = &cdepth[(y<<cubemap_shift)+rx1a];
			
			for(x = rx1a; x < rx1b; x++)
			{
				if(*cptr == fog_color)
				{
					*cptr = color;
					*dptr = depth;
				}
				cptr++;
				dptr++;
			}
		}
		
		sub_x1a += m_x1a;
		sub_x1b += m_x1b;
		x1a += (sub_x1a>>16);
		x1b += (sub_x1b>>16);
		sub_x1a &= 0xFFFF;
		sub_x1b &= 0xFFFF;
	}
}

void render_vxl_cube_vtrap(uint32_t *ccolor, float *cdepth,
	int x1, int y1a, int y1b, int x2, int y2a, int y2b, uint32_t color, float depth)
{
	// TODO: make this not so bloody horrible for the cache
	
	// dropout
	if(y1b <= 0 && y2b <= 0)
		return;
	if(y1a >= cubemap_size && y2a >= cubemap_size)
		return;
	if(x2 <= 0)
		return;
	if(x1 >= cubemap_size)
		return;
	if(y1a >= y1b || y2a >= y2b)
		return;
	if(x1 >= x2)
		return;
	
	// calc gradients
	int m_y1a = ((y2a-y1a)<<16);
	int m_y1b = ((y2b-y1b)<<16);
	m_y1a /= (x2-x1);
	m_y1b /= (x2-x1);
	int sub_y1a = 0;
	int sub_y1b = 0;
	
	// X clamp
	// TODO: clamp x1 properly
	// TODO: fix the leaks properly
	x2++; y1b++; y2b++;
	if(x2 >= cubemap_size)
		x2 = cubemap_size;
	
	// render
	//uint32_t *cptr = &ccolor[(y1<<cubemap_shift)+x1a];
	//float *dptr = &cdepth[(y1<<cubemap_shift)+x1a];
	uint32_t *cptr = ccolor;
	float *dptr = cdepth;
	int pitch = cubemap_size;
	int x,y;
	for(x = x1; x < x2; x++)
	{
		if(x >= 0)
		{
			int ry1a = (y1a < 0 ? 0 : y1a);
			int ry1b = (y1b >= cubemap_size ? cubemap_size : y1b);
			
			cptr = &ccolor[(ry1a<<cubemap_shift)+x];
			dptr = &cdepth[(ry1a<<cubemap_shift)+x];
			
			for(y = ry1a; y < ry1b; y++)
			{
				if(*cptr == fog_color)
				{
					*cptr = color;
					*dptr = depth;
				}
				cptr += pitch;
				dptr += pitch;
			}
		}
		
		sub_y1a += m_y1a;
		sub_y1b += m_y1b;
		y1a += (sub_y1a>>16);
		y1b += (sub_y1b>>16);
		sub_y1a &= 0xFFFF;
		sub_y1b &= 0xFFFF;
	}
}

uint32_t render_shade(uint32_t color, int face)
{
	uint32_t fc = cam_shading[face];
	return (((((color&0x00FF00FF)*fc)>>8)&0x00FF00FF))
		|((((((color>>8)&0x00FF00FF)*fc))&0xFF00FF00))|0x01000000;
}

void render_vxl_cube_sides(uint32_t *ccolor, float *cdepth, int x1, int y1, int x2, int y2, uint32_t color, float depth, int face, float fdist)
{		
	
	int hsize = (cubemap_size>>1);

#if 0	
	if(depth > CUBESUX_MARKER)
	{
		int x3 = ((x1-hsize)*depth)/(depth+1.0f)+hsize;
		int y3 = ((y1-hsize)*depth)/(depth+1.0f)+hsize;
		int x4 = ((x2-hsize+1)*depth)/(depth+1.0f)+hsize;
		int y4 = ((y2-hsize+1)*depth)/(depth+1.0f)+hsize;
		if(x1 > x3) x1 = x3;
		if(y1 > y3) y1 = y3;
		if(x2 < x4) x2 = x4;
		if(y2 < y4) y2 = y4;
		
		render_vxl_rect_ftb_fast(ccolor, cdepth, x1, y1, x2, y2, render_shade(color, face), depth+0.5f);
		return;
	}
#endif
	
	int x3 = ((x1-hsize)*depth)/(depth+1.0f)+hsize;
	int y3 = ((y1-hsize)*depth)/(depth+1.0f)+hsize;
	int x4 = ((x2-hsize)*depth)/(depth+1.0f)+hsize;
	int y4 = ((y2-hsize)*depth)/(depth+1.0f)+hsize+1;
	
	render_vxl_rect_ftb_fast(ccolor, cdepth, x1, y1, x2, y2, render_fog_apply_new(render_shade(color, face), fdist), depth);
	
	depth += 0.5f;
	
	if(y3 < y1)
		render_vxl_cube_htrap(ccolor, cdepth,
			x3, x4, y3, x1, x2, y1,
			render_fog_apply_new(render_shade(color, cam_shading_map[face][2]), fdist), depth+1.0f);
	else if(y2 < y4)
		render_vxl_cube_htrap(ccolor, cdepth,
			x1, x2, y2, x3, x4, y4,
			render_fog_apply_new(render_shade(color, cam_shading_map[face][3]), fdist), depth+1.0f);
	
	if(x3 < x1)
		render_vxl_cube_vtrap(ccolor, cdepth,
			x3, y3, y4, x1, y1, y2,
			render_fog_apply_new(render_shade(color, cam_shading_map[face][0]), fdist), depth+1.0f);
	else if(x2 < x4)
		render_vxl_cube_vtrap(ccolor, cdepth,
			x2, y1, y2, x4, y3, y4,
			render_fog_apply_new(render_shade(color, cam_shading_map[face][1]), fdist), depth+1.0f);
}

void render_vxl_cube(uint32_t *ccolor, float *cdepth, int x1, int y1, int x2, int y2, uint32_t color, float depth, int face, float fdist)
{
	render_vxl_cube_sides(ccolor, cdepth, x1, y1, x2, y2, color, depth, face, fdist);
}

void render_vxl_face_raycast(int blkx, int blky, int blkz,
	float subx, float suby, float subz,
	int face,
	int gx, int gy, int gz)
{
	int i;
	
	float tracemul = cubemap_size/2;
	float traceadd = tracemul;
	
	// get cubemaps
	uint32_t *ccolor = cubemap_color[face];
	float *cdepth = cubemap_depth[face];
	
	// clear cubemap
	for(i = 0; i < cubemap_size*cubemap_size; i++)
	{
		ccolor[i] = fog_color;
		cdepth[i] = fog_distance;
	}
	
	// get X cube direction
	int xgx = gz+gy;
	int xgy = 0;
	int xgz = -gx;
	
	// get Y cube direction
	int ygx = 0;
	int ygy = fabsf(gx+gz);
	int ygz = gy;
	
	// get base pos
	float bx = blkx+subx;
	float by = blky+suby;
	float bz = blkz+subz;
	
	if(xgx+xgy+xgz < 0)
	{
		bx += xgx;
		by += xgy;
		bz += xgz;
	}
	
	if(ygx+ygy+ygz < 0)
	{
		bx += ygx;
		by += ygy;
		bz += ygz;
	}
	
	if(gx+gy+gz < 0)
	{
		bx += gx;
		by += gy;
		bz += gz;
	}
	
	// now crawl through the block list
#ifdef DEBUG_INVERT_DRAW_DIR
	{
		{
			int bctr;
			for(bctr = rayc_block_len-1; bctr >= 0; bctr--)
#else
#ifndef RENDER_CUBES_MULTITHREADED 
	{
		{
			int bctr;
			for(bctr = 0; bctr <= rayc_block_len; bctr++)
#else
	int ord_accum_start = 0;
	int ord_accum_end = 0;
	int ord_idx;
	//printf("start\n");
	for(ord_idx = 0; rayc_stack_ordlen[ord_idx] != -1; ord_idx++)
	{
		ord_accum_start = ord_accum_end;
		ord_accum_end += rayc_stack_ordlen[ord_idx];
		//printf("%i\n", ord_accum_end);
		int pil_idx;
		#pragma omp parallel for
		for(pil_idx = ord_accum_start; pil_idx < ord_accum_end; pil_idx++)
		{
			int bctr;
			int bc_start = rayc_stack_len[pil_idx];
			int bc_end = rayc_stack_len[pil_idx+1];
			for(bctr = bc_start; bctr < bc_end && bctr < rayc_block_len; bctr++)
#endif
#endif
			{
				rayblock_t *b = &rayc_block[bctr];
				// get block delta
				float dx = b->x - bx;
				float dy = b->y - by;
				float dz = b->z - bz;
				
				// get correct screen positions
				float sx = dx*xgx+dy*xgy+dz*xgz;
				float sy = dx*ygx+dy*ygy+dz*ygz;
				float sz = dx* gx+dy* gy+dz* gz;
				
				// check distance
				if(sz < 0.001f || sz >= fog_distance)
					continue;
				
				// frustum cull
				if(fabsf(sx) > fabsf(sz+2.0f) || fabsf(sy) > fabsf(sz+2.0f))
					continue;
				
				// draw
				float boxsize = tracemul/fabsf(sz);
				float px1 = sx*boxsize+traceadd;
				float py1 = sy*boxsize+traceadd;
				float px2 = px1+boxsize;
				float py2 = py1+boxsize;
				
				render_vxl_cube(ccolor, cdepth,
					(int)px1, (int)py1, (int)px2, (int)py2,
					b->color, sz, face, sx*sx+sy*sy+sz*sz);
			}
		}
	}
}

void render_vxl_redraw(camera_t *camera, map_t *map)
{
	// if there isn't a map, clear screen and return
	if(map == NULL)
	{
		int face,i;
		
		for(face = 0; face < 6; face++)
		{
			// get cubemaps
			uint32_t *ccolor = cubemap_color[face];
			float *cdepth = cubemap_depth[face];
			
			// clear cubemap
			for(i = 0; i < cubemap_size*cubemap_size; i++)
			{
				ccolor[i] = fog_color;
				cdepth[i] = fog_distance;
			}
		}
		
		return;
	}
	
	int i;
	
	// stash stuff in globals to prevent spamming the stack too much
	// (and in turn thrashing the cache)
	rtmp_camera = camera;
	rtmp_map = map;
	
	// stash x/y/zlen
	int xlen = map->xlen;
	int ylen = map->ylen;
	int zlen = map->zlen;
	
	// get block pos
	int blkx = ((int)floor(camera->mpx)) & (xlen-1);
	int blky = ((int)floor(camera->mpy));// & (ylen-1);
	int blkz = ((int)floor(camera->mpz)) & (zlen-1);
	
	// get block subpos
	float subx = (camera->mpx - floor(camera->mpx));
	float suby = (camera->mpy - floor(camera->mpy));
	float subz = (camera->mpz - floor(camera->mpz));
	
	// get centre (base) pos
	float bx = blkx + subx;
	float by = blky + suby;
	float bz = blkz + subz;
	
	int byi = blky;
	
	// check if we need to reallocate the mark table and block list
	{
		int markbase = xlen * zlen;
		int blockbase = markbase * ylen;
		
		if(rayc_mark_size != markbase)
		{
			rayc_mark_size = markbase;
			rayc_mark = (int*)realloc(rayc_mark, rayc_mark_size*sizeof(int));
#ifdef RENDER_CUBES_MULTITHREADED
			rayc_stack_len = realloc(rayc_stack_len, rayc_mark_size*8*sizeof(int));
			rayc_stack_ordlen = realloc(rayc_stack_ordlen, rayc_mark_size*8*sizeof(int));
#endif
		}
		
		if(rayc_block_size != blockbase)
		{
			rayc_block_size = blockbase;
			rayc_block = (rayblock_t*)realloc(rayc_block, rayc_block_size*sizeof(rayblock_t));
		}
	}
	
	// clear the mark table
	memset(rayc_mark, 0, rayc_mark_size*sizeof(int));
	
	// prep the starting block
	rayc_block_len = 0;
	rayc_block_head = 0;
	rayc_data_len = 1;
	rayc_data_head = 0;
	
	rayc_data[0].x = blkx;
	rayc_data[0].y = blky;
	rayc_data[0].z = blkz;
	rayc_data[0].gx = 0;
	rayc_data[0].gz = 0;
	rayc_data[0].y1 = blky+suby;
	rayc_data[0].y2 = blky+suby;
	rayc_data[0].sx = subx;
	rayc_data[0].sy = suby;
	rayc_data[0].sz = subz;
	rayc_mark[blkx + blkz*xlen] = 1;
	
#ifdef RENDER_CUBES_MULTITHREADED
	// get the block order stack set up for multiprocessing stuff
	int stack_ordidx = 0;
	int stack_pilidx = 0;
	int stack_ordrem = 1;
	int stack_ordptr = 0;
	int stack_pilptr = 0;
#endif
	
	// build your way up
	while(rayc_data_head < rayc_data_len)
	{	
		raydata_t *rd = &(rayc_data[rayc_data_head++]);
		
		// back this up so we can flip the top
		rayblock_t *b_pstart = &(rayc_block[rayc_block_len]);
		rayblock_t *b_pmid = b_pstart;
		
		// get delta
		float dx = rd->x - bx;
		float dz = rd->z - bz;
		if(rd->gx < 0) dx++;
		else if(rd->gx == 0) dx = 0;
		if(rd->gz < 0) dz++;
		else if(rd->gz == 0) dz = 0;
		
		// skip this if it's in the fog
		if(dx*dx+dz*dz >= fog_distance*fog_distance)
			continue;
		
		int near_cast = (rayc_data_head == 1);
		
		// find where we are
		int idx = (((int)(rd->z)) & (zlen-1))*xlen + (((int)rd->x) & (xlen-1));
		uint8_t *p = map->pillars[idx]+4;
		rayc_mark[idx] = -1;
		int lastn = 0;
		int topcount = 0;
		int lasttop = 0;
		
		float ysearch = rd->y1;
		while(p[0] != 0)
		{
			if(ysearch < p[2] && (lastn == 0 || ysearch >= lasttop))
				break;
			
			lastn = p[0];
			lasttop = p[1];
			topcount = p[0] - (p[2]-p[1]+1);
			p += p[0]*4;
		}
		
		int spreadflag = 1;
		
		// advance y1/y2
		float y1 = rd->y1;
		float y2 = rd->y2;
		
		if(near_cast)
		{
			y1 = (lastn == 0 ? 0.0f : p[3]);
			if(y1 > rd->y1)
				y1 = rd->y1;
			rd->y1 = y1;
			rd->y2 = y2 = p[1];
		} else {
			float dist1 = sqrtf(dx*dx+dz*dz);
			float dist2 = dist1 + 1.0f; // approx max dist this can travel
			float travel = dist2/dist1;
			if(y1 < by)
				y1 = by + (y1-by)*travel;
			if(y2 > by)
				y2 = by + (y2-by)*travel;
		}
		
		int iy1 = floor(y1);
		int iy2 = floor(y2);
		float by1 = y1;
		float by2 = y2;
		
		// TODO: get the order right!
		
#ifdef DEBUG_SHOW_TOP_BOTTOM
		{
			rayblock_t *b = &rayc_block[rayc_block_len++];
			b->x = rd->x;
			b->z = rd->z;
			b->y = iy1;
			b->color = 0xFFFF0000;
		}
		{
			rayblock_t *b = &rayc_block[rayc_block_len++];
			b->x = rd->x;
			b->z = rd->z;
			b->y = iy2;
			b->color = 0xFF0000FF;
		}
		b_pstart += 2;
		b_pmid += 2;
#endif
		// add the top blocks (if they exist and we can see them)
		if(lastn == 0)
		{
			if(y1 > 0.0f) y1 = 0;
			y2 = p[1];
		} else if(rayc_data_head == 1) {
			y1 = p[3];
			y2 = p[1];
			// just the immediate ceiling, thanks.
#ifndef DEBUG_HIDE_MAIN
			{
				rayblock_t *b = &rayc_block[rayc_block_len++];
				b->x = rd->x;
				b->z = rd->z;
				b->y = p[3]-1;
				b->color = *(uint32_t *)(&p[-4]);
			}
#endif
		} else if(p[3] >= rd->y1-1) {
			y1 = p[3];
			y2 = p[1];
			uint32_t *c = (uint32_t *)(&p[-4*topcount]);
#ifndef DEBUG_HIDE_MAIN
			for(i = p[3]-topcount; i <= p[3]-1; i++)
			{
				if(i < iy1)
				{
					c++;
					continue;
				}
				
				rayblock_t *b = &rayc_block[rayc_block_len++];
				b->x = rd->x;
				b->z = rd->z;
				b->y = i;
				b->color = *(c++);
			}
#endif
		}
		
		// sneak your way down
		while(p[1] <= iy2)
		{
			if(p[1] != p[3])
				y2 = p[1];
			
			//printf("%i %i %i %i [%i, %i]\n", p[0],p[1],p[2],p[3],iy1,iy2);
			uint32_t *c = (uint32_t *)(&p[4]);
#ifndef DEBUG_HIDE_MAIN
			for(i = p[1]; i <= p[2] && i <= iy2; i++)
			{
				rayblock_t *b = &rayc_block[rayc_block_len++];
				b->x = rd->x;
				b->z = rd->z;
				b->y = i;
				b->color = *(c++);
			}
#endif
			if(p[0] == 0)
				break;
			
			lastn = p[0];
			lasttop = p[1];
			topcount = p[0] - (p[2]-p[1]+1);
			p += 4*p[0];
			
			if(p[1] != p[3] && rd->y2 >= p[3])
				y2 = p[1];
			
			c = (uint32_t *)(&p[-4*topcount]);
#ifndef DEBUG_HIDE_MAIN
			for(i = p[3]-topcount; i <= p[3]-1 && i <= iy2; i++)
			{
				rayblock_t *b = &rayc_block[rayc_block_len++];
				b->x = rd->x;
				b->z = rd->z;
				b->y = i;
				b->color = *(c++);
			}
#endif
		}
		
		// find the y middle
		while(b_pmid < &rayc_block[rayc_block_len] && b_pmid->y <= byi)
			b_pmid++;
		b_pmid--;
		
		// flip!
		while(b_pstart < b_pmid)
		{
			rayblock_t t;
			
			t = *b_pstart;
			*b_pstart = *b_pmid;
			*b_pmid = t;
			
			b_pstart++;
			b_pmid--;
		}
		
#ifdef RENDER_CUBES_MULTITHREADED
		// add blockdata to span length table
		rayc_stack_len[stack_pilidx++] = rayc_block_len - stack_pilptr;
		stack_pilptr = rayc_block_len;
#endif
		
		// correct the y spread
		if(y1 < by1)
			y1 = by1;
		if(y2 > by2)
			y2 = by2;
		
		spreadflag = spreadflag && (y1 < y2);
		//spreadflag = 1;
		
		// spread out
		int ofx = 1;
		int ofz = 0;
		if(spreadflag) do
		{
			int idx2 = ((ofx + (int)rd->x) & (xlen-1))
				+ xlen * ((ofz + (int)rd->z) & (zlen-1));
			
			if(ofx * rd->gx < 0 || ofz * rd->gz < 0)
			{
				// do nothing
			} else if(rayc_mark[idx2] == 0) {
				rayc_mark[idx2] = rayc_data_len+1;
				raydata_t *rd2 = &(rayc_data[rayc_data_len++]);
				
				rd2->x = ofx + (int)rd->x;
				rd2->z = ofz + (int)rd->z;
				rd2->y1 = y1;
				rd2->y2 = y2;
				rd2->sx = subx;
				rd2->sy = suby;
				rd2->sz = subz;
				rd2->gx = (ofx == 0 ? rd->gx : ofx);
				rd2->gz = (ofz == 0 ? rd->gz : ofz);
			} else if(rayc_mark[idx2] != -1) {
				raydata_t *rd2 = &(rayc_data[rayc_mark[idx2]-1]);
				
				if(y1 < rd2->y1)
					rd2->y1 = y1;
				if(y2 > rd2->y2)
					rd2->y2 = y2;
				if(rd2->gx == 0)
					rd2->gx = (ofx == 0 ? rd->gx : ofx);
				if(rd2->gz == 0)
					rd2->gz = (ofz == 0 ? rd->gz : ofz);
			}
			
			{
				int t = ofx;
				ofx = -ofz;
				ofz = t;
			}
		} while(ofx != 1);
		
#ifdef RENDER_CUBES_MULTITHREADED
		// add data to ordered span count table if necessary
		if(--stack_ordrem <= 0)
		{
			rayc_stack_ordlen[stack_ordidx++] = stack_ordrem = rayc_data_len - stack_ordptr;
			stack_ordptr = rayc_data_len;
		}
#endif
	}
	
#ifdef RENDER_CUBES_MULTITHREADED
	// terminate stack lists
	rayc_stack_len[stack_pilidx++] = 0;
	rayc_stack_len[stack_pilidx] = -1;
	rayc_stack_ordlen[stack_ordidx++] = rayc_data_len - stack_ordptr; 
	rayc_stack_ordlen[stack_ordidx] = -1;

	// apply running sum to stack lists
	{
		int rsum = 0;
		for(i = 0; rayc_stack_len[i] != -1; i++)
		{
			rsum += rayc_stack_len[i];
			rayc_stack_len[i] = rsum;
		}
	}	
#endif

	//printf("%i %i %i\n", stack_pilidx, stack_ordidx, stack_ordrem);
	
	// render each face
#ifdef RENDER_FACE_COUNT
	for(i = 0; i < RENDER_FACE_COUNT && render_face_remain > 0; i++)
	{
		switch(render_face_current)
		{
			default:
				render_face_current = 0;
				/* FALL THROUGH */
			case 0:
				render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NX, -1,  0,  0);
				break;
			case 1:
				render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NY,  0, -1,  0);
				break;
			case 2:
				render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NZ,  0,  0, -1);
				break;
			case 3:
				render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PX,  1,  0,  0);
				break;
			case 4:
				render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PY,  0,  1,  0);
				break;
			case 5:
				render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PZ,  0,  0,  1);
				break;
		}
		render_face_current++;
		render_face_remain--;
	}
#else
#if 1
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NX, -1,  0,  0);
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NY,  0, -1,  0);
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NZ,  0,  0, -1);
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PX,  1,  0,  0);
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PY,  0,  1,  0);
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PZ,  0,  0,  1);
#else
#pragma omp sections
{
#pragma omp section
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NX, -1,  0,  0);
#pragma omp section
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NY,  0, -1,  0);
#pragma omp section
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_NZ,  0,  0, -1);
#pragma omp section
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PX,  1,  0,  0);
#pragma omp section
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PY,  0,  1,  0);
#pragma omp section
	render_vxl_face_raycast(blkx, blky, blkz, subx, suby, subz, CM_PZ,  0,  0,  1);
}
#endif
#endif
}

void render_cubemap_edge(
	int face,
	int x1, int y1, float z1, float u1, float v1,
	int x2, int y2, float z2, float u2, float v2)
{
	int y;
	
	// if out of Y range, drop out early.
	if(y1 < 0 && y2 < 0)
		return;
	if(y1 >= rtmp_height && y2 >= rtmp_height)
		return;
	
	// if perfectly horizontal, drop out early.
	if(y1 == y2)
		return;
	
	// prep line drawer
	int dx = x2-x1;
	int dy = y2-y1;
	int xadd = 0;
	int xinc = 1;
	int dc = 0;
	
	// ensure dy is positive
	if(dy < 0)
	{
		dx = -dx;
		dy = -dy;
	}
	
	// ensure dx is positive
	if(dx < 0)
	{
		dx = -dx;
		xinc = -1;
	}
	
	// calculate correct xadd,dx,dy
	xadd = dx/dy;
	xadd *= xinc;
	dx %= dy;
	
	// we are going clockwise.
	if(y1 < y2)
	{
		// right side
		
		// clip for y
		if(y1 < 0)
		{
			y1 = -y1;
			
			dc = dx*y1;
			z1 += (z2-z1)*y1;
			u1 += (u2-u1)*y1;
			v1 += (v2-v1)*y1;
			
			x1 += xinc*(dc/dy) + xadd*y1;
			dc %= dy;
			
			y1 = 0;
		}
		
		if(y2 > rtmp_height)
			y2 = rtmp_height;
		
		// expand list top/bottom
		if(y1 < elist_y1) elist_y1 = y1;
		if(y2 > elist_y2) elist_y2 = y2;
		
		// calc deltas
		float dz = z2-z1;
		float du = u2-u1;
		float dv = v2-v1;
		
		// apply
		edgebit_t *eb = &elist[y1];
		for(y = y1; y < y2; y++)
		{
			eb->x2 = x1;
			eb->z2 = z1;
			eb->u2 = u1;
			eb->v2 = v1;
			
			z1 += dz;
			u1 += du;
			v1 += dv;
			
			x1 += xadd;
			dc += dx;
			if(dc >= dy)
			{
				x1 += xinc;
				dc -= dy;
			}
			
			eb++;
		}
	} else {
		// left side
		
		// clip for y
		if(y2 < 0)
		{
			y2 = -y2;
			
			dc = dx*y2;
			z2 += (z1-z2)*y2;
			u2 += (u1-u2)*y2;
			v2 += (v1-v2)*y2;
			
			x2 += xinc*(dc/dy) + xadd*y1;
			dc %= dy;
			
			y2 = 0;
		}
		
		if(y1 > rtmp_height)
			y1 = rtmp_height;
		
		// expand list top/bottom
		if(y2 < elist_y1) elist_y1 = y2;
		if(y1 > elist_y2) elist_y2 = y1;
		
		// calc deltas
		float dz = z1-z2;
		float du = u1-u2;
		float dv = v1-v2;
		
		// apply
		edgebit_t *eb = &elist[y1];
		for(y = y2; y < y1; y++)
		{
			eb->x1 = x2;
			eb->z1 = z2;
			eb->u1 = u2;
			eb->v1 = v2;
			
			z2 += dz;
			u2 += du;
			v2 += dv;
			
			x2 += xadd;
			dc += dx;
			if(dc >= dy)
			{
				x2 += xinc;
				dc -= dy;
			}
			
			eb++;
		}
	}
	
	// clamp y1,y2 to screen size
	// NOTE: shouldn't be necessary if the algo is correct
	if(elist_y1 < 0) elist_y1 = 0;
	if(elist_y2 > rtmp_height) elist_y2 = rtmp_height;
}

void render_cubemap_quad(
	int face,
	float x1, float y1, float z1,
	float x2, float y2, float z2,
	float x3, float y3, float z3,
	float x4, float y4, float z4)
{
	float u1,u2,u3,u4;
	float v1,v2,v3,v4;
	
	// precalc 1/z
	z1 = 1.0f/z1;
	z2 = 1.0f/z2;
	z3 = 1.0f/z3;
	z4 = 1.0f/z4;
	
	// prep u/v values
	u1 = -z1; v1 = -z1;
	u2 =  z1; v2 = -z1;
	u3 =  z1; v3 =  z1;
	u4 = -z1; v4 =  z1;
	
	// copy to some "unclipped" things
	float x1a,x1b,y1a,y1b,z1a,z1b,u1a,u1b,v1a,v1b;
	float x2a,x2b,y2a,y2b,z2a,z2b,u2a,u2b,v2a,v2b;
	float x3a,x3b,y3a,y3b,z3a,z3b,u3a,u3b,v3a,v3b;
	float x4a,x4b,y4a,y4b,z4a,z4b,u4a,u4b,v4a,v4b;
	
	x1a=x1b=x1; y1a=y1b=y1; z1a=z1b=z1; u1a=u1b=u1; v1a=v1b=v1;
	x2a=x2b=x2; y2a=y2b=y2; z2a=z2b=z2; u2a=u2b=u2; v2a=v2b=v2;
	x3a=x3b=x3; y3a=y3b=y3; z3a=z3b=z3; u3a=u3b=u3; v3a=v3b=v3;
	x4a=x4b=x4; y4a=y4b=y4; z4a=z4b=z4; u4a=u4b=u4; v4a=v4b=v4;
	
	// TODO: clip stuff
	
	// render edges
	if(x1a != x1b || y1a != y1b)
		render_cubemap_edge(face, x1a,y1a,z1a,u1a,v1a, x1b,y1b,z1b,u1b,v1b);
	render_cubemap_edge(face, x1b,y1b,z1b,u1b,v1b, x2a,y2a,z2a,u2a,v2a);
	
	if(x2a != x2b || y2a != y2b)
		render_cubemap_edge(face, x2a,y2a,z2a,u2a,v2a, x2b,y2b,z2b,u2b,v2b);
	render_cubemap_edge(face, x2b,y2b,z2b,u2b,v2b, x3a,y3a,z3a,u3a,v3a);
	
	if(x3a != x3b || y3a != y3b)
		render_cubemap_edge(face, x3a,y3a,z3a,u3a,v3a, x3b,y3b,z3b,u3b,v3b);
	render_cubemap_edge(face, x3b,y3b,z3b,u3b,v3b, x4a,y4a,z4a,u4a,v4a);
	
	if(x4a != x4b || y4a != y4b)
		render_cubemap_edge(face, x4a,y4a,z4a,u4a,v4a, x4b,y4b,z4b,u4b,v4b);
	render_cubemap_edge(face, x4b,y4b,z4b,u4b,v4b, x1a,y1a,z1a,u1a,v1a);
}

void render_cubemap_face(int face, int gx, int gy, int gz)
{
	int x,y;
	
	// reset edge list
	elist_y1 = rtmp_height;
	elist_y2 = 0;
	
	// calculate corners
	float cx1 = gx, cx2 = gx, cx3 = gx, cx4 = gx;
	float cy1 = gy, cy2 = gy, cy3 = gy, cy4 = gy;
	float cz1 = gz, cz2 = gz, cz3 = gz, cz4 = gz;
	
	// populate edge list
	render_cubemap_quad(face,
		cx1,cy1,cz1,
		cx2,cy2,cz2,
		cx3,cy3,cz3,
		cx4,cy4,cz4);
	
	// render edge list
	uint32_t *pb = rtmp_pixels + (rtmp_pitch*elist_y1);
	float *db = dbuf + (rtmp_width*elist_y1);
	
	for(y = elist_y1; y < elist_y2; y++)
	{
		edgebit_t *eb = &elist[y];
		
		// get start/end
		int x1 = eb->x1;
		int x2 = eb->x2;
		
		// get start z/u/v
		float zi = eb->z1;
		float ui = eb->u1;
		float vi = eb->v1;
		
		// get delta z/u/v
		float dzi = eb->z2-eb->z1;
		float dui = eb->u2-eb->u1;
		float dvi = eb->v2-eb->v1;
		
		uint32_t *p = &pb[x1];
		float *d = &db[x1];
		for(x = x1; x < x2; x++)
		{
			// invert z
			float z = 1/zi;
			
			// calculate u,v
			float u = ui*z;
			float v = vi*z;
			
			// TODO: fetch
			// TODO: plot
			//*(p++);
			//*(d++);
		}
		
		pb += rtmp_pitch;
		db += rtmp_width;
	}
	
}

// TODO: get this working
void render_cubemap_new(uint32_t *pixels, int width, int height, int pitch, camera_t *camera, map_t *map)
{
	// stash stuff in globals to prevent spamming the stack too much
	// (and in turn thrashing the cache)
	rtmp_pixels = pixels;
	rtmp_width = width;
	rtmp_height = height;
	rtmp_pitch = pitch;
	rtmp_camera = camera;
	rtmp_map = map;
	
	// prep edge list
	if(elist_len != height)
	{
		if(elist != NULL)
			free(elist);
		
		elist_len = height;
		elist = (edgebit_t*)malloc(sizeof(edgebit_t)*elist_len);
	}
	
	// do each face
	// TODO? backface cull?
	render_cubemap_face(CM_NX, -1,  0,  0);
	render_cubemap_face(CM_NY,  0, -1,  0);
	render_cubemap_face(CM_NZ,  0,  0, -1);
	render_cubemap_face(CM_PX,  1,  0,  0);
	render_cubemap_face(CM_PY,  0,  1,  0);
	render_cubemap_face(CM_PZ,  0,  0,  1);
}


void render_cubemap(uint32_t *pixels, int width, int height, int pitch, camera_t *camera, map_t *map)
{
	int x,y;
	
	// stash stuff in globals to prevent spamming the stack too much
	// (and in turn thrashing the cache)
	rtmp_pixels = pixels;
	rtmp_width = width;
	rtmp_height = height;
	rtmp_pitch = pitch;
	rtmp_camera = camera;
	rtmp_map = map;
	
	// get corner traces
	float tracemul = cubemap_size/2;
	float traceadd = tracemul;
	float ctrx1 = (camera->mzx+camera->mxx-camera->myx);
	float ctry1 = (camera->mzy+camera->mxy-camera->myy);
	float ctrz1 = (camera->mzz+camera->mxz-camera->myz);
	float ctrx2 = (camera->mzx-camera->mxx-camera->myx);
	float ctry2 = (camera->mzy-camera->mxy-camera->myy);
	float ctrz2 = (camera->mzz-camera->mxz-camera->myz);
	float ctrx3 = (camera->mzx+camera->mxx+camera->myx);
	float ctry3 = (camera->mzy+camera->mxy+camera->myy);
	float ctrz3 = (camera->mzz+camera->mxz+camera->myz);
	float ctrx4 = (camera->mzx-camera->mxx+camera->myx);
	float ctry4 = (camera->mzy-camera->mxy+camera->myy);
	float ctrz4 = (camera->mzz-camera->mxz+camera->myz);
	
	// calculate deltas
	float fbxq = ctrx1, fbyq = ctry1, fbzq = ctrz1; // base
	float fexq = ctrx2, feyq = ctry2, fezq = ctrz2; // end
	float flx = ctrx3-fbxq, fly = ctry3-fbyq, flz = ctrz3-fbzq; // left side
	float frx = ctrx4-fexq, fry = ctry4-feyq, frz = ctrz4-fezq; // right side
	flx /= (float)width; fly /= (float)width; flz /= (float)width;
	frx /= (float)width; fry /= (float)width; frz /= (float)width;
	
	// scale cubemap correctly
	fbxq += flx*((float)(width-height))/2.0f;
	fbyq += fly*((float)(width-height))/2.0f;
	fbzq += flz*((float)(width-height))/2.0f;
	fexq += frx*((float)(width-height))/2.0f;
	feyq += fry*((float)(width-height))/2.0f;
	fezq += frz*((float)(width-height))/2.0f;
	
	// raytrace it
	// TODO: find some faster method
	int hwidth = width/2;
	int hheight = height/2;
#if 0
	uint32_t *p = pixels;
	float *d = dbuf;
	for(y = -hheight; y < hheight; y++)
#else
#pragma omp parallel
{
	int x,y,z;
	float fex = fexq;
	float fey = feyq;
	float fez = fezq;
	float fbx = fbxq;
	float fby = fbyq;
	float fbz = fbzq;

	uint32_t *p = pixels;
	float *d = dbuf;
	int t_count = omp_get_num_threads();
	int t_idx = omp_get_thread_num();
	int y_start = (height*t_idx)/t_count-hheight; 
	int y_end = (height*(t_idx+1))/t_count-hheight; 
	p += (y_start+hheight)*pitch;
	d += (y_start+hheight)*width; 

	fbx += flx*(y_start+hheight);
	fby += fly*(y_start+hheight);
	fbz += flz*(y_start+hheight);
	
	fex += frx*(y_start+hheight);
	fey += fry*(y_start+hheight);
	fez += frz*(y_start+hheight);
	for(y = y_start; y < y_end; y++)
#endif
	{
		float fx = fbx;
		float fy = fby;
		float fz = fbz;
		
		float fdx = (fex-fbx)/(float)width;
		float fdy = (fey-fby)/(float)width;
		float fdz = (fez-fbz)/(float)width;
		
		for(x = -hwidth; x < hwidth; x++)
		{
			int pidx, pmap;
			// get correct cube map + pos
			float tx,ty,tz,atz;
			
			if(fabsf(fx) > fabsf(fy) && fabsf(fx) > fabsf(fz))
			{
				tx = -fz;
				ty = fy;
				tz = fx;
				atz = fabs(tz);
				pmap = fx >= 0.0f ? CM_PX : CM_NX;
			} else if(fabsf(fz) > fabsf(fy) && fabsf(fz) > fabsf(fx)) {
				tx = fx;
				ty = fy;
				tz = fz;
				atz = fabs(tz);
				pmap = fz >= 0.0f ? CM_PZ : CM_NZ;
			} else {
				tx = fx;
				ty = fz;
				tz = fy;
				atz = tz;
				pmap = fy >= 0.0f ? CM_PY : CM_NY;
			}
			
			pidx = ((cubemap_size-1)&(int)(tx*tracemul/tz+traceadd))
				|(((cubemap_size-1)&(int)(ty*tracemul/atz+traceadd))<<cubemap_shift);
			
			*(p++) = cubemap_color[pmap][pidx];
			*(d++) = cubemap_depth[pmap][pidx];//*sqrtf(fx*fx+fy*fy+fz*fz);
			
			fx += fdx;
			fy += fdy;
			fz += fdz;
		}
		
		p += pitch-width;
		
		fbx += flx;
		fby += fly;
		fbz += flz;
		
		fex += frx;
		fey += fry;
		fez += frz;
	}
#if 0
#else
}
#endif
	
	/*
	// TEST: draw something
	for(x = 0; x < 512; x++)
	for(y = 0; y < 512; y++)
	{
		pixels[y*pitch+x] = *(uint32_t *)&(map->pillars[y*map->xlen+x][8]);
		//pixels[y*pitch+x] = cubemap_color[CM_PZ][y*cubemap_size+x];
	}*/
}

void render_pmf_box(float x, float y, float z, float depth, float r, uint32_t color)
{
	// check Z straight away
	if(z < 0.001f)
		return;
	
	// get box
	int x1 = (( x-r)/z)*rtmp_width/2+rtmp_width/2;
	int y1 = (( y-r)/z)*rtmp_width/2+rtmp_height/2;
	int x2 = (( x+r)/z)*rtmp_width/2+rtmp_width/2;
	int y2 = (( y+r)/z)*rtmp_width/2+rtmp_height/2;
	
	// render
	render_rect_zbuf(rtmp_pixels, dbuf, x1, y1, x2, y2, color, depth);
}

void render_pmf_bone(uint32_t *pixels, int width, int height, int pitch, camera_t *cam_base,
	model_bone_t *bone, int islocal,
	float px, float py, float pz, float ry, float rx, float ry2, float scale)
{
	// stash stuff in globals to prevent spamming the stack too much
	// (and in turn thrashing the cache)
	rtmp_pixels = pixels;
	rtmp_width = width;
	rtmp_height = height;
	rtmp_pitch = pitch;
	rtmp_camera = cam_base;
	
	// get zoom factor
	float bzoom = (cam_base->mzx*cam_base->mzx
		+ cam_base->mzy*cam_base->mzy
		+ cam_base->mzz*cam_base->mzz);
	float unzoom = 1.0f/bzoom;
	float rezoom = sqrtf(bzoom);
	scale /= 256.0f;
	int i;
	for(i = 0; i < bone->ptlen; i++)
	{
		model_point_t *pt = &(bone->pts[i]);
		
		// get color
		uint32_t color = (pt->b)|(pt->g<<8)|(pt->r<<16)|(1<<24);
		
		// get position
		float x = pt->x;
		float y = pt->y;
		float z = pt->z;
		
		// rotate
		float sry = sin(ry);
		float cry = cos(ry);
		float srx = sin(rx);
		float crx = cos(rx);
		float sry2 = sin(ry2);
		float cry2 = cos(ry2);
		
		float tx = (x*cry+z*sry);
		float ty = y;
		float tz = (z*cry-x*sry);
		
		y = (ty*crx-tz*srx);
		tz = (tz*crx+ty*srx);
		
		x = (tx*cry2+tz*sry2);
		z = (tz*cry2-tx*sry2);
		
		// scalinate
		x *= scale;
		y *= scale;
		z *= scale;
		
		// offsettate
		x += px;
		y += py;
		z += pz;
		
		if(!islocal)
		{
			x -= cam_base->mpx;
			y -= cam_base->mpy;
			z -= cam_base->mpz;
		}
		
		// get correct centre depth
		float m = fabsf(x);
		if(m < fabsf(y))
			m = fabsf(y);
		if(m < fabsf(z))
			m = fabsf(z);
		//float dlen2 = x*x + y*y + z*z;
		//float dlen = sqrtf(dlen2);
		//float depth = sqrtf(2*m*m - dlen2);
		float depth = m;
		
		// cameranananinate
		if(!islocal)
		{
			float nx = x*cam_base->mxx+y*cam_base->mxy+z*cam_base->mxz;
			float ny = x*cam_base->myx+y*cam_base->myy+z*cam_base->myz;
			float nz = x*cam_base->mzx*unzoom+y*cam_base->mzy*unzoom+z*cam_base->mzz*unzoom;
			
			x = nx;
			y = ny;
			z = nz;
		}
		//depth *= z*rezoom;
		
		// plotinate
		render_pmf_box(-x, y, z, depth, pt->radius*scale, color);
	}
}

int render_init(int width, int height)
{
	int i;
	int size = (width > height ? width : height);
	
	// get nearest power of 2
	size = (size-1);
	size |= size>>1;
	size |= size>>2;
	size |= size>>4;
	size |= size>>8;
	size++;
	
	int msize = size;
	
	// reduce quality a little bit
	// 800x600 -> 1024^2 -> 512^2 ends up as 1MB x 6 textures = 6MB
	
	size >>= 1;
	
	// allocate cubemaps
	for(i = 0; i < CM_MAX; i++)
	{
		cubemap_color[i] = (uint32_t*)malloc(size*size*4);
		cubemap_depth[i] = (float*)malloc(size*size*4);
		if(cubemap_color[i] == NULL || cubemap_depth[i] == NULL)
		{
			// Can't allocate :. Can't continue
			// Clean up like a boss
			fprintf(stderr, "render_init: could not allocate cubemap %i\n", i);
			for(; i >= 0; i--)
			{
				if(cubemap_color[i] != NULL)
					free(cubemap_color[i]);
				if(cubemap_depth[i] != NULL)
					free(cubemap_depth[i]);
				cubemap_color[i] = NULL;
				cubemap_depth[i] = NULL;
			}
			
			return 1;
		}
	}
	
	// we might as well set this, too!
	cubemap_size = size;
	
	// calculate shift factor
	cubemap_shift = -1;
	while(size != 0)
	{
		cubemap_shift++;
		size >>= 1;
	}
	
	// allocate space for depth buffer
	dbuf = (float*)malloc(width*height*sizeof(float));
	// TODO: check if NULL
	
	return 0;
}

void render_deinit(void)
{
	int i;
	
	// deallocate cubemaps
	for(i = 0; i < CM_MAX; i++)
	{
		if(cubemap_color[i] != NULL)
		{
			free(cubemap_color[i]);
			cubemap_color[i] = NULL;
		}
		if(cubemap_depth[i] != NULL)
		{
			free(cubemap_depth[i]);
			cubemap_depth[i] = NULL;
		}
	}
	
	// deallocate edgelist
	if(elist != NULL)
	{
		free(elist);
		elist = NULL;
		elist_len = 0;
	}
	
	// deallocate depth buffer
	if(dbuf != NULL)
	{
		free(dbuf);
		dbuf = NULL;
	}
}
