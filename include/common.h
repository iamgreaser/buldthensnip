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

#define MODEL_BONE_MAX  256
#define MODEL_POINT_MAX 4096
#define PACKET_LEN_MAX 1280
#define PATH_LEN_MAX 128

// i wouldn't go near this limit if i were you...
#define CLIENT_MAX 512

#define WAV_MFREQ 44100

//define RENDER_FACE_COUNT 2

#include <immintrin.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include <sys/time.h>
#ifndef WIN32
#include <signal.h>
#endif

#include <math.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifndef DEDI
#include <SDL.h>
#endif

#include <zlib.h>

#ifdef WIN32

// just so we can get getaddrinfo
// you will need Windows 2000 at least!
#define _WIN32_WINNT 0x0501
#include <winsock2.h>
#include <ws2tcpip.h>
extern WSADATA windows_sucks;

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <fcntl.h>

#endif

enum
{
	UD_INVALID = 0,
	
	UD_JSON,
	UD_LOG,
	UD_LUA,
	UD_MAP_ICEMAP,
	UD_MAP_VXL,
	UD_MAP,
	UD_PMF,
	UD_IMG_TGA,
	UD_WAV,
	
	UD_MAX_SUPPORTED,
	
	UD_IMG,
	
	UD_MAX
};

// if this flag is set, free when finished sending
#define UDF_TEMPSEND 0x8000

typedef union vec4f
{
	struct { float x,y,z,w; } __attribute__((__packed__)) p;
	float a[4];
#ifdef __SSE__
	__m128 m;
#endif
} __attribute__((__packed__)) vec4f_t;

typedef struct matrix
{
	vec4f_t r[4];
} __attribute__((__packed__)) matrix_t;

typedef struct camera
{
	// camera bollocks
	float mxx,mxy,mxz,mxpad;
	float myx,myy,myz,mypad;
	float mzx,mzy,mzz,mzpad;
	float mpx,mpy,mpz,mppad;
} camera_t;

typedef struct model_point
{
	uint16_t radius;
	int16_t x,y,z;
	uint8_t b,g,r,resv1;
} __attribute__((__packed__)) model_point_t;

typedef struct model model_t;
typedef struct model_bone
{
	int udtype;
	char name[16];
	model_t *parent;
	int parent_idx;
	int ptlen, ptmax;
	model_point_t pts[];
} model_bone_t;

struct model
{
	int udtype;
	int bonelen, bonemax;
	model_bone_t *bones[];
};

// source: http://paulbourke.net/dataformats/tga/
typedef struct img_tgahead
{
	uint8_t idlen;
	uint8_t cmtype;
	uint8_t imgtype;
	uint16_t cmoffs;
	uint16_t cmlen;
	uint8_t cmbpp;
	uint16_t xstart;
	uint16_t ystart;
	uint16_t width;
	uint16_t height;
	uint8_t bpp;
	uint8_t flags;
} __attribute__((__packed__)) img_tgahead_t;

typedef struct img
{
	int udtype;
	img_tgahead_t head;
	uint32_t pixels[];
} img_t;

/*

Pillar data:

Note, indices are like so:
0 1 2 3

Column header:
L - - -:
  (L+1)*4 = length in bytes
  - = reserved

Chunk header:
N S E A:
  N = number of 4bytes including header this chunk has (N=0 => last chunk)
  S = starting block for top part
  E = ending block for top part
  A = air start after bottom part (N=0 => E-S+1 blocks are stored)

Block data:
B G R T:
  B = blue
  G = green
  R = red! suprised?
  T = type of block.

In other words, VOXLAP vxl with a length header and different 4th data byte,
  and you can actually store crap in the invisible sections.
(Trust me. This format packs incredibly well.)

If you're keen to store interesting stuff that's not visible,
feel free to store it in the "invisible" parts.

*Yes*, you can get away with this! We're not using a static 16MB heap.

*/

typedef struct map
{
	int udtype;
	int xlen, ylen, zlen;
	uint8_t **pillars;
	// TODO ? heap allocator ?
} map_t;

enum
{
	BT_INVALID = 0, // don't use this type!
	BT_SOLID_BREAKABLE,
	
	BT_MAX
};

typedef struct packet packet_t;
struct packet
{
	packet_t *p, *n;
	int sockfd;
	int len;
	char data[];
};

typedef struct client
{
	packet_t *head, *tail;
	packet_t *send_head, *send_tail;
	int sockfd;
	int isfull;
	
	// client only
	char *cfetch_ubuf;
	char *cfetch_cbuf;
	int cfetch_ulen, cfetch_clen;
	int cfetch_cpos;
	int cfetch_udtype;
	
	// server only
	char *sfetch_ubuf;
	char *sfetch_cbuf;
	int sfetch_ulen, sfetch_clen;
	int sfetch_cpos;
	int sfetch_udtype;
	
	// serialisation
	char rpkt_buf[PACKET_LEN_MAX*2];
	int rpkt_len;
	char spkt_buf[PACKET_LEN_MAX*2];
	int spkt_ppos,spkt_len;
} client_t;

