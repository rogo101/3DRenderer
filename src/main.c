#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "array.h"
#include "display.h"
#include "vector.h"
#include "matrix.h"
#include "light.h"
#include "triangle.h"
#include "texture.h"
#include "mesh.h"
#include "upng.h"

enum cull_method {
    CULL_NONE,
    CULL_BACKFACE
} cull_method;

enum render_method {
    RENDER_WIRE,
    RENDER_WIRE_VERTEX,
    RENDER_FILL_TRIANGLE,
    RENDER_FILL_TRIANGLE_WIRE,
    RENDER_TEXTURED,
    RENDER_TEXTURED_WIRE
} render_method;

#define MAX_TRIANGLES_PER_MESH 10000
triangle_t triangles_to_render[MAX_TRIANGLES_PER_MESH];
int num_triangles_to_render = 0;

bool is_running = false;
uint32_t previous_frame_time = 0;

vec3_t camera_position = {0, 0, 0};
mat4_t proj_matrix;
light_t light_source = {.direction = {0, 0 , 1}};

bool setup(void) {
    render_method = RENDER_TEXTURED;
    cull_method = CULL_BACKFACE;

    // Allocate the required bytes in memory for the color buffer
    color_buffer = (uint32_t*) malloc(sizeof(uint32_t) * window_width * window_height);
    if (!color_buffer) {
        fprintf(stderr, "Error allocating memory for color_buffer.\n");
        return false;
    }
    // Allocate the required bytes in memory for the z-buffer
    z_buffer = (float*) malloc(sizeof(float) * window_width * window_height);
    if (!color_buffer) {
        fprintf(stderr, "Error allocating memory for z_buffer.\n");
        return false;
    }
    // Creating an SDL texture to display the color buffer
    color_buffer_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        window_width,
        window_height
    );
    if (!color_buffer_texture) {
        fprintf(stderr, "Error creating color_buffer texture.\n");
        return false;
    }

    // Initialize the perspective projection matrix
    float fov = M_PI / 3.0; // 60 deg fov in radians
    float aspect = window_height / (float)window_width;
    float znear = 0.1;
    float zfar = 100.0;
    proj_matrix = mat4_make_perspective(fov, aspect, znear, zfar);

    // Loads the cube mesh data using static cube mesh definiton in mesh.c
    // load_cube_mesh_data();

    // Loads the obj file values in the mesh data structure
    load_obj_file_data("src\\assets\\drone.obj");

    // Translate the points away from the camera
    mesh.translation.z = 5.0;

    // Manually load the hardcoded texture data from static array
    // mesh_texture = (uint32_t*)REDBRICK_TEXTURE;

    // Load the texture information from an external PNG file
    load_png_texture_data("src\\assets\\drone.png");

    return true;
}

void process_input(void) {
    SDL_Event event;
    SDL_PollEvent(&event);

    switch (event.type) {
        case SDL_QUIT: // clicking the x button on top right
            is_running = false;
            break;
        case SDL_KEYDOWN: // pressing a key
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    is_running = false;
                    break;
                case SDLK_1:
                    render_method = RENDER_WIRE_VERTEX;
                    break;
                case SDLK_2:
                    render_method = RENDER_WIRE;
                    break;
                case SDLK_3:
                    render_method = RENDER_FILL_TRIANGLE;
                    break;
                case SDLK_4:
                    render_method = RENDER_FILL_TRIANGLE_WIRE;
                    break;
                case SDLK_5:
                    render_method = RENDER_TEXTURED;
                    break;
                case SDLK_6:
                    render_method = RENDER_TEXTURED_WIRE;
                    break;
                case SDLK_c:
                    cull_method = CULL_BACKFACE;
                    break;
                case SDLK_d:
                    cull_method = CULL_NONE;
                    break;
            }
    }
}

void frame_delay(void) {
    int delay_time = FRAME_TARGET_TIME - (SDL_GetTicks() - previous_frame_time);
    if (delay_time > 0) {
        SDL_Delay(delay_time);
    }
    previous_frame_time = SDL_GetTicks();
}

