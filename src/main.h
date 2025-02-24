#include <stdint.h>
#include <zos_errors.h>
#include <zvb_gfx.h>

#ifndef MAIN_H
#define MAIN_H

#define DEBUG 1

#define ACTION_NONE     0
#define ACTION_PAUSE    1
#define ACTION_CONTINUE 1
#define ACTION_QUIT     10

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480
#define WIDTH         40
#define HEIGHT        30

#define EMPTY_TILE 0x0F

#define LEFT1     (input1 & BUTTON_LEFT)
#define RIGHT1    (input1 & BUTTON_RIGHT)
#define UP1       (input1 & BUTTON_UP)
#define DOWN1     (input1 & BUTTON_DOWN)
#define BUTTON1_B (input1 & BUTTON_B)
#define BUTTON1_A (input1 & BUTTON_A)
#define START1    (input1 & BUTTON_START)
#define SELECT1   (input1 & BUTTON_SELECT)

typedef uint8_t error;

extern uint8_t buffer[WIDTH];

void on_error(zos_err_t err);
void init(void);
void deinit(void);
void reset(void);
uint8_t input(void);
void clear_tilemap(uint8_t layer, uint8_t tile, uint8_t x, uint8_t y, uint16_t width, uint16_t height);

void update(void);
void draw_paused(uint8_t paused);
void draw_gameover(uint8_t gameover);
void draw(void);

extern gfx_context vctx;

#endif
