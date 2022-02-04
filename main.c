#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxcd.h>
#include "TYPEDEFS.h"

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

void init_context(RenderContext *ctx) {
	RenderBuffer *db;

	ResetGraph(0);
	SetVideoMode(MODE_PAL);

	ctx->xres      = SCREEN_XRES;
	ctx->yres      = SCREEN_YRES;
	ctx->db_active = 0;

	db = &(ctx->db[0]);
	SetDefDispEnv(&(db->disp), 0, 0, SCREEN_XRES, SCREEN_YRES);
	SetDefDrawEnv(&(db->draw), 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);
	setRGB0(&(db->draw), BGCOLOR_R, BGCOLOR_G, BGCOLOR_B);
	db->draw.isbg = 1;
	db->draw.dtd  = 1;
	db->disp.screen.y = 24; // PAL offset

	db = &(ctx->db[1]);
	SetDefDispEnv(&(db->disp), 0, SCREEN_YRES, SCREEN_XRES, SCREEN_YRES);
	SetDefDrawEnv(&(db->draw), 0, 0, SCREEN_XRES, SCREEN_YRES);
	setRGB0(&(db->draw), BGCOLOR_R, BGCOLOR_G, BGCOLOR_B);
	db->draw.isbg = 1;
	db->draw.dtd  = 1;
	db->disp.screen.y = 24; // PAL offset

	// Set up the ordering tables and primitive buffers
	db = &(ctx->db[0]);
	ctx->db_nextpri = db->p;
	ClearOTagR((u_long *) db->ot, OT_LEN);

	PutDrawEnv(&(db->draw));
	PutDispEnv(&(db->disp));

	db = &(ctx->db[1]);
	ClearOTagR((u_long *) db->ot, OT_LEN);
}

void swap_buffers(RenderContext *ctx) {
	RenderBuffer *db;

	DrawSync(0);
	VSync(0);
	ctx->db_active ^= 1;

	// Clear the buffer that was being displayed up until now
	db = &(ctx->db[ctx->db_active]);
	ctx->db_nextpri = db->p;
	ClearOTagR((u_long *) db->ot, OT_LEN);

	PutDrawEnv(&(db->draw));
	PutDispEnv(&(db->disp));
	SetDispMask(1);

	// Start drawing the buffer that has just been filled
	db = &(ctx->db[!ctx->db_active]);
	DrawOTag((u_long *) &(db->ot[OT_LEN - 1]));
}

/* Image loading */

void LoadTexture(u_long *tim, TIM_IMAGE *tparam) {
	// Read TIM information (PSn00bSDK)
	GetTimInfo(tim, tparam);

	// Upload pixel data to framebuffer
	LoadImage(tparam->prect, (u_long*)tparam->paddr);
	DrawSync(0);

	// Upload CLUT to framebuffer
	LoadImage(tparam->crect, (u_long*)tparam->caddr);
	DrawSync(0);

}

unsigned long *load_file(const char* filename)
{
	CdlFILE	file;
	unsigned long *buffer;


	printf( "Reading file %s... ", filename );

	// Search for the file
	if( !CdSearchFile( &file, (char*)filename ) )
	{
		// Return value is NULL, file is not found
		printf( "Not found!\n" );
		buffer = NULL;
		//return NULL;
	}

	// Allocate a buffer for the file
	buffer = (unsigned long*)malloc( 2048*((file.size+2047)/2048) );

	// Set seek target (seek actually happens on CdRead())
	CdControl( CdlSetloc, (unsigned char*)&file.pos, 0 );

	// Read sectors
	CdRead( (file.size+2047)/2048,buffer, CdlModeSpeed );

	// Wait until read has completed
	CdReadSync( 0, 0 );

	printf( "Done.\n" );

	return buffer;
}

/* Main */

#define ENABLE_DITHER 1

static RenderContext global_ctx;
static TIM_IMAGE quest;


void draw_sprites(RenderContext *ctx,struct STAGE* level) {
	RenderBuffer *db = &(ctx->db[ctx->db_active]);
	SPRT_8 *sprttile = (SPRT_8 *) ctx->db_nextpri;

	// Calculate U,V offset for TIMs that are not page aligned
	uint16_t tim_clutx = quest.crect->x;
	uint16_t tim_cluty = quest.crect->y;
	uint16_t tim_uoffs = (quest.prect->x&0x3f)<<(2-(quest.mode&0x3));
	uint16_t tim_voffs = (quest.prect->y&0xff);

	// Sort textured sprite
	for(int8_t y = 0; y <32; y++){
        for(int8_t i = 0; i < 40; i++){

            setSprt8(sprttile);                  // Initialize the primitive (very important)
            setXY0(sprttile,(i*8), (y*8));           // Position the sprite at (48,48)
            setUV0(sprttile,                    // Set UV coordinates
                tim_uoffs + level->tiles[y][i].uv[0],
                tim_voffs + level->tiles[y][i].uv[1]);
            setClut(sprttile,                   // Set CLUT coordinates to sprite
                tim_clutx,
                tim_cluty);
            setRGB0(sprttile,                   // Set primitive color
                128, 128, 128);

            addPrim(&(db->ot[OT_LEN - 1]), sprttile);          // Sort primitive to OT
            sprttile++; // Add sizeof(SPRT_8) to sprttile pointer
        }
	}

	// Update the primitive buffer pointer in the context
	ctx->db_nextpri = (uint8_t *) sprttile;

	// Add a TPAGE "primitive" to ensure the GPU finds the texture
	DR_TPAGE *tpri = (DR_TPAGE *) ctx->db_nextpri;

	setDrawTPage(tpri, 0, ENABLE_DITHER, getTPage(quest.mode&0x3, 0, quest.prect->x, quest.prect->y));
	addPrim(&(db->ot[OT_LEN - 1]), tpri);
	tpri++;

	// Update the primitive buffer pointer in the context (again)
	ctx->db_nextpri = (uint8_t *) tpri;
}

int main(){
	init_context(&global_ctx);
	CdInit();
    struct STAGE* level = malloc(sizeof(struct STAGE));

	u_long *data = load_file("\\Q.TIM;1");
	LoadTexture(data, &quest);
	free(data); // Data is not needed after uploading to VRAM

    //u_long *lvldata = load_file("\\LVL.BAL;1");
    //level = (struct STAGE*)lvldata;
    //free(lvldata);

    printf("%d %d %d",level->tiles[0][0].uv[0],level->tiles[0][0].uv[1],level->tiles[0][0].property);
	while (1) {
		draw_sprites(&global_ctx,level);
		swap_buffers(&global_ctx);
	}
}