void update(void) {
    frame_delay();

    // Initialize the counter of triangles to render for every frame
    num_triangles_to_render = 0;

    // Change the mesh scale/rotation/translation values per animation frame
    // mesh.scale.x += 0.001;
    // mesh.scale.y += 0.001;
    // mesh.scale.z += 0.001;
    // mesh.rotation.x += 0.01;
    mesh.rotation.y += 0.01;
    // mesh.rotation.z += 0.01;
    // mesh.translation.x += 0.01;
    // mesh.translation.y += 0.01;
    // mesh.translation.z += 0.01;

    // Create a scale, rotation, and translation matrices that will be used to multiply the mesh vertices
    mat4_t scale_matrix = mat4_make_scale(mesh.scale.x, mesh.scale.y, mesh.scale.z);
    mat4_t rotation_matrix_x = mat4_make_rotation_x(mesh.rotation.x);
    mat4_t rotation_matrix_y = mat4_make_rotation_y(mesh.rotation.y);
    mat4_t rotation_matrix_z = mat4_make_rotation_z(mesh.rotation.z);
    mat4_t translation_matrix = mat4_make_translation(mesh.translation.x, mesh.translation.y, mesh.translation.z);

    // Create a World Matrix combining scale, rotation, and translation matrices
    mat4_t world_matrix = mat4_identity();
    world_matrix = mat4_mul_mat4(scale_matrix, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_x, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_y, world_matrix);
    world_matrix = mat4_mul_mat4(rotation_matrix_z, world_matrix);
    world_matrix = mat4_mul_mat4(translation_matrix, world_matrix);

    // Loop through all the triangle faces of our cube mesh
    int num_faces = array_length(mesh.faces);
    for (int i = 0; i < num_faces; i++) {
        face_t mesh_face = mesh.faces[i];

        vec3_t face_vertices[3] = {
            mesh.vertices[mesh_face.a],
            mesh.vertices[mesh_face.b],
            mesh.vertices[mesh_face.c]
        };
        
        vec4_t transformed_vertices[3];

        // Loop all 3 vertices of this current face and apply transformations
        for (int j = 0; j < 3; j++) {
            vec4_t transformed_vertex = vec4_from_vec3(face_vertices[j]);

            // Use a matrix to scale, rotate, and translate our original vertex
            transformed_vertex = mat4_mul_vec4(world_matrix, transformed_vertex);
            
            //Save transformed vertex in the array of transformed vertices
            transformed_vertices[j] = transformed_vertex;
        }

        // Calculate triangle face normal
        vec3_t vector_a = vec3_from_vec4(transformed_vertices[0]); /*   A   */
        vec3_t vector_b = vec3_from_vec4(transformed_vertices[1]); /*  / \  */ // Triangle is clockwise, hence the order of A, B, and C
        vec3_t vector_c = vec3_from_vec4(transformed_vertices[2]); /* C---B */

        // Get triangle side vectors with respect to A as the origin
        vec3_t vector_ab = vec3_sub(vector_b, vector_a);
        vec3_t vector_ac = vec3_sub(vector_c, vector_a);
        vec3_normalize(&vector_ab);
        vec3_normalize(&vector_ac);

        // Compute the face normal (using cross product to find perpendicular)
        vec3_t normal = vec3_cross(vector_ab, vector_ac); // Note that the order of the cross product matter because our coordinate system is left handed
        vec3_normalize(&normal);

        // Perform backface culling if enabled
        if (cull_method == CULL_BACKFACE) {
            // Compute the camera ray vector (vector pointing from the vertex A directly into the triangle)
            vec3_t camera_ray = vec3_sub(camera_position, vector_a);

            /* Check how aligned is my normal with my camera ray ->
            If the dot product between these two vectors is negative, then this face is not visible from the perspective of the camera so should not be rendered */
            float dot_normal_camera = vec3_dot(normal, camera_ray);
            if (dot_normal_camera < 0) continue;
        }
      
        vec4_t projected_points[3];

        // Loop all 3 transformed vertices for this current face and apply projections
        for (int j = 0; j < 3; j++) {
            projected_points[j] = mat4_mul_vec4_project(proj_matrix, transformed_vertices[j]);

            // Invert the y-values to account for flipped screen y coordinate
            projected_points[j].y *= -1;

            // Scale into view
            projected_points[j].x *= (window_width / 2.0);
            projected_points[j].y *= (window_height / 2.0);

            // Translate the points to the center of the screen
            projected_points[j].x += (window_width / 2.0);
            projected_points[j].y += (window_height / 2.0);
        }

        /* Use light source and face normal to caluclate intensity of triangle color by checking
        how aligned my light source is with face normal by taking their dot product.*/
        float light_intensity_factor = -vec3_dot(normal, light_source.direction);
        uint32_t triangle_color = light_apply_intensity(mesh_face.color, light_intensity_factor);

        /* Calculate average of the z/depth of all three vertices in the face to be used by painter's algorithm
        after the triangles are updated to sort the order the faces will be rendered in (to avoid faces in back showing in front of faces in the front) */
        float avg_depth = (transformed_vertices[0].z + transformed_vertices[1].z + transformed_vertices[2].z) / 3.0;

        triangle_t projected_triangle = {
            .points = {
                {projected_points[0].x, projected_points[0].y, projected_points[0].z, projected_points[0].w},
                {projected_points[1].x, projected_points[1].y, projected_points[1].z, projected_points[1].w},
                {projected_points[2].x, projected_points[2].y, projected_points[2].z, projected_points[2].w}
            },
            .texcoords = {
                {mesh_face.a_uv.u, mesh_face.a_uv.v},
                {mesh_face.b_uv.u, mesh_face.b_uv.v},
                {mesh_face.c_uv.u, mesh_face.c_uv.v}
            },
            .color = triangle_color,
            .avg_depth = avg_depth
        };

        // Save the projected triangle in the array of triangles to render
        if (num_triangles_to_render < MAX_TRIANGLES_PER_MESH) {
            triangles_to_render[num_triangles_to_render++] = projected_triangle;
        }
    }

    // // Painter's algorithm:
    // // Sort the triangles to render by their average depth using bubble sort
    // // TODO:: Convert this sorting into merge_sort for better runtime complexity
    // int num_triangles = array_length(triangles_to_render);
    // for (int i = 0; i < num_triangles; i++) {
    //     for (int j = i; j < num_triangles; j++) {
    //         if (triangles_to_render[i].avg_depth < triangles_to_render[j].avg_depth) {
    //             triangle_t temp = triangles_to_render[i];
    //             triangles_to_render[i] = triangles_to_render[j];
    //             triangles_to_render[j] = temp;
    //         }
    //     }
    // }
}