struct netdata
{
	int sockfd;
} netdata_t;

#define SOCKFD_NONE -1
#define SOCKFD_LOCAL -2

enum
{
	PATH_INVALID_ENUM = 0, // don't use this!
	
	PATH_CLSAVE_BASEDIR,
	PATH_CLSAVE_BASEDIR_VOLATILE,
	PATH_CLSAVE_PUBLIC,
	PATH_CLSAVE_VOLATILE,
	PATH_SVSAVE_BASEDIR,
	PATH_SVSAVE_BASEDIR_VOLATILE,
	PATH_SVSAVE_PUBLIC,
	PATH_SVSAVE_VOLATILE,
	PATH_PKG_BASEDIR,
	PATH_PKG,
	
	PATH_ERROR_BADCHARS,
	PATH_ERROR_ACCDENIED,
	
	PATH_ENUM_MAX
};

// img.c
void img_free(img_t *img);
img_t *img_parse_tga(int len, const char *data);
img_t *img_load_tga(const char *fname);

// json.c
int json_parse(lua_State *L, const char *p);
int json_load(lua_State *L, const char *fname);

// lua.c
extern lua_State *lstate_client;
extern lua_State *lstate_server;
int icelua_initfetch(void);
int icelua_init(void);
void icelua_deinit(void);

// main.c
extern camera_t tcam;
extern map_t *clmap, *svmap;
#ifndef DEDI
extern SDL_Surface *screen;
#endif
extern int force_redraw;

extern int net_port;
extern char *net_addr;
extern int boot_mode;
extern char *mod_basedir;

extern int main_argc;
extern char **main_argv;
extern int main_largstart;

int run_game_cont1(void);
int run_game_cont2(void);

int error_sdl(char *msg);
int error_perror(char *msg);

// map.c
map_t *map_parse_aos(int len, const char *data);
map_t *map_parse_icemap(int len, const char *data);
map_t *map_load_aos(const char *fname);
map_t *map_load_icemap(const char *fname);
char *map_serialise_icemap(map_t *map, int *len);
int map_save_icemap(map_t *map, const char *fname);
void map_free(map_t *map);

// model.c
model_bone_t *model_bone_new(model_t *pmf, int ptmax);
model_bone_t *model_bone_extend(model_bone_t *bone, int ptmax);
void model_bone_free(model_bone_t *bone);
model_t *model_new(int bonemax);
model_t *model_extend(model_t *pmf, int bonemax);
void model_free(model_t *pmf);
model_t *model_parse_pmf(int len, const char *data);
model_t *model_load_pmf(const char *fname);
int model_save_pmf(model_t *pmf, const char *fname);

// network.c
extern client_t to_server;
extern client_t to_clients[];
extern client_t to_client_local;
char *net_fetch_file(const char *fname, int *flen);
int net_packet_push(int len, const char *data, int sockfd, packet_t **head, packet_t **tail);
int net_packet_push_lua(int len, const char *data, int sockfd, packet_t **head, packet_t **tail);
packet_t *net_packet_pop(packet_t **head, packet_t **tail);
void net_packet_free(packet_t *pkt, packet_t **head, packet_t **tail);
void net_flush(void);
int net_connect(void);
void net_disconnect(void);
int net_bind(void);
void net_unbind(void);
int net_init(void);
void net_deinit(void);

// path.c
char *path_filter(const char *path);
int path_get_type(const char *path);
int path_type_client_local(int type);
int path_type_client_readable(int type);
int path_type_client_writable(int type);
int path_type_server_readable(int type);
int path_type_server_writable(int type);

// render.c
#ifdef RENDER_FACE_COUNT
extern int render_face_remain;
#endif
#define FOG_MAX_DISTANCE 511.5f /* that's not going to work well, by the way! */
#define FOG_INIT_DISTANCE 60.0f
extern float fog_distance;
extern uint32_t fog_color;
void render_vxl_redraw(camera_t *camera, map_t *map);
void render_cubemap(uint32_t *pixels, int width, int height, int pitch, camera_t *camera, map_t *map);
void render_pmf_bone(uint32_t *pixels, int width, int height, int pitch, camera_t *cam_base,
	model_bone_t *bone, int islocal,
	float px, float py, float pz, float ry, float rx, float ry2, float scale);
void render_blit_img(uint32_t *pixels, int width, int height, int pitch,
	img_t *src, int dx, int dy, int bw, int bh, int sx, int sy, uint32_t color);
int render_init(int width, int height);
void render_deinit(void);

// vecmath.c
vec4f_t mtx_apply_vec(matrix_t *mtx, vec4f_t *vec);
void mtx_identity(matrix_t *mtx);
void cam_point_dir(camera_t *model, float dx, float dy, float dz, float zoom, float roll);

// wav.c
int wav_init(void);
void wav_deinit(void);
