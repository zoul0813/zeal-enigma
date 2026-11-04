#ifndef GAMEPLAY_TEST_PLATFORM_H
#define GAMEPLAY_TEST_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zgdk/types.h>

typedef uint8_t zos_err_t;
#define ERR_SUCCESS 0
#define ERR_FAILURE 1
#define ERR_NO_MORE_MEMORY 4
#define ERR_INVALID_PARAMETER 6
#define GFX_SUCCESS 0
#define SPRITE_NONE 0
#define SPRITE_FLIP_X 1

typedef struct { uint8_t unused; } gfx_context;
typedef struct { uint16_t x, y; uint8_t tile, flags; } gfx_sprite;
typedef struct { uint8_t unused; } gfx_tileset_options;
typedef struct { uint8_t unused; } zar_file_t;
typedef struct { uint16_t size; } zar_file_entry_t;
typedef struct { Point point; Rect rect; } Tile;

typedef struct { uint8_t *tiles; Rect rect; } Tilemap;
extern Tilemap *_TILEMAP;
extern Vector2_u8 world_position;
gfx_sprite *sprites_register(gfx_sprite);
void sprite_set_xy(gfx_sprite *, uint16_t, uint16_t);
uint8_t rand8(void);
int16_t tilemap_get_xy(uint8_t, uint8_t);
int16_t tilemap_get(const Point *);
int16_t tilemap_set_xy(uint8_t, uint8_t, uint8_t);
bool tile_get_edge(Rect *, Edge, Tile *);
void hud_score(uint32_t);
void hud_energy(uint8_t);
void hud_keys(uint16_t);

#endif
