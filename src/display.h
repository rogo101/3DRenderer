#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define FPS 60
#define FRAME_TARGET_TIME (1000 / FPS)

typedef uint32_t color_t; //TODO:: Convert all color values typed as uint32_t to color_t

extern int window_width;
extern int window_height;
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern uint32_t* color_buffer;
extern float* z_buffer;
extern SDL_Texture* color_buffer_texture;

bool initialize_window(void);
void destroy_window(void);

bool render_color_buffer(void);
void clear_color_buffer(uint32_t color);
void clear_z_buffer();

void draw_pixel(int x, int y, uint32_t color);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
/**
*    Function for superimposing a black grid on top of a color buffer.
*    TODO:: connected: true: grid is connected; false: grid is not (dot spaced grid)
*    \param spacing: The amount of space in between grid lines (for both x and y)
*    \param thickness: The thickness of the lines that you want to render (note that a thickness of 1 is 1 pixel,
*                                                                   and thickness of n + 1 is n + 1 pixels)    
**/
void draw_grid(int spacing, int thickness, uint32_t color, bool connected); // TODO:: can we have optional parameters with default values in c
void draw_rectangle(int x_start, int y_start, int width, int height, uint32_t color);

#endif