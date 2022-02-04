#ifndef TYPEDEFS_H_INCLUDED
#define TYPEDEFS_H_INCLUDED

struct BGTILE {
	uint8_t uv[2];
	uint8_t property;
};

struct STAGE {
	struct BGTILE tiles[32][40];
};

#endif // TYPEDEFS_H_INCLUDED
