#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "nova_render.h"
#include "nova_math.h"
#include "nova_utility.h"

static int screen_width;
static int screen_height;
static float hfov;
static float vfov;

static struct TextureMap *pixel_buffer;
static struct TextureMap *depth_buffer;

struct Matrix screen_matrix;
struct Matrix projection_matrix;

static float calc_2xtri_area(struct Vector *v1, struct Vector *v2);
static void raster_scanline(struct Vertex *v0, struct Vertex *v1, struct Vertex *v2, struct UVCoord *uv0, struct UVCoord *uv1, struct UVCoord *uv2, struct Material *material);
static void process_pixel_default(int x, int y, int r, int g, int b, int a, float light);

static bool set_depth_if_z_is_closer(int x, int y, float Z);

#define MAX_NUM_VERTS 8192
static struct Vector vert_buffer[MAX_NUM_VERTS];

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
	MatSetPerspective(&projection_matrix, hfov, vfov, 0.5f, 10.0f);
}

uint32_t *get_pixel_buffer()
{
	return pixel_buffer->buffer;
}

void set_pixel(int x, int y, uint32_t rgba)
{
	pixel_buffer->buffer[x + y * screen_width] = rgba;
}

void render_mesh_scanline(const struct Mesh *mesh, const struct Matrix *mat)
{
	struct Vertex v0, v1, v2;
	struct UVCoord uv0, uv1, uv2;
	struct Vector tranformed_normal;
	struct Matrix final_mat, render_mat;

	struct Triangle *tris = mesh->triangles;
	struct Vertex *verts = mesh->vertices;
	struct UVCoord *uvcoords = mesh->uvcoords;
	struct Material *materials = mesh->materials;

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
			v0.pos.z /= v0.pos.w;

			v1.pos.x /= v1.pos.w;
			v1.pos.y /= v1.pos.w;
			v1.pos.z /= v1.pos.w;

			v2.pos.x /= v2.pos.w;
			v2.pos.y /= v2.pos.w;
			v2.pos.z /= v2.pos.w;

			uv0.u = uvcoords[tris[i].uv0].u * v0.pos.z;
			uv0.v = uvcoords[tris[i].uv0].v * v0.pos.z;

			uv1.u = uvcoords[tris[i].uv1].u * v1.pos.z;
			uv1.v = uvcoords[tris[i].uv1].v * v1.pos.z;

			uv2.u = uvcoords[tris[i].uv2].u * v2.pos.z;
			uv2.v = uvcoords[tris[i].uv2].v * v2.pos.z;

			raster_scanline(&v0, &v1, &v2, &uv0, &uv1, &uv2, &materials[tris[i].material]);
		}
	}
}

