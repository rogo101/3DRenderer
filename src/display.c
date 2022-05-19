#include <math.h>
#include "display.h"

int window_width = -1;
int window_height = -1;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

uint32_t* color_buffer = NULL;
float* z_buffer = NULL;
SDL_Texture* color_buffer_texture = NULL;

bool initialize_window(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initializing SDL.\n");
        return false;
    }

    // Use SDL to query what is the fullscreen max. width and height
    SDL_DisplayMode display_mode;
    SDL_GetCurrentDisplayMode(
        0,                      // Display index (0 for default)
        &display_mode           // Pointer to display_mode struct
    );
    window_width = display_mode.w;
    window_height = display_mode.h;

    // Create an SDL Window
    window = SDL_CreateWindow(
        "Hello",                // Window Title
        SDL_WINDOWPOS_CENTERED, // x Loc
        SDL_WINDOWPOS_CENTERED, // y Loc
        window_width,           // width
        window_height,          // height
        SDL_WINDOW_BORDERLESS   // flags
    );
    if (!window) {
        fprintf(stderr, "Error creating SDL window.\n");
        return false;
    }
    // Create an SDL Rendere
    renderer = SDL_CreateRenderer(
        window,                 // Window to render in
        -1,                     // Display device (usually a code); -1 uses default display device
        0                       // flags
    );
    if (!renderer) {
        fprintf(stderr, "Error creating SDL renderer.\n");
        return false;
    }
    // if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) < 0) {
    //     fprintf(stderr, "Error setting window to full screen mode.\n");
    //     return false;
    // }

    return true;
}

void destroy_window(void) {
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

bool render_color_buffer(void) {
    int success = 0;
    success = SDL_UpdateTexture(
        color_buffer_texture,
        NULL,
        color_buffer,
        (int) (window_width * sizeof(uint32_t))
    );
    if (success < 0) {
        fprintf(stderr, "Error rendering color_buffer: UpdateTexture\n");
        return false;
    }
    success = SDL_RenderCopy(
        renderer,
        color_buffer_texture,
        NULL,
        NULL
    );
    if (success < 0) {
        fprintf(stderr, "Error rendering color_buffer: RenderCopy\n");
        return false;
    }
    
    return true;
}

void clear_color_buffer(uint32_t color) {
    for (int y = 0; y < window_height; y++) {
        for (int x = 0; x < window_width; x++) {
            draw_pixel(x, y, color);
        }
    }
}

void clear_z_buffer() {
    for (int y = 0; y < window_height; y++) {
        for (int x = 0; x < window_width; x++) {
           z_buffer[(window_width * y) + x] = 1.0; 
        }
    }
}

void draw_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && y >= 0 && x < window_width && y < window_height) {
        color_buffer[(window_width * y) + x] = color;
    }
}

/**
*    Function for drawing a line using DDA algorithm.
**/
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int delta_x = (x1 - x0);
    int delta_y = (y1 - y0);

    int side_length = (abs(delta_x) >= abs(delta_y)) ? abs(delta_x) : abs(delta_y);

    // Find how much we should increment in both x and y each step
    float x_inc = delta_x / (float) side_length;
    float y_inc = delta_y / (float) side_length;

    float current_x = x0;
    float current_y = y0;

    for(int i = 0; i <= side_length; i++) {
        draw_pixel(round(current_x), round(current_y), color);
        current_x += x_inc;
        current_y += y_inc;
    }
}

/**
*    Function for superimposing a black grid on top of a color buffer.
*    \param spacing: The amount of space in between grid lines (for both x and y)
*    \param thickness: The thickness of the lines that you want to render (note that a thickness of 1 is 1 pixel,
*                                                                   and thickness of n + 1 is n + 1 pixels)
*    \param color: Desired color of the grid in uint32_t => 0xAARRGGBB format
*    \param connected: true: grid is connected; false: grid is not (dot spaced grid). 
*                                                                   currently only implemented for when thickness == 1, else ignored
**/
void draw_grid(int spacing, int thickness, uint32_t color, bool connected) {
    int jumpAmount = 1;
    if (!connected && thickness == 1) {
        jumpAmount = spacing;
    }
    for (int y = 0; y < window_height; y+=jumpAmount) {
        for (int x = 0; x < window_width; x+=jumpAmount) {
            if (y % spacing < thickness || x % spacing < thickness) {
                draw_pixel(x, y, color);
            }
        }
    }
}

void draw_rectangle(int x_start, int y_start, int width, int height, uint32_t color) {
    for (int y = y_start; y < y_start + height; y++) {
        for (int x = x_start; x < x_start + width; x++) {
            draw_pixel(x, y, color);
        }
    }
}