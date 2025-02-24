#include <stdint.h>

#ifndef GATES_H
#define GATES_H

#define TOTAL_GATES 9
#define GATES_INDEX 80
#define KEYS_INDEX  96

typedef enum {
    OBJECT_INACTIVE = 0,
    OBJECT_ACTIVE   = 1,
} ObjectFlags;

typedef struct {
        uint8_t room;
        uint8_t x;
        uint8_t y;
        ObjectFlags flags;
} object_t;

extern object_t gates[];
extern const uint8_t gates_len;
extern object_t keys[];
extern const uint8_t keys_len;


#endif
