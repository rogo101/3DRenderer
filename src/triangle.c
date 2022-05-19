#include "triangle.h"
#include "display.h"
#include "swap.h"

vec3_t barycentric_weights(vec2_t a, vec2_t b, vec2_t c, vec2_t p) {
    vec2_t ab = vec2_sub(b, a);
    vec2_t bc = vec2_sub(c, b);
    vec2_t ac = vec2_sub(c, a);
    vec2_t ap = vec2_sub(p, a);
    vec2_t bp = vec2_sub(p, b);

    float area_triangle_abc = (ab.x * ac.y - ab.y * ac.x);

    float alpha = (bc.x * bp.y - bp.x * bc.y) / area_triangle_abc;

    float beta = (ap.x * ac.y - ac.x * ap.y) / area_triangle_abc;

    float gamma = 1 - alpha - beta;

    vec3_t weights = {alpha, beta, gamma};
    return weights;
}

void draw_unfilled_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    draw_line(x0, y0, x1, y1, color);
    draw_line(x1, y1, x2, y2, color);
    draw_line(x2, y2, x0, y0, color);
}

void draw_filled_triangle(
    int x0, int y0, float w0,
    int x1, int y1, float w1,
    int x2, int y2, float w2,
    uint32_t color
) {
    // Sort the vertices and their corresponding uv values by y-coordinate ascending (y0, y1, y2)
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&w0, &w1);
    }
    if (y1 > y2) {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&w1, &w2);
    }
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&w0, &w1);
    }

    // Create vector points after we sort the vertices
    vec2_t point_a = {x0, y0}; // and w0
    vec2_t point_b = {x1, y1}; // and w1
    vec2_t point_c = {x2, y2}; // and w2

    float inv_slope_1 = 0;
    float inv_slope_2 = 0;

    if (y1 - y0 != 0)
        inv_slope_1 = (float) (x1 - x0) / abs(y1 - y0);
    if (y2 - y0 != 0)
        inv_slope_2 = (float) (x2 - x0) / abs(y2 - y0);

    if (y1 - y0 != 0) {
        for (int y = y0; y <= y1; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;
            if (x_end < x_start) int_swap(&x_start, &x_end);
            for (int x = x_start; x < x_end; x++) {
                vec2_t point_p = {x, y};
                // Get the barycentric weights for the current point
                vec3_t weights = barycentric_weights(point_a, point_b, point_c, point_p);
                // Use the barycentric weights to caluclate the interpolated reciprocal of the non perspective divided z: w
                float interpolated_reciprocal_w = (1 / w0) * weights.x + (1 / w1) * weights.y + (1 / w2) * weights.z;
                interpolated_reciprocal_w = 1 - interpolated_reciprocal_w;
                // Determine if this pixel is closer to the screen, and if so, render it and update the z-buffer
                if (interpolated_reciprocal_w < z_buffer[(window_width * y) + x]) {
                    // Draw a pixel at position (x, y) with the color that comes from the mapped texture
                    draw_pixel(x, y, color);
                    // Update the z-buffer value with the 1/w of this current pixel
                    z_buffer[(window_width * y) + x] = interpolated_reciprocal_w;
                }
            }
        }
    }

    // Render the bottom part of the triangle (flat-top)
    inv_slope_1 = 0;
    inv_slope_2 = 0;

    if (y2 - y1 != 0)
        inv_slope_1 = (float) (x2 - x1) / abs(y2 - y1);
    if (y2 - y0 != 0)
        inv_slope_2 = (float) (x2 - x0) / abs(y2 - y0);

    if (y2 - y1 != 0) {
        for (int y = y1; y <= y2; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;
            if (x_end < x_start) int_swap(&x_start, &x_end);
            for (int x = x_start; x < x_end; x++) {
                vec2_t point_p = {x, y};
                // Get the barycentric weights for the current point
                vec3_t weights = barycentric_weights(point_a, point_b, point_c, point_p);
                // Use the barycentric weights to caluclate the interpolated reciprocal of the non perspective divided z: w
                float interpolated_reciprocal_w = (1 / w0) * weights.x + (1 / w1) * weights.y + (1 / w2) * weights.z;
                interpolated_reciprocal_w = 1 - interpolated_reciprocal_w;
                // Determine if this pixel is closer to the screen, and if so, render it and update the z-buffer
                if (interpolated_reciprocal_w < z_buffer[(window_width * y) + x]) {
                    // Draw a pixel at position (x, y) with the color that comes from the mapped texture
                    draw_pixel(x, y, color);
                    // Update the z-buffer value with the 1/w of this current pixel
                    z_buffer[(window_width * y) + x] = interpolated_reciprocal_w;
                }
            }
        }
    }
}

