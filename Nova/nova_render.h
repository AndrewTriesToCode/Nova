#ifndef _NOVA_RENDER_H_
#define _NOVA_RENDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "nova_math.h"
#include "nova_geometry.h"

void set_screen_size(int width, int height);
void set_fov(float fov);
uint32_t *get_pixel_buffer();
void clear_pixel_buffer();
extern void(*draw_line)(float x0, float y0, float x1, float y1);
void set_pixel(unsigned int x, unsigned int y, uint32_t rgba);
uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void RenderMesh(const struct Mesh *mesh, const struct Matrix *mat);

#ifdef __cplusplus
}
#endif

#endif