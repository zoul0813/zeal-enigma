#include <stdio.h>
#include <zvb_gfx.h>
#include "assets.h"
#include "world.h"

gfx_sprite SPRITE_ARENA[MAX_SPRITES];
zar_file_t zar_file;
zar_file_entry_t entry;
uint8_t data[MAX_DATA_KB * 1024];

zos_err_t load_asset_file(const char *path) {
    zos_err_t err = zar_file_open(path, &zar_file);
    return err;
}

zos_err_t load_palette(gfx_context* ctx) {
    zos_err_t err = zar_file_entry_from_name(&zar_file, "tileset.ztp", &entry);
    if(err != ERR_SUCCESS) return err | 0xC0;

    uint16_t size = entry.size;
    err = zar_file_read(&zar_file, &entry, data, &size);
    if(err != ERR_SUCCESS) return err | 0xC0;

    err = gfx_palette_load(ctx, data, size, 0);
    if(err != ERR_SUCCESS) return err | 0x80;
    return err;
}

zos_err_t load_tileset(gfx_context* ctx, gfx_tileset_options* options) {
    zos_err_t err = zar_file_entry_from_name(&zar_file, "tileset.zts", &entry);
    if(err != ERR_SUCCESS) return err | 0xC0;

    uint16_t size = entry.size;
    err = zar_file_read(&zar_file, &entry, data, &size);
    if(err != ERR_SUCCESS) return err | 0xC0;

    err = gfx_tileset_load(ctx, data, size, options);
    if(err != ERR_SUCCESS) return err | 0x80;
    return err;
}

zos_err_t load_tilemap(gfx_context* ctx, uint8_t x, uint8_t y, uint8_t map_x, uint8_t map_y)
{
    (void)x;
    (void)y;

    uint8_t offset = (y * WORLD_WIDTH) + x;
    zos_err_t err = zar_file_entry_from_index(&zar_file, offset, &entry);
    if(err != ERR_SUCCESS) return err | 0xC0;

    uint16_t size = entry.size;
    err = zar_file_read(&zar_file, &entry, data, &size);
    if(err != ERR_SUCCESS) return err | 0xC0;

    uint8_t line[20];

    for (uint16_t row = 0; row < 15; row++) {
        uint16_t offset = row * 20;
        for (uint16_t col = 0; col < 20; col++) {
            uint8_t tile = data[offset + col];
            TileType type = TILE_OPEN;

            if((tile >= 1) && (tile <= 13)) {
                type = TILE_WALL;
            } else if((tile >= 16) && (tile <= 28)) {
                type = TILE_WATER;
            } else if(tile == 43) {
                type = TILE_CHEST;
            } else if(tile > 0) {
                type = TILE_UNKNOWN;
            }

            SCREEN[row][col] = type;

            line[col] = tile;
        }
        err = gfx_tilemap_load(ctx, line, 20, 0, (map_x * 20), (map_y * 15) + row);
        if(err != ERR_SUCCESS) return err | 0x80;
    }

    return ERR_SUCCESS;
}