// Function to draw the textured pixel at position x and y using interpolation
void draw_texel(
    int x, int y, uint32_t* texture,
    vec4_t point_a, vec4_t point_b, vec4_t point_c,
    tex2_t a_uv, tex2_t b_uv, tex2_t c_uv
) {
    vec2_t a = vec2_from_vec4(point_a);
    vec2_t b = vec2_from_vec4(point_b);
    vec2_t c = vec2_from_vec4(point_c);
    vec2_t p = {x, y};
    vec3_t weights = barycentric_weights(a, b, c, p);

    float alpha = weights.x
    ;
    float beta = weights.y;
    float gamma = weights.z;

    // Variables to store the interpolated values of U, V, and also 1/w for the current pixel
    float interpolated_u;
    float interpolated_v;
    float interpolated_reciprocal_w; // Cannot directly interpolate w since it is non linear, however the reciprocal is linear


    // Perform the interpolation of all U/w and V/w using barycentric weights and a factor of 1/w
    interpolated_u = (a_uv.u / point_a.w) * alpha + (b_uv.u / point_b.w) * beta + (c_uv.u / point_c.w) * gamma;
    interpolated_v = (a_uv.v / point_a.w) * alpha + (b_uv.v / point_b.w) * beta + (c_uv.v / point_c.w) * gamma;

    // Also interpolate the value of 1/w for the current pixel
    interpolated_reciprocal_w = (1 / point_a.w) * alpha + (1 / point_b.w) * beta + (1 / point_c.w) * gamma;

    // Now we can divide back both interpolated values by 1/w
    interpolated_u /= interpolated_reciprocal_w;
    interpolated_v /= interpolated_reciprocal_w;

    // Map the UV Coordinate to the full texture width and height
    int tex_x = abs((int)(interpolated_u * texture_width)) % texture_width;
    int tex_y = abs((int)(interpolated_v * texture_height)) % texture_height;

    // Adjust 1/w values so that the pixels that are closer to the camera have smaller values
    interpolated_reciprocal_w = 1.0 - interpolated_reciprocal_w;

    // Only draw the pixel if the depth value is less than the one previously stored in the z-buffer
    if (interpolated_reciprocal_w < z_buffer[(window_width * y) + x]) {
        // Draw a pixel at position (x, y) with the color that comes from the mapped texture
        draw_pixel(x, y, texture[(texture_width * tex_y) + tex_x]);

        // Update the z-buffer value with the 1/w of this current pixel
        z_buffer[(window_width * y) + x] = interpolated_reciprocal_w;
    }
}

void draw_textured_triangle(
    int x0, int y0, float z0, float w0, float u0, float v0,
    int x1, int y1, float z1, float w1, float u1, float v1,
    int x2, int y2, float z2, float w2, float u2, float v2,
    uint32_t* texture
) {
    // Sort the vertices and their corresponding uv values by y-coordinate ascending (y0, y1, y2)
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
    }
    if (y1 > y2) {
        int_swap(&y1, &y2);
        int_swap(&x1, &x2);
        float_swap(&z1, &z2);
        float_swap(&w1, &w2);
        float_swap(&u1, &u2);
        float_swap(&v1, &v2);
    }
    if (y0 > y1) {
        int_swap(&y0, &y1);
        int_swap(&x0, &x1);
        float_swap(&z0, &z1);
        float_swap(&w0, &w1);
        float_swap(&u0, &u1);
        float_swap(&v0, &v1);
    }

    // Flip the v component to account for inverted UV-coordinated (V grows downwards)
    v0 = 1 - v0;
    v1 = 1 - v1;
    v2 = 1 - v2;

    // Create vector points and texture coordinates after we sort the vertices
    vec4_t point_a = {x0, y0, z0, w0};
    vec4_t point_b = {x1, y1, z1, w1};
    vec4_t point_c = {x2, y2, z2, w2};
    tex2_t a_uv = {u0, v0};
    tex2_t b_uv = {u1, v1};
    tex2_t c_uv = {u2, v2};

    // Render the upper part of the triangle (flat-bottom)

    float inv_slope_1 = 0;
    float inv_slope_2 = 0;

    if (y1 - y0 != 0)
        inv_slope_1 = (float) (x1 - x0) / abs(y1 - y0);
    if (y2 - y0 != 0)
        inv_slope_2 = (float) (x2 - x0) / abs(y2 - y0);

    if (y1 - y0 != 0) {
        for (int y = y0; y <= y1; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;
            if (x_end < x_start) int_swap(&x_start, &x_end);
            for (int x = x_start; x < x_end; x++) {
                // Draw out pixel with the color that comes from the texture
                draw_texel(x, y, texture, point_a, point_b, point_c, a_uv, b_uv, c_uv);
            }
        }
    }

    // Render the bottom part of the triangle (flat-top)

    inv_slope_1 = 0;
    inv_slope_2 = 0;

    if (y2 - y1 != 0)
        inv_slope_1 = (float) (x2 - x1) / abs(y2 - y1);
    if (y2 - y0 != 0)
        inv_slope_2 = (float) (x2 - x0) / abs(y2 - y0);

    if (y2 - y1 != 0) {
        for (int y = y1; y <= y2; y++) {
            int x_start = x1 + (y - y1) * inv_slope_1;
            int x_end = x0 + (y - y0) * inv_slope_2;
            if (x_end < x_start) int_swap(&x_start, &x_end);
            for (int x = x_start; x < x_end; x++) {
                // Draw out pixel with the color that comes from the texture
                draw_texel(x, y, texture, point_a, point_b, point_c, a_uv, b_uv, c_uv);
            }
        }
    }
}