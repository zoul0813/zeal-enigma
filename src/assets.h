#include <stdint.h>
#include <zvb_gfx.h>
#include <zar.h>

#define MAX_SPRITES   3
#define MAX_DATA_KB   16

extern gfx_sprite SPRITE_ARENA[MAX_SPRITES];
extern uint8_t data[MAX_DATA_KB * 1024];
extern zar_file_t zar_file;
extern zar_file_entry_t entry;

zos_err_t load_palette(gfx_context* ctx);
zos_err_t load_tileset(gfx_context* ctx, gfx_tileset_options* options);
zos_err_t load_tilemap(gfx_context* ctx, uint8_t x, uint8_t y, uint8_t map_x, uint8_t map_y);

zos_err_t load_asset_file(const char *path);