void raster_scanline(struct Vertex *v0, struct Vertex *v1, struct Vertex *v2, struct UVCoord *uv0, struct UVCoord *uv1, struct UVCoord *uv2, struct Material *material)
{
	struct Vertex *v_top, *v_mid, *v_bot, *v_temp;
	v_top = v0;
	v_mid = v1;
	v_bot = v2;

	struct UVCoord *uv_top, *uv_mid, *uv_bot, *uv_temp;

	uv_top = uv0;
	uv_mid = uv1;
	uv_bot = uv2;

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

	float long_zid = (v_top->pos.z - v_bot->pos.z) / (v_top->pos.y - v_bot->pos.y);
	float short_zid = (v_top->pos.z - v_mid->pos.z) / (v_top->pos.y - v_mid->pos.y);

	float long_uid = (uv_top->u - uv_bot->u) / (v_top->pos.y - v_bot->pos.y);
	float short_uid = (uv_top->u - uv_mid->u) / (v_top->pos.y - v_mid->pos.y);

	float long_vid = (uv_top->v - uv_bot->v) / (v_top->pos.y - v_bot->pos.y);
	float short_vid = (uv_top->v - uv_mid->v) / (v_top->pos.y - v_mid->pos.y);

	float long_dlight = (v_top->light - v_bot->light) / (v_top->pos.y - v_bot->pos.y);
	float short_dlight = (v_top->light - v_mid->light) / (v_top->pos.y - v_mid->pos.y);

	int y_start = max(0, (int)v_top->pos.y + 1);
	int y_end = min((int)v_mid->pos.y, screen_height - 1);

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

	float zi_start = v_top->pos.z + (y_start - v_top->pos.y) * zid_start;
	float zi_end = v_top->pos.z + (y_start - v_top->pos.y) * zid_end;
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
		int_x_start = max(0, (int)x_start + 1);
		int_x_end = min(((int)x_end), screen_width - 1);

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

			if (set_depth_if_z_is_closer(x, y, zi))
			{
				uint8_t diffuse[4] = { 255, 255, 255, 255 };
				//*((uint32_t *)diffuse) = sample_texture_map_nearest_neighbor(tm, u, v);

				process_pixel_default(x, y, diffuse[2], diffuse[1], diffuse[0], diffuse[3], light);
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

	y_start = max(0, (int)v_mid->pos.y + 1);
	y_end = min((int)v_bot->pos.y, screen_height - 1);

	short_dx = (v_mid->pos.x - v_bot->pos.x) / (v_mid->pos.y - v_bot->pos.y);
	short_zid = (v_mid->pos.z - v_bot->pos.z) / (v_mid->pos.y - v_bot->pos.y);
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
		zi_start = v_top->pos.z + (y_start - v_top->pos.y) * zid_start;
		zi_end = v_mid->pos.z + (y_start - v_mid->pos.y) * zid_end;

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
		zi_start = v_mid->pos.z + (y_start - v_mid->pos.y) * zid_start;
		zi_end = v_top->pos.z + (y_start - v_top->pos.y) * zid_end;

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
		int_x_start = max(0, (int)x_start + 1);
		int_x_end = min(((int)x_end), screen_width - 1);

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

			if (set_depth_if_z_is_closer(x, y, zi))
			{
				uint8_t diffuse[4] = { 255, 255, 255, 255 };
				//*((uint32_t *)diffuse) = sample_texture_map_nearest_neighbor(, u, v);

				process_pixel_default(x, y, diffuse[2], diffuse[1], diffuse[0], diffuse[3], light);
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

void render_mesh_bary(const struct Mesh *mesh, const struct Matrix *mat)
{
	struct Vertex v0, v1, v2;
	struct Vector tranformed_normal;
	struct Matrix final_mat, render_mat;

	struct Triangle *tris = mesh->triangles;
	struct Vertex *verts = mesh->vertices;
	struct UVCoord *uvcoords = mesh->uvcoords;
	struct Material *materials = mesh->materials;

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
			v0.light = max(0.4f, -VecDot3(&tranformed_normal, &light_vec));

			MatVecMul(mat, &mesh->normals[mesh->triangles[i].n1], &tranformed_normal);
			v1.light = max(0.4f, -VecDot3(&tranformed_normal, &light_vec));

			MatVecMul(mat, &mesh->normals[mesh->triangles[i].n2], &tranformed_normal);
			v2.light = max(0.4f, -VecDot3(&tranformed_normal, &light_vec));

			MatVecMul(&render_mat, &verts[tris[i].v0].pos, &v0.pos);
			MatVecMul(&render_mat, &verts[tris[i].v1].pos, &v1.pos);
			MatVecMul(&render_mat, &verts[tris[i].v2].pos, &v2.pos);

			v0.pos.x /= v0.pos.w;
			v0.pos.y /= v0.pos.w;
			v0.pos.z /= v0.pos.w;

			v1.pos.x /= v1.pos.w;
			v1.pos.y /= v1.pos.w;
			v1.pos.z /= v1.pos.w;

			v2.pos.x /= v2.pos.w;
			v2.pos.y /= v2.pos.w;
			v2.pos.z /= v2.pos.w;

			struct UVCoord uv0 = { uvcoords[tris[i].uv0].u * v0.pos.z, uvcoords[tris[i].uv0].v * v0.pos.z };
			struct UVCoord uv1 = { uvcoords[tris[i].uv1].u * v1.pos.z, uvcoords[tris[i].uv1].v * v1.pos.z };
			struct UVCoord uv2 = { uvcoords[tris[i].uv2].u * v2.pos.z, uvcoords[tris[i].uv2].v * v2.pos.z };

			int xmin = max(0, (int)min(min(v0.pos.x, v1.pos.x), v2.pos.x));
			int xmax = min((int)max(max(v0.pos.x, v1.pos.x), v2.pos.x) + 1, screen_width - 1);
			int ymin = max(0, (int)min(min(v0.pos.y, v1.pos.y), v2.pos.y));
			int ymax = min((int)max(max(v0.pos.y, v1.pos.y), v2.pos.y) + 1, screen_height - 1);

			struct Vector v0v1;
			VecSub(&v1.pos, &v0.pos, &v0v1);

			struct Vector v1v2;
			VecSub(&v2.pos, &v1.pos, &v1v2);

			struct Vector v2v0;
			VecSub(&v0.pos, &v2.pos, &v2v0);

			float t_area = -calc_2xtri_area(&v0v1, &v2v0);

			struct Vector p_vec;

			for (int y = ymin; y <= ymax; y++)
				for (int x = xmin; x <= xmax; x++)
				{
					struct Vector p = { (float)x, (float)y };

					VecSub(&p, &v1, &p_vec);
					float w0 = calc_2xtri_area(&v1v2, &p_vec);
					w0 /= t_area;

					VecSub(&p, &v2, &p_vec);
					float w1 = calc_2xtri_area(&v2v0, &p_vec);
					w1 /= t_area;

					VecSub(&p, &v0, &p_vec);
					float w2 = calc_2xtri_area(&v0v1, &p_vec);
					w2 /= t_area;

					if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
					{
						float Z = v0.pos.z * w0 + v1.pos.z * w1 + v2.pos.z * w2;
						float z = 1.0f / Z;

						if (set_depth_if_z_is_closer(x, y, Z))
						{
							float ui = uv0.u * w0 + uv1.u * w1 + uv2.u * w2;
							float u = z * ui;

							float vi = uv0.v * w0 + uv1.v * w1 + uv2.v * w2;
							float v = z * vi;

							float light = v0.light * w0 + v1.light * w1 + v2.light * w2;

							uint8_t diffuse[4] = { 255, 255, 255, 255 };
							*((uint32_t *)diffuse) = sample_texture_map_nearest_neighbor(materials[tris[i].material].tex_map, u, v);

							process_pixel_default(x, y, diffuse[2], diffuse[1], diffuse[0], diffuse[3], light);
						}
					}
				}
		}
	}
}

void process_pixel_default(int x, int y, int r, int g, int b, int a, float light)
{
	r = max(0, min((int)(light * r), 255));
	g = max(0, min((int)(light * g), 255));
	b = max(0, min((int)(light * b), 255));
	a = max(0, min((int)(light * a), 255));

	set_pixel(x, y, rgba(r, g, b, a));
}

void render_mesh_bary2(const struct Mesh *mesh, const struct Matrix *mat)
{
	struct Vertex v0, v1, v2;
	struct Vector tranformed_normal;
	struct Matrix final_mat, render_mat;

	struct Triangle *tris = mesh->triangles;
	struct Vertex *verts = mesh->vertices;
	struct UVCoord *uvcoords = mesh->uvcoords;
	struct Material *materials = mesh->materials;

	MatMul(&projection_matrix, mat, &final_mat);
	MatMul(&screen_matrix, &final_mat, &render_mat);

	struct Vector cull_vec = { 0.0f, 0.0f, -1.0f };

	for (int i = 0; i < mesh->num_vertices; i++)
	{
		MatVecMul(&render_mat, &verts[i].pos, &vert_buffer[i]);
	}

	for (int i = 0; i < mesh->num_triangles; i++)
	{
		MatVecMul(mat, &mesh->triangles[i].normal, &tranformed_normal);

		if (tranformed_normal.z > 0)
		{
			struct Vector light_vec = { 0.0f, 0.0f, -1.0f };
			VecNormalize(&light_vec, &light_vec);

			VecNormalize(&tranformed_normal, &tranformed_normal);

			MatVecMul(mat, &mesh->normals[mesh->triangles[i].n0], &tranformed_normal);
			v0.light = max(0.4f, -VecDot3(&tranformed_normal, &light_vec));

			MatVecMul(mat, &mesh->normals[mesh->triangles[i].n1], &tranformed_normal);
			v1.light = max(0.4f, -VecDot3(&tranformed_normal, &light_vec));

			MatVecMul(mat, &mesh->normals[mesh->triangles[i].n2], &tranformed_normal);
			v2.light = max(0.4f, -VecDot3(&tranformed_normal, &light_vec));

			v0.pos = vert_buffer[mesh->triangles[i].v0];
			v1.pos = vert_buffer[mesh->triangles[i].v1];
			v2.pos = vert_buffer[mesh->triangles[i].v2];

			v0.pos.x /= v0.pos.w;
			v0.pos.y /= v0.pos.w;
			v0.pos.z /= v0.pos.w;

			v1.pos.x /= v1.pos.w;
			v1.pos.y /= v1.pos.w;
			v1.pos.z /= v1.pos.w;

			v2.pos.x /= v2.pos.w;
			v2.pos.y /= v2.pos.w;
			v2.pos.z /= v2.pos.w;

			struct UVCoord uv0 = { uvcoords[tris[i].uv0].u / v0.pos.z, uvcoords[tris[i].uv0].v / v0.pos.z };
			struct UVCoord uv1 = { uvcoords[tris[i].uv1].u / v1.pos.z, uvcoords[tris[i].uv1].v / v1.pos.z };
			struct UVCoord uv2 = { uvcoords[tris[i].uv2].u / v2.pos.z, uvcoords[tris[i].uv2].v / v2.pos.z };

			int xmin = max(0, (int)min(min(v0.pos.x, v1.pos.x), v2.pos.x));
			int xmax = min((int)max(max(v0.pos.x, v1.pos.x), v2.pos.x) + 1, screen_width - 1);
			int ymin = max(0, (int)min(min(v0.pos.y, v1.pos.y), v2.pos.y));
			int ymax = min((int)max(max(v0.pos.y, v1.pos.y), v2.pos.y) + 1, screen_height - 1);

			struct Vector v0v1;
			VecSub(&v1.pos, &v0.pos, &v0v1);

			struct Vector v1v2;
			VecSub(&v2.pos, &v1.pos, &v1v2);

			struct Vector v2v0;
			VecSub(&v0.pos, &v2.pos, &v2v0);

			float t_area = -calc_2xtri_area(&v2v0, &v1v2);

			struct Vector p_vec;

			for (int y = ymin; y <= ymax; y++)
				for (int x = xmin; x <= xmax; x++)
				{
					struct Vector p = { (float)x, (float)y };

					VecSub(&p, &v1, &p_vec);
					float w0 = calc_2xtri_area(&v1v2, &p_vec);
					w0 /= t_area;

					VecSub(&p, &v2, &p_vec);
					float w1 = calc_2xtri_area(&v2v0, &p_vec);
					w1 /= t_area;

					float w2 = 1 - w0 - w1;

					if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
					{
						float Z = v0.pos.z * w0 + v1.pos.z * w1 + v2.pos.z * w2;

						if (set_depth_if_z_is_closer(x, y, Z))
						{
							float ui = uv0.u * w0 + uv1.u * w1 + uv2.u * w2;
							float u = Z * ui;
							//u = min(u, 1.0f);

							float vi = uv0.v * w0 + uv1.v * w1 + uv2.v * w2;
							float v = Z * vi;
							//v = min(v, 1.0f);

							float light = v0.light * w0 + v1.light * w1 + v2.light * w2;

							uint8_t diffuse[4] = { 255, 255, 255, 255 };
							*((uint32_t *)diffuse) = sample_texture_map_nearest_neighbor(materials[tris[i].material].tex_map, u, v);

							uint8_t r, g, b, a;
							r = (int)(light * diffuse[2] + 0.5f);
							g = (int)(light * diffuse[1] + 0.5f);
							b = (int)(light * diffuse[0] + 0.5f);
							a = diffuse[3];

							set_pixel(x, y, rgba(r, g, b, a));
						}
					}
				}
		}
	}
}

float calc_2xtri_area(struct Vector *v1, struct Vector *v2)
{
	float area = v1->x * v2->y - v1->y * v2->x;
	return area;
}

static bool set_depth_if_z_is_closer(int x, int y, float Z)
{
	float current_z = *(float*)&(depth_buffer->buffer[x + y * screen_width]);
	if (current_z > Z)
	{
		depth_buffer->buffer[x + y * screen_width] = *(int32_t*)&Z;
		return true;
	}

	return false;
}

void clear_pixel_buffer()
{
	memset(pixel_buffer->buffer, rgba(0, 0, 0, 255), screen_width * screen_height * sizeof(uint32_t));
}

void clear_depth_buffer()
{
	float f = 1.0f;
	memset(depth_buffer->buffer, *(int*)&f, screen_width * screen_height * sizeof(uint32_t));
}

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (r << 16) | (g << 8) | (b << 0) | (a << 24);
}

uint32_t frgba(float r, float g, float b, float a)
{
	return rgba((int)(r * 255 + 0.5f), (int)(g * 255 + 0.5f), (int)(b * 255 + 0.5f), (int)(a * 255 + 0.5f));
}

uint32_t sample_texture_map_nearest_neighbor(struct TextureMap *texture_map, float u, float v)
{
	int x = (int)(u * (texture_map->width - 1) + 0.5f);
	int y = (int)(v * (texture_map->height - 1) + 0.5f);

	return texture_map->buffer[x + y * texture_map->width];
}