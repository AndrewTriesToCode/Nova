#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "nova_render.h"
#include "nova_math.h"
#include "nova_utility.h"

static void render_mesh_bary_naive(struct RenderContext *context, const struct Mesh *mesh);
static void render_mesh_bary_step(struct RenderContext *context, const struct Mesh *mesh);

static inline void process_pixel_default(struct RenderContext *context, int x, int y, int r, int g, int b, int a, float light);
static inline float calc_2xtri_area(struct Vector *v0, struct Vector *v1, struct Vector *v2);

static inline void set_pixel(struct RenderContext *context, int x, int y, uint32_t rgba);
static inline uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
static inline uint32_t sample_texture_map_nearest_neighbor(struct TextureMap *texture_map, float u, float v);

static inline bool set_depth_if_z_is_closer(struct RenderContext *context, int x, int y, float Z);

void init(struct RenderContext *context)
{
	if (context == NULL)
		return;

	context->vertex_buffer = malloc(MAX_MESH_VERTICES * sizeof(struct Vertex));
	context->vertex_normal_buffer = malloc(MAX_MESH_VERTICES * sizeof(struct Vector));
}

void set_screen_size(struct RenderContext *context, int width, int height)
{
	if (context == NULL)
		return;

	context->screen_width = width;
	context->screen_height = height;

	DestroyTextureMap(context->pixel_buffer);
	context->pixel_buffer = malloc(sizeof(struct TextureMap));
	context->pixel_buffer->width = width;
	context->pixel_buffer->height = height;
	context->pixel_buffer->buffer = malloc(width * height * BYTES_PER_PIXEL);

	DestroyTextureMap(context->depth_buffer);
	context->depth_buffer = malloc(sizeof(struct TextureMap));
	context->depth_buffer->width = width;
	context->depth_buffer->height = height;
	context->depth_buffer->buffer = malloc(width * height * BYTES_PER_PIXEL);

	MatSetIdentity(context->screen_mat);
	context->screen_mat->e[0][0] = width / 2.0f;
	context->screen_mat->e[1][1] = height / 2.0f;
	context->screen_mat->e[0][3] = width / 2.0f;
	context->screen_mat->e[1][3] = height / 2.0f;
}

void set_hfov(struct RenderContext *context, float new_hfov)
{
	if (context == NULL)
		return;

	context->hfov = new_hfov;
	context->vfov = context->hfov * context->screen_height / context->screen_width;
	MatSetPerspective(context->proj_mat, context->hfov, context->vfov, 0.5f, 10.0f);
}

uint32_t *get_pixel_buffer(struct RenderContext *context)
{
	if (context == NULL)
		return NULL;

	return context->pixel_buffer->buffer;
}

void clear_pixel_buffer(struct RenderContext *context)
{
	if (context == NULL)
		return;

	memset(context->pixel_buffer->buffer, rgba(0, 0, 0, 255), context->screen_width * context->screen_height * sizeof(uint32_t));
}

void clear_depth_buffer(struct RenderContext *context)
{
	if (context == NULL)
		return;

	float f = 1000.0f; // arbitrarily chosen highish
	memset(context->depth_buffer->buffer, *(int*)&f, context->screen_width * context->screen_height * sizeof(uint32_t));
}

void render_mesh(struct RenderContext *context, struct Mesh *mesh)
{
	if (context == NULL || mesh == NULL)
		return;
    
	struct Vertex *vertices = mesh->vertices;
	struct Vector *normals = mesh->normals;

	struct Vertex *vertex_buffer = context->vertex_buffer;
	struct Vector *vertex_normal_buffer = context->vertex_normal_buffer;

	// apply the model view matrix
	for (int i = 0; i < mesh->num_vertices; i++)
	{
		MatVecMul(context->mv_mat, &vertices[i].pos, &vertex_buffer[i].pos);
	}

	for (int i = 0; i < mesh->num_normals; i++)
	{
		MatVecMul(context->mv_mat, &normals[i], &vertex_normal_buffer[i]);
	}

	// apply the projection matrix
	for (int i = 0; i < mesh->num_vertices; i++)
	{
		MatVecMul(context->proj_mat, &vertex_buffer[i].pos, &vertex_buffer[i].pos);
	}

	// perform clipping

	// apply the screen matrix
	for (int i = 0; i < mesh->num_vertices; i++)
	{
		MatVecMul(context->screen_mat, &vertex_buffer[i].pos, &vertex_buffer[i].pos);
		vertex_buffer[i].pos.x /= vertex_buffer[i].pos.w;
		vertex_buffer[i].pos.y /= vertex_buffer[i].pos.w;
		vertex_buffer[i].pos.z /= vertex_buffer[i].pos.w;
	}

	// render the mesh
	render_mesh_bary_step(context, mesh);
	//render_mesh_bary_naive(context, mesh);
}

void set_pixel(struct RenderContext *context, int x, int y, uint32_t rgba)
{
	if (context == NULL)
		return;

	context->pixel_buffer->buffer[x + y * context->screen_width] = rgba;
}

