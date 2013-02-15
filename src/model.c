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

model_t *model_new(int bonemax)
{
	model_t *pmf = (model_t*)malloc(sizeof(model_t)+sizeof(model_bone_t *)*bonemax);
	// TODO: check if NULL
	
	pmf->bonelen = 0;
	pmf->bonemax = bonemax;
	pmf->udtype = UD_PMF;
	
	return pmf;
}

model_t *model_extend(model_t *pmf, int bonemax)
{
	pmf = (model_t*)realloc(pmf, sizeof(model_t)+sizeof(model_bone_t *)*bonemax);
	// TODO: check if NULL
	
	pmf->bonemax = bonemax;
	
	return pmf;
}

void model_free(model_t *pmf)
{
	while(pmf->bonelen != 0)
		model_bone_free(pmf->bones[pmf->bonelen-1]);
	
	free(pmf);
}

model_bone_t *model_bone_new(model_t *pmf, int ptmax)
{
	model_bone_t *bone = (model_bone_t*)malloc(sizeof(model_bone_t)+sizeof(model_point_t)*ptmax);
	// TODO: check if NULL
	
	bone->ptlen = 0;
	bone->ptmax = ptmax;
	
	bone->parent = pmf;
	bone->parent_idx = pmf->bonelen++;
	pmf->bones[bone->parent_idx] = bone;

#ifdef USE_OPENGL
	bone->vbo_dirty = 1;
	bone->vbo = 0;
	bone->vbo_arr = NULL;
#endif
	
	return bone;
}

model_bone_t *model_bone_extend(model_bone_t *bone, int ptmax)
{
	model_t *pmf = bone->parent;
	bone = (model_bone_t*)realloc(bone, sizeof(model_bone_t)+sizeof(model_point_t)*ptmax);
	// TODO: check if NULL
	
	pmf->bones[bone->parent_idx] = bone;
	bone->ptmax = ptmax;
	
	return bone;
}

void model_bone_free(model_bone_t *bone)
{
	int i = bone->parent_idx;
	
	bone->parent->bonelen--;
	for(i = 0; i < bone->parent->bonelen; i++)
		bone->parent->bones[i] = bone->parent->bones[i+1];
	
	free(bone);
}

model_t *model_parse_pmf(int len, const char *data)
{
	int i,j;
	
	const char *p = data;
	const char *dend = data + len;
	
	// and now we crawl through the spec.
	
	// start with the header of "PMF",0x1A,1,0,0,0
	if(memcmp(p, "PMF\x1A\x01\x00\x00\x00", 8))
	{
		fprintf(stderr, "model_load_pmf: not a valid PMF v1 file\n");
		return NULL;
	}
	p += 8;
	
	// then there's a uint32_t denoting how many body parts there are
	uint32_t bone_count = *(uint32_t *)p;
	p += 4;
	if(bone_count > MODEL_BONE_MAX)
	{
		fprintf(stderr, "model_parse_pmf: too many bones (%i > %i)\n"
			, bone_count, MODEL_BONE_MAX);
		return NULL;
	}
	
	model_t *pmf = model_new(bone_count);
	if(pmf == NULL)
	{
		error_perror("model_parse_pmf");
		return NULL;
	}
	pmf->udtype = UD_PMF;
	
	// then, for each body part,
	for(i = 0; i < (int)bone_count; i++)
	{
		// there's a null-terminated 16-byte string (max 15 chars) denoting the part
		const char *namebuf = p;
		p += 16;
		
		if(namebuf[15] != '\x00')
		{
			fprintf(stderr, "model_load_pmf: name not null terminated\n");
			model_free(pmf);
			return NULL;
		}
		
		// then there's a uint32_t denoting how many points there are in this body part
		uint32_t pt_count = *(uint32_t *)p;
		p += 4;
		
		// (just allocating the bone here)
		model_bone_t *bone = model_bone_new(pmf, pt_count);
		pmf = bone->parent;
		memcpy(bone->name, namebuf, 16);
		if(bone == NULL)
		{
			error_perror("model_parse_pmf");
			model_free(pmf);
			return NULL;
		}
		
		// then there's a whole bunch of this:
		//   uint16_t radius;
		//   int16_t x,y,z;
		//   uint8_t b,g,r,reserved;
		int bonelen = sizeof(model_point_t)*pt_count;
		memcpy(bone->pts, p, bonelen);
		p += bonelen;
		bone->ptlen = pt_count;
		
		// "reserved" needs to be 0 or else you suck
		// NO SIDECHANNELING YOUR NAME IN THERE
		// i'm going to enforce this in the loader
		// and will outright reject files which don't have 0 in ALL of these slots
		for(j = 0; j < bone->ptmax; j++)
			if(bone->pts[j].resv1 != 0)
			{
				fprintf(stderr, "model_load_pmf: file corrupted or made by a smartass\n");
				model_free(pmf);
				return NULL;
			}
		
		// rinse, lather, repeat
		
		// units are 8:8 fixed point in terms of the vxl grid by default
	}
	
	return pmf;
}

model_t *model_load_pmf(const char *fname)
{
	int flen;
	char *buf = net_fetch_file(fname, &flen);
	if(buf == NULL)
		return NULL;
	model_t *ret = model_parse_pmf(flen, buf);
	free(buf);
	return ret;
}

int model_save_pmf(model_t *pmf, const char *fname)
{
	int i;
	
	FILE *fp = fopen(fname, "wb");
	
	// check for errors
	if(fp == NULL)
		return error_perror("model_save_pmf");
	
	// and now we crawl through the spec.
	
	// start with the header of "PMF",0x1A,1,0,0,0
	fwrite("PMF\x1A\x01\x00\x00\x00", 8, 1, fp);
	
	// then there's a uint32_t denoting how many body parts there are
	uint32_t bone_count = pmf->bonelen;
	fwrite(&bone_count, 4, 1, fp);
	
	// then, for each body part,
	for(i = 0; i < (int)bone_count; i++)
	{
		// there's a null-terminated 16-byte string (max 15 chars) denoting the part
		fwrite(pmf->bones[i]->name, 16, 1, fp);
		
		// then there's a uint32_t denoting how many points there are in this body part
		uint32_t pt_count = pmf->bones[i]->ptlen;
		fwrite(&pt_count, 4, 1, fp);
		
		// then there's a whole bunch of this:
		//   uint16_t radius;
		//   int16_t x,y,z;
		//   uint8_t b,g,r,reserved;
		fwrite(pmf->bones[i]->pts, sizeof(model_point_t), pt_count, fp);
		
		// rinse, lather, repeat
		
		// units are 8:8 fixed point in terms of the vxl grid by default
	}
	
	fclose(fp);
	
	return 0;
}
