#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "assets.h"
#include "game_world.h"

#define ROOM_SIZE (ROOM_WIDTH * ROOM_HEIGHT)

static uint8_t archive_data[sizeof(data)];
static uint16_t archive_size;
static uint8_t short_read;
static zos_err_t lookup_error, read_error;
static unsigned reads, uploads, decodes, collision_writes;

zos_err_t zar_file_open(const char *path, zar_file_t *file)
{
    (void)path; (void)file;
    return ERR_SUCCESS;
}

zos_err_t zar_file_entry_from_index(zar_file_t *file, uint8_t index, zar_file_entry_t *result)
{
    (void)file; (void)index;
    result->size = archive_size;
    return lookup_error;
}

zos_err_t zar_file_read(zar_file_t *file, zar_file_entry_t *item, uint8_t *destination, uint16_t *size)
{
    (void)file; (void)item;
    reads++;
    assert(*size <= sizeof(data) - (size_t)(destination - data));
    if (read_error) return read_error;
    if (short_read && *size) --*size;
    memcpy(destination, archive_data, *size);
    return ERR_SUCCESS;
}

zos_err_t gfx_palette_load(gfx_context *ctx, void *bytes, uint16_t size, uint8_t from)
{
    (void)ctx; (void)bytes; (void)size; (void)from;
    uploads++;
    return ERR_SUCCESS;
}

zos_err_t gfx_tileset_load(gfx_context *ctx, void *bytes, uint16_t size, const gfx_tileset_options *options)
{
    (void)ctx; (void)bytes; (void)size; (void)options;
    uploads++;
    return ERR_SUCCESS;
}

zos_err_t gfx_tilemap_load(gfx_context *ctx, void *bytes, uint16_t size, uint8_t layer, uint8_t x, uint8_t y)
{
    (void)ctx; (void)layer; (void)x; (void)y;
    assert(size == ROOM_WIDTH);
    assert(memcmp(bytes, data + (y - ROOM_OFFSET_Y) * ROOM_WIDTH, size) == 0);
    uploads++;
    return ERR_SUCCESS;
}

int16_t tilemap_set_xy(uint8_t x, uint8_t y, uint8_t value)
{
    assert(value == (data[y * ROOM_WIDTH + x] ? TILE_WALL : TILE_NONE));
    collision_writes++;
    return value;
}

/* Model the SDK decoder; assert that validation precedes every unsafe access. */
void decompress_rle(uint8_t *source, uint16_t size, uint8_t *output)
{
    uint16_t i = 0, j = 0;
    decodes++;
    while (i < size) {
        uint8_t code = source[i++];
        uint16_t length = (code & 0x7f) + 1;
        assert(length <= ROOM_SIZE - j);
        if (code & 0x80) {
            assert(i < size);
            memset(output + j, source[i++], length);
        } else {
            assert(length <= size - i);
            memcpy(output + j, source + i, length);
            i += length;
        }
        j += length;
    }
    assert(j == ROOM_SIZE);
}

static void reset_test(uint16_t size)
{
    archive_size = size;
    short_read = 0;
    lookup_error = read_error = 0;
    reads = uploads = decodes = collision_writes = 0;
    memset(data, 0xa5, sizeof(data));
}

static void reject_room(uint16_t size)
{
    reset_test(size);
    assert(load_tilemap(NULL, 0, 0, ROOM_OFFSET_X, ROOM_OFFSET_Y) != ERR_SUCCESS);
    assert(uploads == 0 && decodes == 0 && collision_writes == 0);
    for (unsigned i = 0; i < ROOM_SIZE; i++) assert(data[i] == 0xa5);
}

