#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <assert.h>
#include <inttypes.h>

#include "nova_render.h"
#include "nova_math.h"
#include "nova_geometry.h"

static unsigned int screen_width;
static unsigned int screen_height;
static uint32_t *pixel_buffer;
static uint32_t *depth_buffer;
static float h_fov;
static float v_fov;
static struct Matrix screen_mat;
static struct Matrix proj_mat;

void set_screen_size(int width, int height)
{
	screen_width = width;
	screen_height = height;

	if (pixel_buffer != NULL)
		free(pixel_buffer);
	pixel_buffer = malloc(screen_width * screen_height * sizeof(uint32_t));

	if (depth_buffer != NULL)
		free(depth_buffer);
	depth_buffer = (uint32_t *)malloc(screen_width * screen_height * sizeof(uint32_t));

	MatSetIdentity(&screen_mat);
	screen_mat.e[0][0] = width / 2.0f;
	screen_mat.e[1][1] = -height / 2.0f;
	screen_mat.e[0][3] = width / 2.0f;
	screen_mat.e[1][3] = height / 2.0f;
}

void set_fov(float fov)
{
	h_fov = fov;
	v_fov = h_fov * screen_height / screen_width;
	MatSetPerspective(&proj_mat, h_fov, v_fov, 1.0f, 10.0f);
}

uint32_t *get_pixel_buffer()
{
	return pixel_buffer;
}

void(*draw_line)(float x0, float y0, float x1, float y1);

void set_pixel(unsigned int x, unsigned int y, uint32_t rgba)
{
	pixel_buffer[x + y * screen_width] = rgba;
}

bool set_depth_if_zgt(unsigned int x, unsigned int y, float z)
{
	assert(z <= 0.0f && z >= -1.0f);

	uint32_t new_z = (uint32_t)(-z * UINT32_MAX);

	if (depth_buffer[x + y * screen_width] > new_z)
	{
		depth_buffer[x + y * screen_width] = new_z;
		return true;
	}

	return false;
}


