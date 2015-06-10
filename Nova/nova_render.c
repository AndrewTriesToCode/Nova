#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "nova_render.h"
#include "nova_math.h"
#include "nova_utility.h"

static unsigned int screen_width;
static unsigned int screen_height;
static float hfov;
static float vfov;

static struct TextureMap *pixel_buffer;
static struct TextureMap *depth_buffer;

struct Matrix screen_matrix;
struct Matrix projection_matrix;

void set_screen_size(int width, int height)
{
	screen_width = width;
	screen_height = height;

	DestroyTextureMap(pixel_buffer);
	pixel_buffer = malloc(sizeof(struct TextureMap));
	pixel_buffer->width = width;
	pixel_buffer->height = height;
	pixel_buffer->buffer = malloc(width * height * BYTES_PER_PIXEL);

	DestroyTextureMap(depth_buffer);
	depth_buffer = malloc(sizeof(struct TextureMap));
	depth_buffer->width = width;
	depth_buffer->height = height;
	depth_buffer->buffer = malloc(width * height * BYTES_PER_PIXEL);

	MatSetIdentity(&screen_matrix);
	screen_matrix.e[0][0] = width / 2.0f;
	screen_matrix.e[1][1] = -height / 2.0f;
	screen_matrix.e[0][3] = width / 2.0f;
	screen_matrix.e[1][3] = height / 2.0f;
}

void set_hfov(float new_hfov)
{
	hfov = new_hfov;
	vfov = hfov * screen_height / screen_width;
	MatSetPerspective(&projection_matrix, hfov, vfov, 1.0f, 10.0f);
}

uint32_t *get_pixel_buffer()
{
	return pixel_buffer->buffer;
}

void set_pixel(unsigned int x, unsigned int y, uint32_t rgba)
{
#ifdef _DEBUG
	y = max(min(y, screen_height - 1), 0);
#endif

	pixel_buffer->buffer[x + y * screen_width] = rgba;
}

bool set_depth_if_z_is_closer(unsigned int x, unsigned int y, float z)
{
	z = -z;

#ifdef _DEBUG
	y = max(min(y, screen_height - 1), 0);
	z = max(min(z, 1.0f), 0.0f);
#endif

	uint32_t new_z = (uint32_t)(z * UINT32_MAX);

	if (depth_buffer->buffer[x + y * screen_width] > new_z)
	{
		depth_buffer->buffer[x + y * screen_width] = new_z;
		return true;
	}

	return false;
}

