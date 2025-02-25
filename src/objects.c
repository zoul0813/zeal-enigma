#include "objects.h"


object_t keys[] = {
	{ .room = 48, .id = 0, .x = 20, .y = 18, .flags = 1, }, // value = 96
	{ .room = 16, .id = 1, .x = 20, .y = 11, .flags = 1, }, // value = 97
	{ .room = 26, .id = 2, .x = 12, .y = 11, .flags = 1, }, // value = 98
	{ .room = 33, .id = 3, .x = 12, .y = 11, .flags = 1, }, // value = 99
	{ .room = 47, .id = 4, .x = 28, .y = 11, .flags = 1, }, // value = 100
	{ .room = 60, .id = 5, .x = 20, .y = 11, .flags = 1, }, // value = 101
	{ .room = 62, .id = 6, .x = 28, .y = 11, .flags = 1, }, // value = 102
	{ .room = 58, .id = 7, .x = 12, .y = 11, .flags = 1, }, // value = 103
	{ .room = 45, .id = 8, .x = 12, .y = 4, .flags = 1, }, // value = 104
};
const uint8_t keys_len = 9;

object_t gates[] = {
	{ .room = 0, .id = 5, .x = 1, .y = 20, .flags = 1, }, // value = 85
	{ .room = 3, .id = 7, .x = 8, .y = 20, .flags = 1, }, // value = 87
	{ .room = 4, .id = 1, .x = 23, .y = 1, .flags = 129, }, // value = 81
	{ .room = 5, .id = 3, .x = 32, .y = 7, .flags = 129, }, // value = 83
	{ .room = 8, .id = 5, .x = 1, .y = 0, .flags = 1, }, // value = 85
	{ .room = 11, .id = 7, .x = 8, .y = 0, .flags = 1, }, // value = 87
	{ .room = 12, .id = 1, .x = 7, .y = 14, .flags = 129, }, // value = 81
	{ .room = 15, .id = 3, .x = 8, .y = 20, .flags = 1, }, // value = 83
	{ .room = 18, .id = 5, .x = 24, .y = 20, .flags = 1, }, // value = 85
	{ .room = 20, .id = 5, .x = 16, .y = 20, .flags = 1, }, // value = 85
	{ .room = 21, .id = 1, .x = 8, .y = 20, .flags = 1, }, // value = 81
	{ .room = 22, .id = 1, .x = 8, .y = 20, .flags = 1, }, // value = 81
	{ .room = 23, .id = 3, .x = 8, .y = 0, .flags = 1, }, // value = 83
	{ .room = 25, .id = 6, .x = 24, .y = 20, .flags = 1, }, // value = 86
	{ .room = 26, .id = 5, .x = 24, .y = 0, .flags = 1, }, // value = 85
	{ .room = 28, .id = 5, .x = 16, .y = 0, .flags = 1, }, // value = 85
	{ .room = 29, .id = 1, .x = 8, .y = 0, .flags = 1, }, // value = 81
	{ .room = 30, .id = 1, .x = 8, .y = 0, .flags = 1, }, // value = 81
	{ .room = 31, .id = 1, .x = 16, .y = 20, .flags = 1, }, // value = 81
	{ .room = 33, .id = 6, .x = 24, .y = 0, .flags = 1, }, // value = 86
	{ .room = 34, .id = 2, .x = 16, .y = 20, .flags = 1, }, // value = 82
	{ .room = 36, .id = 2, .x = 16, .y = 0, .flags = 1, }, // value = 82
	{ .room = 39, .id = 1, .x = 16, .y = 0, .flags = 1, }, // value = 81
	{ .room = 41, .id = 4, .x = 1, .y = 14, .flags = 1, }, // value = 84
	{ .room = 44, .id = 8, .x = 25, .y = 20, .flags = 1, }, // value = 88
	{ .room = 45, .id = 0, .x = 1, .y = 20, .flags = 1, }, // value = 80
	{ .room = 47, .id = 8, .x = 32, .y = 14, .flags = 1, }, // value = 88
	{ .room = 50, .id = 4, .x = 23, .y = 15, .flags = 129, }, // value = 84
	{ .room = 52, .id = 8, .x = 25, .y = 0, .flags = 1, }, // value = 88
	{ .room = 53, .id = 0, .x = 1, .y = 0, .flags = 1, }, // value = 80
	{ .room = 58, .id = 4, .x = 24, .y = 1, .flags = 129, }, // value = 84
	{ .room = 61, .id = 0, .x = 14, .y = 7, .flags = 129, }, // value = 80
};
const uint8_t gates_len = 32;

object_t fuel[] = {
	{ .room = 0, .id = 5, .x = 3, .y = 3, .flags = 1, }, // value = 5
	{ .room = 1, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 2, .id = 5, .x = 3, .y = 10, .flags = 1, }, // value = 5
	{ .room = 3, .id = 5, .x = 3, .y = 3, .flags = 1, }, // value = 5
	{ .room = 5, .id = 5, .x = 19, .y = 10, .flags = 1, }, // value = 5
	{ .room = 8, .id = 5, .x = 27, .y = 10, .flags = 1, }, // value = 5
	{ .room = 9, .id = 5, .x = 35, .y = 3, .flags = 1, }, // value = 5
	{ .room = 11, .id = 5, .x = 19, .y = 17, .flags = 1, }, // value = 5
	{ .room = 14, .id = 5, .x = 3, .y = 3, .flags = 1, }, // value = 5
	{ .room = 16, .id = 5, .x = 3, .y = 3, .flags = 1, }, // value = 5
	{ .room = 19, .id = 5, .x = 11, .y = 3, .flags = 1, }, // value = 5
	{ .room = 20, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 21, .id = 5, .x = 35, .y = 10, .flags = 1, }, // value = 5
	{ .room = 22, .id = 5, .x = 35, .y = 3, .flags = 1, }, // value = 5
	{ .room = 23, .id = 5, .x = 19, .y = 17, .flags = 1, }, // value = 5
	{ .room = 25, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 26, .id = 5, .x = 35, .y = 3, .flags = 1, }, // value = 5
	{ .room = 30, .id = 5, .x = 27, .y = 10, .flags = 1, }, // value = 5
	{ .room = 32, .id = 5, .x = 3, .y = 3, .flags = 1, }, // value = 5
	{ .room = 33, .id = 5, .x = 3, .y = 3, .flags = 1, }, // value = 5
	{ .room = 36, .id = 5, .x = 35, .y = 17, .flags = 1, }, // value = 5
	{ .room = 37, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 38, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 39, .id = 5, .x = 27, .y = 17, .flags = 1, }, // value = 5
	{ .room = 41, .id = 5, .x = 3, .y = 17, .flags = 1, }, // value = 5
	{ .room = 42, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 43, .id = 5, .x = 19, .y = 17, .flags = 1, }, // value = 5
	{ .room = 44, .id = 5, .x = 19, .y = 10, .flags = 1, }, // value = 5
	{ .room = 46, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 48, .id = 5, .x = 19, .y = 10, .flags = 1, }, // value = 5
	{ .room = 50, .id = 5, .x = 19, .y = 3, .flags = 1, }, // value = 5
	{ .room = 51, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 54, .id = 5, .x = 35, .y = 17, .flags = 1, }, // value = 5
	{ .room = 55, .id = 5, .x = 19, .y = 17, .flags = 1, }, // value = 5
	{ .room = 58, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 59, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 62, .id = 5, .x = 11, .y = 10, .flags = 1, }, // value = 5
	{ .room = 63, .id = 5, .x = 35, .y = 3, .flags = 1, }, // value = 5
};
const uint8_t fuel_len = 38;

