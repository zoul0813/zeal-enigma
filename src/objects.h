#include <stdint.h>

#ifndef GATES_H
#define GATES_H

#define TOTAL_GATES 9
#define GATES_INDEX 80
#define GATE_LENGTH 8
#define KEYS_INDEX  96
#define FUEL_INDEX  5
#define FUEL_VALUE  10

typedef enum {
    OBJECT_INACTIVE = 0x00,
    OBJECT_ACTIVE   = 0x01,
    OBJECT_VERTICAL = 0x80,
} ObjectFlags;

typedef struct {
        uint8_t room;
        uint8_t id;
        uint8_t x;
        uint8_t y;
        ObjectFlags flags;
} object_t;

extern object_t gates[];
extern const uint8_t gates_len;
extern object_t keys[];
extern const uint8_t keys_len;
extern object_t fuel[];
extern const uint8_t fuel_len;


#endif
