#include <stdio.h>
#include <zvb_gfx.h>
#include <zgdk/compress/rle.h>

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

zos_err_t load_palette(gfx_context* ctx, const char *path, uint8_t from) {
    zos_err_t err = zar_file_entry_from_name(&zar_file, path, &entry);
    if(err != ERR_SUCCESS) return err | 0xC0;

    uint16_t size = entry.size;
    err = zar_file_read(&zar_file, &entry, data, &size);
    if(err != ERR_SUCCESS) return err | 0xC0;

    err = gfx_palette_load(ctx, data, size, from);
    if(err != ERR_SUCCESS) return err | 0x80;
    return err;
}

zos_err_t load_tileset(gfx_context* ctx, const char *path, gfx_tileset_options* options) {
    zos_err_t err = zar_file_entry_from_name(&zar_file, path, &entry);
    if(err != ERR_SUCCESS) return err | 0xC0;

    uint16_t size = entry.size;
    err = zar_file_read(&zar_file, &entry, data, &size);
    if(err != ERR_SUCCESS) return err | 0xC0;

    err = gfx_tileset_load(ctx, data, size, options);
    if(err != ERR_SUCCESS) return err | 0x80;
    return err;
}

zos_err_t load_tilemap(gfx_context* ctx, uint8_t x, uint8_t y, uint8_t map_x, uint8_t map_y) {
    (void)x;
    (void)y;

    uint8_t offset = (y * WORLD_WIDTH) + x;
    zos_err_t err = zar_file_entry_from_index(&zar_file, TILEMAP_OFFSET + offset, &entry);
    if (err != ERR_SUCCESS) return err | 0xC0;

    uint16_t size = entry.size;
    uint8_t *rle_data = &data[(ROOM_WIDTH * ROOM_HEIGHT)];
    err = zar_file_read(&zar_file, &entry, rle_data, &size);
    if (err != ERR_SUCCESS) return err | 0xC0;

    // decompress the RLE'd map
    decompress_rle(rle_data, size, data);

    uint8_t line[ROOM_WIDTH + 1];

    for (uint8_t row = 0; row < ROOM_HEIGHT; row++) {
        for (uint8_t col = 0; col < ROOM_WIDTH; col++) {
            uint8_t tile = data[(row * ROOM_WIDTH) + col];
            TileType type = TILE_OPEN;

            if ((tile == 1)) {
                type = TILE_WALL;
            } else if (tile > 0) {
                type = TILE_UNKNOWN;
            }

            SCREEN[row][col] = type;
            line[col] = tile;
        }

        err = gfx_tilemap_load(ctx, line, ROOM_WIDTH, LAYER0, (map_x * WIDTH) + ROOM_OFFSET_X, (map_y * HEIGHT) + row + ROOM_OFFSET_Y);
        if (err != ERR_SUCCESS) return err | 0x80;
    }

    return ERR_SUCCESS;
}