void RenderMesh(const struct Mesh *mesh, const struct Matrix *mat)
{
	struct Vertex v0, v1, v2;
	struct Vector n;
	struct Matrix final_mat, render_mat;
	struct Tri *tris = mesh->tris;
	struct Vertex *verts = mesh->verts;

	MatMul(&screen_mat, &proj_mat, &final_mat);
	MatMul(&final_mat, mat, &render_mat);

	struct Vector cull_vec = { 0.0f, 0.0f, -1.0f };

	for (int i = 0; i < mesh->num_tris; i++)
	{

		VecAdd(&mesh->normals[mesh->tris[i].n0],
			&mesh->normals[mesh->tris[i].n1],
			&n);
		VecAdd(&n,
			&mesh->normals[mesh->tris[i].n2],
			&n);

		if (VecDot3(&cull_vec, &n) < 0)
		{
			MatVecMul(&render_mat, &verts[tris[i].v0].pos, &v0.pos);
			MatVecMul(&render_mat, &verts[tris[i].v1].pos, &v1.pos);
			MatVecMul(&render_mat, &verts[tris[i].v2].pos, &v2.pos);

			v0.pos.x /= v0.pos.w;
			v0.pos.y /= v0.pos.w;
			//v0.pos.z /= v0.pos.w;

			v1.pos.x /= v1.pos.w;
			v1.pos.y /= v1.pos.w;
			//v1.pos.z /= v1.pos.w;

			v2.pos.x /= v2.pos.w;
			v2.pos.y /= v2.pos.w;
			//v2.pos.z /= v2.pos.w;

			VecNormalize(&n, &n);
			struct Vector light_vec = { -0.5f, 0.0f, -1.0f };
			VecNormalize(&light_vec, &light_vec);
			float light = -VecDot3(&light_vec, &n);
			uint8_t color = (uint8_t)max(64.0f, light * 255.0f);

			struct Vertex *top, *mid, *bot, *temp;
			top = &v0;
			mid = &v1;
			bot = &v2;

			if (mid->pos.y < top->pos.y)
			{
				temp = top;
				top = mid;
				mid = temp;
			}

			if (bot->pos.y < mid->pos.y)
			{
				temp = mid;
				mid = bot;
				bot = temp;
			}

			if (mid->pos.y < top->pos.y)
			{
				temp = top;
				top = mid;
				mid = temp;
			}

			float long_dx = (top->pos.x - bot->pos.x) / (top->pos.y - bot->pos.y);
			float short_dx = (top->pos.x - mid->pos.x) / (top->pos.y - mid->pos.y);

			float long_zid = (1.0f / top->pos.z - 1.0f / bot->pos.z) / (top->pos.y - bot->pos.y);
			float short_zid = (1.0f / top->pos.z - 1.0f / mid->pos.z) / (top->pos.y - mid->pos.y);

			int y_start = (int)(top->pos.y + 0.5f);
			int y_mid = (int)(mid->pos.y);
			int y_end = (int)(bot->pos.y);
			
			float xd_start;
			float xd_end;
			float zid_start;
			float zid_end;

			if (short_dx < long_dx)
			{
				// Long side of the upper triangle is on the right.

				xd_start = short_dx;
				xd_end = long_dx;

				zid_start = short_zid;
				zid_end = long_zid;
			}
			else
			{
				// Long side of the upper triangle is on the left.

				xd_start = long_dx;
				xd_end = short_dx;

				zid_start = long_zid;
				zid_end = short_zid;
			}

			float x_start = top->pos.x + (y_start - top->pos.y) * xd_start;
			int int_x_start;
			float x_end = top->pos.x + (y_start - top->pos.y) * xd_end;
			int int_x_end;

			float zi_start = 1.0f / top->pos.z + (y_start - top->pos.y) * zid_start;
			float zi_end = 1.0f / top->pos.z + (y_start - top->pos.y) * zid_end;
			float zi, zid;
			
			int x, y;

			for (y = y_start; y <= y_mid; y++)
			{
				int_x_start = (int)x_start + 1;
				int_x_end = (int)x_end;

				zid = (zi_end - zi_start) / (x_end - x_start);
				zi = zi_start + (int_x_start - x_start) * zid;

				for (x = int_x_start; x <= int_x_end; x++)
				{
					if (set_depth_if_zgt(x, y, 1.0f / zi))
						set_pixel(x, y, RGBA(color, color, color, 255));

					zi += zid;
				}

				x_start += xd_start;
				x_end += xd_end;

				zi_start += zid_start;
				zi_end += zid_end;
			}

			short_dx = (mid->pos.x - bot->pos.x) / (mid->pos.y - bot->pos.y);
			
			if (short_dx < long_dx)
			{
				// Long side of the lower triangle is on the left.

				xd_start = -long_dx;
				xd_end = -short_dx;

				zid_start = -long_zid;
				zid_end = -short_zid;
			}
			else
			{
				// Long side of the lower triangle is on the right.

				xd_start = -short_dx;
				xd_end = -long_dx;

				zid_start = -short_zid;
				zid_end = -long_zid;
			}

			x_start = bot->pos.x + (bot->pos.y - y_end) * xd_start;
			x_end = bot->pos.x + (bot->pos.y - y_end) * xd_end;

			zi_start = 1.0f / bot->pos.z + (y_start - top->pos.y) * zid_start;
			zi_end = 1.0f / bot->pos.z + (y_start - top->pos.y) * zid_end;

			for (y = y_end; y > y_mid; y--)
			{
				int_x_start = (int)x_start + 1;
				int_x_end = (int)x_end;

				zid = (zi_end - zi_start) / (x_end - x_start);
				zi = zi_start + (int_x_start - x_start) * zid;

				for (x = int_x_start; x <= int_x_end; x++)
				{
					if (set_depth_if_zgt(x, y, 1.0f / zi))
						set_pixel(x, y, RGBA(color, color, color, 255));

					zi += zid;
				}

				x_start += xd_start;
				x_end += xd_end;

				zi_start += zid_start;
				zi_end += zid_end;
			}
		}
	}
}

void clear_pixel_buffer()
{
	assert(pixel_buffer != NULL);
	memset(pixel_buffer, RGBA(0, 0, 0, 255), screen_width * screen_height * sizeof(uint32_t));
}

void clear_depth_buffer()
{
	assert(depth_buffer != NULL);
	memset(depth_buffer, UINT32_MAX, screen_width * screen_height * sizeof(uint32_t));
}

uint32_t RGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (r << 16) | (g << 8) | (b << 0) | (a << 24);
}