void render_mesh_bary_naive(struct RenderContext *context, const struct Mesh *mesh)
{
	if (context == NULL || mesh == NULL)
		return;

	struct Vertex *v0, *v1, *v2;

	struct Triangle *tris = mesh->triangles;
	struct Vertex *verts = context->vertex_buffer;
	struct Vector *normals = context->vertex_normal_buffer;
	struct UVCoord *uvcoords = mesh->uvcoords;
	struct Material *materials = mesh->materials;

	struct Vector light_vec = { 0.0f, 0.0f, -1.0f };

	float t_area;

	for (int i = 0; i < mesh->num_triangles; i++)
	{
		v0 = &verts[tris[i].v0];
		v1 = &verts[tris[i].v1];
		v2 = &verts[tris[i].v2];

		t_area = calc_2xtri_area(&v0->pos, &v1->pos, &v2->pos);

		if (t_area > 0)
		{
			v0->light = max(0.4f, -VecDot3(&normals[tris[i].n0], &light_vec));
			v1->light = max(0.4f, -VecDot3(&normals[tris[i].n1], &light_vec));
			v2->light = max(0.4f, -VecDot3(&normals[tris[i].n2], &light_vec));

			struct UVCoord uv0 = { uvcoords[tris[i].uv0].u * v0->pos.z, uvcoords[tris[i].uv0].v * v0->pos.z };
			struct UVCoord uv1 = { uvcoords[tris[i].uv1].u * v1->pos.z, uvcoords[tris[i].uv1].v * v1->pos.z };
			struct UVCoord uv2 = { uvcoords[tris[i].uv2].u * v2->pos.z, uvcoords[tris[i].uv2].v * v2->pos.z };

			int xmin = max(0, (int)min(min(v0->pos.x, v1->pos.x), v2->pos.x));
			int xmax = min((int)max(max(v0->pos.x, v1->pos.x), v2->pos.x) + 1, context->screen_width - 1);
			int ymin = max(0, (int)min(min(v0->pos.y, v1->pos.y), v2->pos.y));
			int ymax = min((int)max(max(v0->pos.y, v1->pos.y), v2->pos.y) + 1, context->screen_height - 1);

			float t_area_inv = 1.0f / t_area;

			uint8_t diffuse[4] = { 255, 255, 255, 255 };

			for (int y = ymin; y <= ymax; y++)
			{
				for (int x = xmin; x <= xmax; x++)
				{
					struct Vector p = { (float)x, (float)y };

					float w0 = calc_2xtri_area(&v1->pos, &v2->pos, &p);
					w0 *= t_area_inv;

					float w1 = calc_2xtri_area(&v0->pos, &p, &v2->pos);
					w1 *= t_area_inv;

					float w2 = 1.0f - w0 - w1;

					if (w0 > 0.0f && w1 > 0.0f && w2 > 0.0f)
					{
						float Z = v0->pos.z * w0 + v1->pos.z * w1 + v2->pos.z * w2;
						float z = 1.0f / Z;

						if (set_depth_if_z_is_closer(context, x, y, Z))
						{
							float ui = uv0.u * w0 + uv1.u * w1 + uv2.u * w2;
							float u = z * ui;

							float vi = uv0.v * w0 + uv1.v * w1 + uv2.v * w2;
							float v = z * vi;

							float light = v0->light * w0 + v1->light * w1 + v2->light * w2;

							*((uint32_t *)diffuse) = sample_texture_map_nearest_neighbor(materials[tris[i].material].tex_map, u, v);

							process_pixel_default(context, x, y, diffuse[2], diffuse[1], diffuse[0], diffuse[3], light);
						}
					}
				}
			}
		}
	}
}

