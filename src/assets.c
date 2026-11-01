#include <zvb_gfx.h>
#include <zgdk/compress/rle.h>
#include <zgdk/tilemap.h>

#include "assets.h"
#include "game_world.h"

gfx_sprite SPRITE_ARENA[MAX_SPRITES];
zar_file_t zar_file;
zar_file_entry_t entry;
uint8_t data[MAX_DATA_KB * 1024];

uint16_t palette[PALETTE_SIZE];

#define ROOM_TILE_COUNT (ROOM_WIDTH * ROOM_HEIGHT)

static zos_err_t read_asset(uint8_t index, uint8_t *destination, uint16_t capacity, uint16_t *size) {
    zos_err_t err = zar_file_entry_from_index(&zar_file, index, &entry);
    if (err != ERR_SUCCESS) return err;
    if (entry.size == 0 || entry.size > capacity) return ERR_INVALID_PARAMETER;

    uint16_t expected = entry.size;
    *size = expected;
    err = zar_file_read(&zar_file, &entry, destination, size);
    if (err != ERR_SUCCESS) return err;
    if (*size != expected) return ERR_FAILURE;
    return ERR_SUCCESS;
}

/* Validate both packet input and exact room output before the SDK writes data. */
static zos_err_t validate_room_rle(const uint8_t *source, uint16_t size) {
    uint16_t input = 0;
    uint16_t output = 0;

    while (input < size) {
        uint8_t code = source[input++];
        uint16_t length = (code & 0x7f) + 1;
        uint16_t encoded = (code & 0x80) ? 1 : length;
        if (encoded > size - input || length > ROOM_TILE_COUNT - output)
            return ERR_INVALID_PARAMETER;
        input += encoded;
        output += length;
    }

    return output == ROOM_TILE_COUNT ? ERR_SUCCESS : ERR_INVALID_PARAMETER;
}

zos_err_t load_asset_file(const char *path) {
    zos_err_t err = zar_file_open(path, &zar_file);
    return err;
}

zos_err_t load_palette(gfx_context* ctx, uint8_t index, uint8_t from) {
    uint16_t size;
    zos_err_t err = read_asset(index, data, sizeof(data), &size);
    if(err != ERR_SUCCESS) return err | 0xC0;
    if (size < sizeof(palette) || (size & 1)) return ERR_INVALID_PARAMETER | 0xC0;

    err = gfx_palette_load(ctx, data, size, from);
    if(err != ERR_SUCCESS) return err | 0x80;

    uint8_t *copy_palette = (uint8_t*)&palette;
    for(uint8_t i = 0; i < sizeof(palette); i++) {
        copy_palette[i] = data[i];
    }

    return err;
}

zos_err_t load_tileset(gfx_context* ctx, uint8_t index, gfx_tileset_options* options) {
    uint16_t size;
    zos_err_t err = read_asset(index, data, sizeof(data), &size);
    if(err != ERR_SUCCESS) return err | 0xC0;

    err = gfx_tileset_load_none(ctx, data, size, options->from_byte, options->pal_offset, options->opacity);
    if(err != ERR_SUCCESS) return err | 0x80;
    return err;
}

zos_err_t load_tilemap(gfx_context* ctx, uint8_t x, uint8_t y, uint8_t tile_x, uint8_t tile_y) {
    uint8_t offset = (y * WORLD_WIDTH) + x;
    uint16_t size;
    uint8_t *rle_data = &data[ROOM_TILE_COUNT];
    zos_err_t err = read_asset(TILEMAP_OFFSET + offset, rle_data, sizeof(data) - ROOM_TILE_COUNT, &size);
    if (err != ERR_SUCCESS) return err | 0xC0;

    err = validate_room_rle(rle_data, size);
    if (err != ERR_SUCCESS) return err | 0xC0;
    decompress_rle(rle_data, size, data);

    uint8_t line[ROOM_WIDTH + 1];

    for (uint8_t row = 0; row < ROOM_HEIGHT; row++) {
        for (uint8_t col = 0; col < ROOM_WIDTH; col++) {
            uint8_t tile = data[(row * ROOM_WIDTH) + col];
            TileType type = TILE_NONE;

            if (tile > 0) {
                type = TILE_WALL;
            }

            tilemap_set_xy(col, row, type);
            line[col] = tile;
        }

        err = gfx_tilemap_load(ctx, line, ROOM_WIDTH, LAYER0, tile_x, tile_y + row);
        if (err != ERR_SUCCESS) return err | 0x80;
    }

    return ERR_SUCCESS;
}