void render(void) {
    draw_grid(10, 1, 0xFFD3D3D3, false); // lightgrey grid

    for (int i = 0; i < num_triangles_to_render; i++) {
            triangle_t triangle = triangles_to_render[i];

            // Draw vertex points as rectangles with width point_scale if enabled
            if (render_method == RENDER_WIRE_VERTEX) {
                int point_scale = 4;
                draw_rectangle(
                    triangle.points[0].x - point_scale/2,
                    triangle.points[0].y - point_scale/2,
                    point_scale,
                    point_scale,
                    0xFFFF0000 // red projected points
                );
                draw_rectangle(
                    triangle.points[1].x - point_scale/2,
                    triangle.points[1].y - point_scale/2,
                    point_scale,
                    point_scale,
                    0xFFFF0000 // red projected points
                );
                draw_rectangle(
                    triangle.points[2].x - point_scale/2,
                    triangle.points[2].y - point_scale/2,
                    point_scale,
                    point_scale,
                    0xFFFF0000 // red projected points
                );
            }

            // Draw filled triangles if enabled
            if (render_method == RENDER_FILL_TRIANGLE || render_method == RENDER_FILL_TRIANGLE_WIRE) {
                draw_filled_triangle(
                    triangle.points[0].x, triangle.points[0].y, triangle.points[0].w,
                    triangle.points[1].x, triangle.points[1].y, triangle.points[1].w,
                    triangle.points[2].x, triangle.points[2].y, triangle.points[2].w,
                    triangle.color
                );
            }

            // Draw textured triangles if enabled
            if (render_method == RENDER_TEXTURED || render_method == RENDER_TEXTURED_WIRE) {
                draw_textured_triangle(
                    triangle.points[0].x, triangle.points[0].y, triangle.points[0].z, triangle.points[0].w, triangle.texcoords[0].u, triangle.texcoords[0].v, // vertex A
                    triangle.points[1].x, triangle.points[1].y, triangle.points[1].z, triangle.points[1].w, triangle.texcoords[1].u, triangle.texcoords[1].v, // vertex B
                    triangle.points[2].x, triangle.points[2].y, triangle.points[2].z, triangle.points[2].w, triangle.texcoords[2].u, triangle.texcoords[2].v, // vertex C
                    mesh_texture
                );
            }

            // Draw wireframe = unfilled triangles if enabled
            if (render_method != RENDER_FILL_TRIANGLE && render_method != RENDER_TEXTURED) {
                draw_unfilled_triangle(
                    triangle.points[0].x,
                    triangle.points[0].y,
                    triangle.points[1].x,
                    triangle.points[1].y,
                    triangle.points[2].x,
                    triangle.points[2].y,
                    0xFFFFFFFF // white lines for the wireframe
                );
            }
    }

    if (!render_color_buffer()) {
        is_running = false;
        return;
    }

    clear_color_buffer(0xFF000000); // black background
    clear_z_buffer();

    SDL_RenderPresent(renderer);
}

// Free the memory that was dynamically allocated by the program
void free_resources(void) {
    free(color_buffer);
    free(z_buffer);
    array_free(mesh.vertices);
    array_free(mesh.faces);
    upng_free(png_texture);
}

int main(int argc, char* args[]) {
    is_running = initialize_window();
    is_running = setup();

    int num_frames_rendered = 0;
    while (is_running) {
        process_input();
        update();
        render();
        num_frames_rendered += 1;
    }
    printf("Actual FPS: %.2f", num_frames_rendered / (SDL_GetTicks() / 1000.0));

    destroy_window();
    free_resources();

    return 0;
}