void render_mesh_bary_step(struct RenderContext *context, const struct Mesh *mesh)
{
	if (context == NULL || mesh == NULL)
		return;

	struct Vertex *v0, *v1, *v2;

	struct Triangle *tris = mesh->triangles;
	struct Vertex *verts = context->vertex_buffer;
	struct Vector *normals = context->vertex_normal_buffer;
	struct UVCoord *uvcoords = mesh->uvcoords;
	struct Material *materials = mesh->materials;

	struct Vector light_vec = { 0.0f, 0.0f, -1.0f };

	float t_area;

	for (int i = 0; i < mesh->num_triangles; i++)
	{
		v0 = &verts[tris[i].v0];
		v1 = &verts[tris[i].v1];
		v2 = &verts[tris[i].v2];

		t_area = calc_2xtri_area(&v0->pos, &v1->pos, &v2->pos);
        
		if (t_area > 0)
		{
            v0->light = max(0.4f, -VecDot3(&normals[tris[i].n0], &light_vec));
			v1->light = max(0.4f, -VecDot3(&normals[tris[i].n1], &light_vec));
			v2->light = max(0.4f, -VecDot3(&normals[tris[i].n2], &light_vec));

			struct UVCoord uv0 = { uvcoords[tris[i].uv0].u * v0->pos.z, uvcoords[tris[i].uv0].v * v0->pos.z };
			struct UVCoord uv1 = { uvcoords[tris[i].uv1].u * v1->pos.z, uvcoords[tris[i].uv1].v * v1->pos.z };
			struct UVCoord uv2 = { uvcoords[tris[i].uv2].u * v2->pos.z, uvcoords[tris[i].uv2].v * v2->pos.z };

			int xmin = max(0, (int)min(min(v0->pos.x, v1->pos.x), v2->pos.x));
			int xmax = min((int)max(max(v0->pos.x, v1->pos.x), v2->pos.x) + 1, context->screen_width - 1);
			int ymin = max(0, (int)min(min(v0->pos.y, v1->pos.y), v2->pos.y));
			int ymax = min((int)max(max(v0->pos.y, v1->pos.y), v2->pos.y) + 1, context->screen_height - 1);

			float t_area_inv = 1.0f / t_area;

			struct Vector p = { (float)xmin, (float)ymin };
            
			float w0 = calc_2xtri_area(&v1->pos, &v2->pos, &p);
			w0 *= t_area_inv;

			float ow0 = w0;
			float w0dx = -(v2->pos.y - v1->pos.y) * t_area_inv;
			float w0dy = (v2->pos.x - v1->pos.x) * t_area_inv;
			float w0ady = 0.0f;

			float w1 = calc_2xtri_area(&v0->pos, &p, &v2->pos);
			w1 *= t_area_inv;
			float ow1 = w1;
			float w1dx = -(v0->pos.y - v2->pos.y) * t_area_inv;
			float w1dy = (v0->pos.x - v2->pos.x) * t_area_inv;
			float w1ady = 0.0f;

			float w2 = 1.0f - w0 - w1;
			float ow2 = w2;
			float w2dx = -(v1->pos.y - v0->pos.y) * t_area_inv;
			float w2dy = (v1->pos.x - v0->pos.x) * t_area_inv;
			float w2ady = 0.0f;

			uint8_t diffuse[4] = { 255, 255, 255, 255 };

			for (int y = ymin; y <= ymax; y++)
			{
				for (int x = xmin; x <= xmax; x++)
				{
					if (w0 > 0.0f && w1 > 0.0f && w2 > 0.0f)
					{
						float Z = v0->pos.z * w0 + v1->pos.z * w1 + v2->pos.z * w2;
						float z = 1.0f / Z;

						if (set_depth_if_z_is_closer(context, x, y, Z))
						{
							float ui = uv0.u * w0 + uv1.u * w1 + uv2.u * w2;
							float u = z * ui;

							float vi = uv0.v * w0 + uv1.v * w1 + uv2.v * w2;
							float v = z * vi;

							float light = v0->light * w0 + v1->light * w1 + v2->light * w2;

							*((uint32_t *)diffuse) = sample_texture_map_nearest_neighbor(materials[tris[i].material].tex_map, u, v);

							process_pixel_default(context, x, y, diffuse[2], diffuse[1], diffuse[0], diffuse[3], light);
						}
					}

					w0 += w0dx;
					w1 += w1dx;
					w2 += w2dx;
				}

				w0ady += w0dy;
				w1ady += w1dy;
				w2ady += w2dy;

				w0 = ow0 + w0ady;
				w1 = ow1 + w1ady;
				w2 = ow2 + w2ady;
			}
		}
	}
}

void process_pixel_default(struct RenderContext *context, int x, int y, int r, int g, int b, int a, float light)
{
	// assumes context is valid

	r = max(0, min((int)(light * r), 255));
	g = max(0, min((int)(light * g), 255));
	b = max(0, min((int)(light * b), 255));
	a = max(0, min((int)(light * a), 255));

	set_pixel(context, x, y, rgba(r, g, b, a));
}

float calc_2xtri_area(struct Vector *v0, struct Vector *v1, struct Vector *v2)
{
	struct Vector v0v1, v0v2;
	VecSub(v1, v0, &v0v1);
	VecSub(v2, v0, &v0v2);

	return v0v1.x * v0v2.y - v0v1.y * v0v2.x;;
}

bool set_depth_if_z_is_closer(struct RenderContext *context, int x, int y, float Z)
{
	if (context == NULL)
		return false;

	float z = *(float*)&(context->depth_buffer->buffer[x + y * context->screen_width]);
	if (z > Z)
	{
		context->depth_buffer->buffer[x + y * context->screen_width] = *(int32_t*)&Z;
		return true;
	}

	return false;
}

uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	return (r << 16) | (g << 8) | (b << 0) | (a << 24);
}

uint32_t sample_texture_map_nearest_neighbor(struct TextureMap *texture_map, float u, float v)
{
	int x = (int)(u * (texture_map->width - 1) + 0.5f);
	int y = (int)(v * (texture_map->height - 1) + 0.5f);

	return texture_map->buffer[x + y * texture_map->width];
}