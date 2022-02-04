#include "RenderContext.h"

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
