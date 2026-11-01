#include <stdint.h>
#include <zvb_gfx.h>
#include <zar.h>

#define MAX_SPRITES   32
#define MAX_DATA_KB   6
#define PALETTE_SIZE  16

extern gfx_sprite SPRITE_ARENA[MAX_SPRITES];
extern uint8_t data[MAX_DATA_KB * 1024];
extern uint16_t palette[PALETTE_SIZE];
extern zar_file_t zar_file;
extern zar_file_entry_t entry;

zos_err_t load_palette(gfx_context* ctx, uint8_t index, uint8_t from);
/* Enigma's tilesets contain raw packed 4-bit pixels. compression is not used. */
zos_err_t load_tileset(gfx_context* ctx, uint8_t index, gfx_tileset_options* options);
zos_err_t load_tilemap(gfx_context* ctx, uint8_t x, uint8_t y, uint8_t tile_x, uint8_t tile_y);

zos_err_t load_asset_file(const char *path);
