#ifndef ASSETS_TEST_PLATFORM_H
#define ASSETS_TEST_PLATFORM_H

#include <stdint.h>

typedef uint8_t zos_err_t;
#define ERR_SUCCESS 0
#define ERR_FAILURE 1
#define ERR_INVALID_PARAMETER 6
#define LAYER0 0
#define SPRITE_WIDTH 16
#define SPRITE_HEIGHT 16

typedef struct { uint8_t unused; } gfx_context;
typedef struct { uint8_t unused; } gfx_sprite;
typedef struct { uint8_t unused; } gfx_tileset_options;
typedef struct { uint8_t unused; } Tilemap;
typedef struct { uint8_t x, y; } Vector2_u8;
typedef struct { uint16_t x, y; } Point;
typedef enum { EdgeNone } Edge;
typedef struct { uint8_t unused; } zar_file_t;
typedef struct { uint16_t position, size, cursor; } zar_file_entry_t;

zos_err_t zar_file_open(const char *, zar_file_t *);
zos_err_t zar_file_entry_from_index(zar_file_t *, uint8_t, zar_file_entry_t *);
zos_err_t zar_file_read(zar_file_t *, zar_file_entry_t *, uint8_t *, uint16_t *);
zos_err_t gfx_palette_load(gfx_context *, void *, uint16_t, uint8_t);
zos_err_t gfx_tileset_load(gfx_context *, void *, uint16_t, const gfx_tileset_options *);
zos_err_t gfx_tilemap_load(gfx_context *, void *, uint16_t, uint8_t, uint8_t, uint8_t);
int16_t tilemap_set_xy(uint8_t, uint8_t, uint8_t);
void decompress_rle(uint8_t *, uint16_t, uint8_t *);

#endif
