#include "gates.h"


object_t keys[TOTAL_GATES] = {
	{ .room = 48, .x = 20, .y = 18, .flags = 0x01, }, // value = 96
	{ .room = 16, .x = 20, .y = 11, .flags = 0x01, }, // value = 97
	{ .room = 26, .x = 12, .y = 11, .flags = 0x01, }, // value = 98
	{ .room = 33, .x = 12, .y = 11, .flags = 0x01, }, // value = 99
	{ .room = 47, .x = 28, .y = 11, .flags = 0x01, }, // value = 100
	{ .room = 60, .x = 20, .y = 11, .flags = 0x01, }, // value = 101
	{ .room = 62, .x = 28, .y = 11, .flags = 0x01, }, // value = 102
	{ .room = 58, .x = 12, .y = 11, .flags = 0x01, }, // value = 103
	{ .room = 45, .x = 12, .y = 4, .flags = 0x01, }, // value = 104
};
const uint8_t keys_len = 9;