int main(void)
{
    gfx_tileset_options options = {0};

    /* All entry reads must reject oversize and empty input before reading. */
    reset_test(sizeof(data) + 1);
    assert(load_tileset(NULL, 0, &options) != ERR_SUCCESS && reads == 0);
    reset_test(sizeof(data) + 1);
    assert(load_palette(NULL, 0, 0) != ERR_SUCCESS && reads == 0);
    reset_test(sizeof(data) - ROOM_SIZE + 1);
    assert(load_tilemap(NULL, 0, 0, ROOM_OFFSET_X, ROOM_OFFSET_Y) != ERR_SUCCESS && reads == 0);
    reset_test(0);
    assert(load_tileset(NULL, 0, &options) != ERR_SUCCESS && reads == 0);

    /* Palette copying requires a complete, even-sized palette. */
    reset_test(sizeof(palette) - 2);
    assert(load_palette(NULL, 0, 0) != ERR_SUCCESS && uploads == 0);
    reset_test(sizeof(palette) + 1);
    assert(load_palette(NULL, 0, 0) != ERR_SUCCESS && uploads == 0);
    reset_test(sizeof(palette));
    assert(load_palette(NULL, 0, 0) == ERR_SUCCESS && uploads == 1);
    assert(memcmp(palette, archive_data, sizeof(palette)) == 0);
    reset_test(sizeof(data));
    assert(load_tileset(NULL, 0, &options) == ERR_SUCCESS && uploads == 1);

    /* Short reads and archive errors must never reach graphics. */
    reset_test(32); short_read = 1;
    assert(load_palette(NULL, 0, 0) != ERR_SUCCESS && uploads == 0);
    reset_test(32); short_read = 1;
    assert(load_tileset(NULL, 0, &options) != ERR_SUCCESS && uploads == 0);
    reset_test(32); short_read = 1;
    assert(load_tilemap(NULL, 0, 0, ROOM_OFFSET_X, ROOM_OFFSET_Y) != ERR_SUCCESS && decodes == 0);
    reset_test(32); lookup_error = ERR_FAILURE;
    assert(load_tileset(NULL, 0, &options) != ERR_SUCCESS && reads == 0);
    reset_test(32); read_error = ERR_FAILURE;
    assert(load_tileset(NULL, 0, &options) != ERR_SUCCESS && uploads == 0);

    reject_room(0);
    archive_data[0] = 0x80; reject_room(1); /* Missing repeat value. */
    archive_data[0] = 2; archive_data[1] = 1;
    reject_room(2); /* Truncated literal. */
    archive_data[0] = 0x80; archive_data[1] = 1;
    reject_room(2); /* Too little decoded output. */
    for (unsigned i = 0; i < 14; i += 2) {
        archive_data[i] = 0xff; archive_data[i + 1] = 0;
    }
    reject_room(14); /* Seven 128-tile runs exceed the room. */

    /* Exact output followed by an extra packet or trailing control byte. */
    archive_data[12] = 0xc7; /* Six runs of 128 plus 72 = 840. */
    reset_test(14);
    assert(load_tilemap(NULL, 0, 0, ROOM_OFFSET_X, ROOM_OFFSET_Y) == ERR_SUCCESS);
    assert(decodes == 1 && uploads == ROOM_HEIGHT && collision_writes == ROOM_SIZE);
    archive_data[14] = 0;
    reject_room(15);
    archive_data[15] = 0;
    reject_room(16);
    archive_data[14] = 0x80;
    reject_room(16);

    /* Maximum literal packets, including an exact 840-tile room. */
    unsigned at = 0;
    for (unsigned remaining = ROOM_SIZE; remaining;) {
        unsigned length = remaining > 128 ? 128 : remaining;
        archive_data[at++] = length - 1;
        memset(archive_data + at, 3, length);
        at += length;
        remaining -= length;
    }
    reset_test(at);
    assert(load_tilemap(NULL, 0, 0, ROOM_OFFSET_X, ROOM_OFFSET_Y) == ERR_SUCCESS);
    assert(decodes == 1 && uploads == ROOM_HEIGHT);

    unsigned rooms = 0;
    for (unsigned room = 0; room < 64; room++) {
        char filename[64];
        snprintf(filename, sizeof(filename), "build/assets/__room%04u.map", room);
        FILE *file = fopen(filename, "rb");
        if (!file) continue;
        size_t length = fread(archive_data, 1, sizeof(archive_data), file);
        assert(!ferror(file));
        fclose(file);
        reset_test(length);
        assert(load_tilemap(NULL, room % WORLD_WIDTH, room / WORLD_WIDTH, ROOM_OFFSET_X, ROOM_OFFSET_Y) == ERR_SUCCESS);
        assert(decodes == 1 && uploads == ROOM_HEIGHT && collision_writes == ROOM_SIZE);
        rooms++;
    }
    printf("Asset safety tests passed; %u generated rooms checked\n", rooms);
    return 0;
}
