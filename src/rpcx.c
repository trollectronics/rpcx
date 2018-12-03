/*
Copyright (c) 2018 Steven Arnow <s@rdw.se>
Adapted for trollbook 2018 by Axel Isaksson <h4xxel>
'rpcx.c' - This file is part of rPCX 

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	distribution.
*/

#define LE_TO_WORD(arr, idx) (((arr)[(idx)]) | (((arr)[(idx)+1]) << 8))

#include <stdlib.h>
#include <string.h>
#include <posix-file.h>
#include "rpcx.h"


static void _process_bits(struct RPCXInfo *ri, unsigned char data, int row, int col, int plane) {
	int pixels = 8 / state->bpp;
	int i, shift;
	unsigned char mask;

	shift = state->bpp;
	mask = (0xFF << shift) ^ 0xFF;

	for (i = 0; i < pixels; i++)
		if (col * pixels + i < ri->w)
			ri->data[ri->w * row + col * pixels + i] |= ((data >> (shift * (pixels - i - 1))) & mask) << (shift * plane);
}


int rpcx_close(RPCXInfo *ri) {
	if (!ri->valid)
		return -1;
	
	if (ri->fd < 0)
		return -1;
	
	close(ri->fd);
	ri->fd = -1;
	return;
}


int rpcx_read(RPCXInfo *ri) {
	int row, plane, px, pxpl, j;
	unsigned char d;
	
	if (!ri->valid)
		return -1;
	
	if (ri->fd < 0)
		return -1;
	
	if (ri->bpp == 8)
		pxpl = ri->w;
	else if (ri->bpp == 1) {
		pxpl = ri->w >> 3;
		if (ri->w & 7)
			pxpl++;
	} else if (ri->bpp == 2) {
		pxpl = ri->w >> 2;
		if (ri->w & 3)
			pxpl++;
	} else {
		pxpl = ri->w >> 1;
		if (ri->w & 1)
			pxpl++;
	}


	for (row = 0; row < ri->h; row++) 
		for (plane = 0; plane < ri->planes; plane++) 
			for (px = 0; px < pxpl;) {
				read(fd, &d, 1);
				if ((d & 0xC0) == 0xC0) {
					j = (d & 0x3F);
					read(fd, &d, 1);
				} else
					j = 1;

				for (; j > 0; j--)
					_process_bits(ri, d, row, px++, plane);
			}

	read(ri->fd, ri->palette, 3*256);

	return 0;
}


RPCXInfo *rpcx_init(const char *fname) {
	RPCXInfo *ri = NULL;
	unsigned char data[128];
	signed short xmin, ymin, xmax, ymax;
	uint8_t *d = NULL;
	
	if(!(ri = malloc(sizeof(RPCXData))))
		goto fail;
	
	state.valid = false;
	ri->w = ri->h = 0;
	
	if ((ri->fd = open(fname, O_RDONLY)) < 0)
		goto fail;
	
	read(ri->fd, data, 128);
	
	if (data[0] != 0xA)
		goto fail;
	if (data[2] != 1)
		goto fail;
	ri->bpp = data[3];
	ri->planes = data[65];
	xmin = LE_TO_WORD(data, 4);
	ymin = LE_TO_WORD(data, 6);
	xmax = LE_TO_WORD(data, 8);
	ymax = LE_TO_WORD(data, 10);

	xmax++;
	ymax++;

	ri->w = xmax - xmin;
	ri->h = ymax - ymin;

	if (ri->planes != 1 && ri->bpp != 1)
		goto fail;
	
	state.valid = true;
	//ri->w = state.w;
	//ri->h = state.h;
	memcpy(ri->palette, data + 16, 48);
	
	//printf("%i x %i, %i planes, %i bits per pixel\n", state.w, state.h, state.planes, state.bpp);
	
	if(!(d = malloc(ri->w * ri->h)))
		goto fail;
	
	ri->data = d;
	
	memset(ri->data, 0, ri->w * ri->h);
	
	return ri;
	
	fail:
	free(ri);
	free(d);
	return NULL;
}


