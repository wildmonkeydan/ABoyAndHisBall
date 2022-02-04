#ifndef RENDERCONTEXT_H_INCLUDED
#define RENDERCONTEXT_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxcd.h>

/* Display/GPU context utilities */

#define SCREEN_XRES 320
#define SCREEN_YRES 256

#define BGCOLOR_R 0
#define BGCOLOR_G 0
#define BGCOLOR_B 0

#define OT_LEN     8
#define PACKET_LEN 32768

typedef struct {
	DISPENV  disp;
	DRAWENV  draw;
	uint32_t ot[OT_LEN];
	uint8_t  p[PACKET_LEN];
} RenderBuffer;

typedef struct {
	uint16_t xres, yres;
	RenderBuffer db[2];
	uint32_t db_active;
	uint8_t	 *db_nextpri;
} RenderContext;

void init_context(RenderContext *ctx);

void swap_buffers(RenderContext *ctx);

#endif // RENDERCONTEXT_H_INCLUDED