void render_mesh(const struct Mesh *mesh, const struct Matrix *mat)
{
	struct Vertex v0, v1, v2;
	struct Vector tranformed_normal;
	struct Matrix final_mat, render_mat;

	struct Triangle *tris = mesh->triangles;
	struct Vertex *verts = mesh->vertices;
	struct UVCoord *uvcoords = mesh->uvcoords;

	MatMul(&projection_matrix, mat, &final_mat);
	MatMul(&screen_matrix, &final_mat, &render_mat);

	struct Vector cull_vec = { 0.0f, 0.0f, -1.0f };

	for (int i = 0; i < mesh->num_triangles; i++)
	{
		MatVecMul(mat, &mesh->triangles[i].normal, &tranformed_normal);

		if (VecDot3(&cull_vec, &tranformed_normal) < 0)
		{
			struct Vector light_vec = { 0.0f, 0.0f, -1.0f };
			VecNormalize(&light_vec, &light_vec);

			VecNormalize(&tranformed_normal, &tranformed_normal);

			MatVecMul(mat, &mesh->normals[mesh->triangles[i].n0], &tranformed_normal);
			v0.light = max(0.2f, -VecDot3(&tranformed_normal, &light_vec));

			MatVecMul(mat, &mesh->normals[mesh->triangles[i].n1], &tranformed_normal);
			v1.light = max(0.2f, -VecDot3(&tranformed_normal, &light_vec));

			MatVecMul(mat, &mesh->normals[mesh->triangles[i].n2], &tranformed_normal);
			v2.light = max(0.2f, -VecDot3(&tranformed_normal, &light_vec));

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

			struct Vertex *v_top, *v_mid, *v_bot, *v_temp;
			v_top = &v0;
			v_mid = &v1;
			v_bot = &v2;

			struct UVCoord *uv_top, *uv_mid, *uv_bot, *uv_temp;
			struct UVCoord uv0 = uvcoords[tris[i].uv0];
			struct UVCoord uv1 = uvcoords[tris[i].uv1];
			struct UVCoord uv2 = uvcoords[tris[i].uv2];

			uv_top = &uv0;
			uv_top->u /= v_top->pos.z;
			uv_top->v /= v_top->pos.z;

			uv_mid = &uv1;
			uv_mid->u /= v_mid->pos.z;
			uv_mid->v /= v_mid->pos.z;

			uv_bot = &uv2;
			uv_bot->u /= v_bot->pos.z;
			uv_bot->v /= v_bot->pos.z;

			if (v_mid->pos.y < v_top->pos.y)
			{
				v_temp = v_top;
				v_top = v_mid;
				v_mid = v_temp;

				uv_temp = uv_top;
				uv_top = uv_mid;
				uv_mid = uv_temp;
			}

			if (v_bot->pos.y < v_mid->pos.y)
			{
				v_temp = v_mid;
				v_mid = v_bot;
				v_bot = v_temp;

				uv_temp = uv_mid;
				uv_mid = uv_bot;
				uv_bot = uv_temp;
			}

			if (v_mid->pos.y < v_top->pos.y)
			{
				v_temp = v_top;
				v_top = v_mid;
				v_mid = v_temp;

				uv_temp = uv_top;
				uv_top = uv_mid;
				uv_mid = uv_temp;
			}

			float long_dx = (v_top->pos.x - v_bot->pos.x) / (v_top->pos.y - v_bot->pos.y);
			float short_dx = (v_top->pos.x - v_mid->pos.x) / (v_top->pos.y - v_mid->pos.y);

			float long_zid = (1.0f / v_top->pos.z - 1.0f / v_bot->pos.z) / (v_top->pos.y - v_bot->pos.y);
			float short_zid = (1.0f / v_top->pos.z - 1.0f / v_mid->pos.z) / (v_top->pos.y - v_mid->pos.y);

			float long_uid = (uv_top->u - uv_bot->u) / (v_top->pos.y - v_bot->pos.y);
			float short_uid = (uv_top->u - uv_mid->u) / (v_top->pos.y - v_mid->pos.y);

			float long_vid = (uv_top->v - uv_bot->v) / (v_top->pos.y - v_bot->pos.y);
			float short_vid = (uv_top->v - uv_mid->v) / (v_top->pos.y - v_mid->pos.y);

			float long_dlight = (v_top->light - v_bot->light) / (v_top->pos.y - v_bot->pos.y);
			float short_dlight = (v_top->light - v_mid->light) / (v_top->pos.y - v_mid->pos.y);

			int y_start = (int)v_top->pos.y + 1;
			int y_end = (int)v_mid->pos.y;

			float xd_start;
			float xd_end;

			float zid_start;
			float zid_end;

			float uid_start;
			float uid_end;

			float vid_start;
			float vid_end;

			float lightd_start;
			float lightd_end;

			if (short_dx < long_dx)
			{
				// Long side of the upper triangle is on the right.

				xd_start = short_dx;
				xd_end = long_dx;

				zid_start = short_zid;
				zid_end = long_zid;

				uid_start = short_uid;
				uid_end = long_uid;

				vid_start = short_vid;
				vid_end = long_vid;

				lightd_start = short_dlight;
				lightd_end = long_dlight;
			}
			else
			{
				// Long side of the upper triangle is on the left.

				xd_start = long_dx;
				xd_end = short_dx;

				zid_start = long_zid;
				zid_end = short_zid;

				uid_start = long_uid;
				uid_end = short_uid;

				vid_start = long_vid;
				vid_end = short_vid;

				lightd_start = long_dlight;
				lightd_end = short_dlight;
			}

			float x_start = v_top->pos.x + (y_start - v_top->pos.y) * xd_start;
			int int_x_start;
			float x_end = v_top->pos.x + (y_start - v_top->pos.y) * xd_end;
			int int_x_end;

			float zi_start = 1.0f / v_top->pos.z + (y_start - v_top->pos.y) * zid_start;
			float zi_end = 1.0f / v_top->pos.z + (y_start - v_top->pos.y) * zid_end;
			float zi, zid;

			float ui_start = uv_top->u + (y_start - v_top->pos.y) * uid_start;
			float ui_end = uv_top->u + (y_start - v_top->pos.y) * uid_end;
			float ui, uid;

			float vi_start = uv_top->v + (y_start - v_top->pos.y) * vid_start;
			float vi_end = uv_top->v + (y_start - v_top->pos.y) * vid_end;
			float vi, vid;

			float light_start = v_top->light + (y_start - v_top->pos.y) * lightd_start;
			float light_end = v_top->light + (y_start - v_top->pos.y) * lightd_end;
			float light, lightd;

			int x, y;
			float z, u, v;

			for (y = y_start; y <= y_end; y++)
			{
				int_x_start = (int)x_start + 1;
				int_x_end = (int)x_end;

				zid = (zi_end - zi_start) / (x_end - x_start);
				zi = zi_start + (int_x_start - x_start) * zid;

				uid = (ui_end - ui_start) / (x_end - x_start);
				ui = ui_start + (int_x_start - x_start) * uid;

				vid = (vi_end - vi_start) / (x_end - x_start);
				vi = vi_start + (int_x_start - x_start) * vid;

				lightd = (light_end - light_start) / (x_end - x_start);
				light = light_start + (int_x_start - x_start) * lightd;

				for (x = int_x_start; x <= int_x_end; x++)
				{
					z = 1.0f / zi;
					u = z * ui;
					v = z * vi;

					if (set_depth_if_z_is_closer(x, y, z))
					{
						uint8_t diffuse[4];
						*((uint32_t *)diffuse) = sample_texture_map_nearest_neighbor(mesh->texture_map, u, v);

						float r, g, b, a;
						r = diffuse[2] / 255.0f;
						g = diffuse[1] / 255.0f;
						b = diffuse[0] / 255.0f;
						a = diffuse[3] / 255.0f;

						set_pixel(x, y, frgba(light * r, light * g, light * b, 1.0f));
					}

					zi += zid;
					ui += uid;
					vi += vid;
					light += lightd;
				}

				x_start += xd_start;
				x_end += xd_end;

				zi_start += zid_start;
				zi_end += zid_end;

				ui_start += uid_start;
				ui_end += uid_end;

				vi_start += vid_start;
				vi_end += vid_end;

				light_start += lightd_start;
				light_end += lightd_end;
			}

			y_start = (int)v_mid->pos.y + 1;
			y_end = (int)v_bot->pos.y;

			short_dx = (v_mid->pos.x - v_bot->pos.x) / (v_mid->pos.y - v_bot->pos.y);
			short_zid = (1.0f / v_mid->pos.z - 1.0f / v_bot->pos.z) / (v_mid->pos.y - v_bot->pos.y);
			short_uid = (uv_mid->u - uv_bot->u) / (v_mid->pos.y - v_bot->pos.y);
			short_vid = (uv_mid->v - uv_bot->v) / (v_mid->pos.y - v_bot->pos.y);
			short_dlight = (v_mid->light - v_bot->light) / (v_mid->pos.y - v_bot->pos.y);

			if (short_dx < long_dx)
			{
				// Long side of the lower triangle is on the left.

				xd_start = long_dx;
				xd_end = short_dx;
				x_start = v_top->pos.x + (y_start - v_top->pos.y) * xd_start;
				x_end = v_mid->pos.x + (y_start - v_mid->pos.y) * xd_end;

				zid_start = long_zid;
				zid_end = short_zid;
				zi_start = 1.0f / v_top->pos.z + (y_start - v_top->pos.y) * zid_start;
				zi_end = 1.0f / v_mid->pos.z + (y_start - v_mid->pos.y) * zid_end;

				uid_start = long_uid;
				uid_end = short_uid;
				ui_start = uv_top->u + (y_start - v_top->pos.y) * uid_start;
				ui_end = uv_mid->u + (y_start - v_mid->pos.y) * uid_end;

				vid_start = long_vid;
				vid_end = short_vid;
				vi_start = uv_top->v + (y_start - v_top->pos.y) * vid_start;
				vi_end = uv_mid->v + (y_start - v_mid->pos.y) * vid_end;

				lightd_start = long_dlight;
				lightd_end = short_dlight;
				light_start = v_top->light + (y_start - v_top->pos.y) * lightd_start;
				light_end = v_mid->light + (y_start - v_mid->pos.y) * lightd_end;
			}
			else
			{
				// Long side of the lower triangle is on the right.

				xd_start = short_dx;
				xd_end = long_dx;
				x_start = v_mid->pos.x + (y_start - v_mid->pos.y) * xd_start;
				x_end = v_top->pos.x + (y_start - v_top->pos.y) * xd_end;

				zid_start = short_zid;
				zid_end = long_zid;
				zi_start = 1.0f / v_mid->pos.z + (y_start - v_mid->pos.y) * zid_start;
				zi_end = 1.0f / v_top->pos.z + (y_start - v_top->pos.y) * zid_end;

				uid_start = short_uid;
				uid_end = long_uid;
				ui_start = uv_mid->u + (y_start - v_mid->pos.y) * uid_start;
				ui_end = uv_top->u + (y_start - v_top->pos.y) * uid_end;

				vid_start = short_vid;
				vid_end = long_vid;
				vi_start = uv_mid->v + (y_start - v_mid->pos.y) * vid_start;
				vi_end = uv_top->v + (y_start - v_top->pos.y) * vid_end;

				lightd_start = short_dlight;
				lightd_end = long_dlight;
				light_start = v_mid->light + (y_start - v_mid->pos.y) * lightd_start;
				light_end = v_top->light + (y_start - v_top->pos.y) * lightd_end;
			}

			for (y = y_start; y <= y_end; y++)
			{
				int_x_start = (int)x_start + 1;
				int_x_end = (int)x_end;

				zid = (zi_end - zi_start) / (x_end - x_start);
				zi = zi_start + (int_x_start - x_start) * zid;

				uid = (ui_end - ui_start) / (x_end - x_start);
				ui = ui_start + (int_x_start - x_start) * uid;

				vid = (vi_end - vi_start) / (x_end - x_start);
				vi = vi_start + (int_x_start - x_start) * vid;

				lightd = (light_end - light_start) / (x_end - x_start);
				light = light_start + (int_x_start - x_start) * lightd;

				for (x = int_x_start; x <= int_x_end; x++)
				{
					z = 1.0f / zi;
					u = z * ui;
					v = z * vi;

					if (set_depth_if_z_is_closer(x, y, z))
					{
						uint8_t diffuse[4];
						*((uint32_t *)diffuse) = sample_texture_map_nearest_neighbor(mesh->texture_map, u, v);
						
						float r, g, b, a;
						r = diffuse[2] / 255.0f;
						g = diffuse[1] / 255.0f;
						b = diffuse[0] / 255.0f;
						a = diffuse[3] / 255.0f;

						set_pixel(x, y, frgba(light * r, light * g, light * b, 1.0f));
					}

					zi += zid;
					ui += uid;
					vi += vid;
					light += lightd;
				}

				x_start += xd_start;
				x_end += xd_end;

				zi_start += zid_start;
				zi_end += zid_end;

				ui_start += uid_start;
				ui_end += uid_end;

				vi_start += vid_start;
				vi_end += vid_end;

				light_start += lightd_start;
				light_end += lightd_end;
			}
		}
	}
}

void clear_pixel_buffer()
{
	memset(pixel_buffer->buffer, rgba(0, 0, 0, 255), screen_width * screen_height * sizeof(uint32_t));
}

void clear_depth_buffer()
{
	memset(depth_buffer->buffer, UINT32_MAX, screen_width * screen_height * sizeof(uint32_t));
}

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (r << 16) | (g << 8) | (b << 0) | (a << 24);
}

uint32_t frgba(float r, float g, float b, float a)
{
#ifdef _DEBUG
	r = max(min(r, 1.0f), 0.0f);
	g = max(min(g, 1.0f), 0.0f);
	b = max(min(b, 1.0f), 0.0f);
	a = max(min(a, 1.0f), 0.0f);
#endif

	return rgba((int)(r * 255 + 0.5f), (int)(g * 255 + 0.5f), (int)(b * 255 + 0.5f), (int)(a * 255 + 0.5f));
}

uint32_t sample_texture_map_nearest_neighbor(struct TextureMap *texture_map, float u, float v)
{
	int x = (int)(u * (texture_map->width - 1) + 0.5f);
	int y = (int)(v * (texture_map->height - 1) + 0.5f);

	return texture_map->buffer[x + y * texture_map->width];
}