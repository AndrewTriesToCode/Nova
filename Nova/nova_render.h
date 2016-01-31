#ifndef _NOVA_RENDER_H_
#define _NOVA_RENDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "nova_math.h"

#define BYTES_PER_PIXEL 4
#define MAX_MESH_VERTICES 4096

	struct TextureMap
	{
		int width;
		int height;
		uint32_t *buffer;
	};

	struct Material
	{
		char *name;
		struct TextureMap *tex_map;
		float ambient_rgb[3];
		float diffuse_rgb[3];
		float specular_rgb[3];
	};

	struct Vertex
	{
		struct Vector pos;
		float light;
	};

	struct Triangle
	{
		int v0, v1, v2;
		int n0, n1, n2;
		int uv0, uv1, uv2;
		int material;
		struct Vector normal;
	};

	struct UVCoord
	{
		float u, v;
	};

	struct Mesh
	{
		struct Vertex *vertices;
		int num_vertices;

		struct Triangle *triangles;
		int num_triangles;

		struct Vector *normals;
		int num_normals;

		struct UVCoord *uvcoords;
		int num_uvcoords;

		struct Material *materials;
		int num_materials;
	};

	struct RenderContext
	{
		int screen_width;
		int screen_height;
		float hfov;
		float vfov;

		struct TextureMap *pixel_buffer;
		struct TextureMap *depth_buffer;
		struct Vector *vertex_buffer;
		struct Vector *vertex_normal_buffer;

		struct Matrix *mv_mat;
		struct Matrix *proj_mat;
		struct Matrix *screen_mat;
		struct Matrix *render_mat;
	};

	void init(struct RenderContext *context);
	void set_screen_size(struct RenderContext *context, int width, int height);
	void set_hfov(struct RenderContext *context, float fov);
	uint32_t *get_pixel_buffer(struct RenderContext *context);
	void clear_pixel_buffer(struct RenderContext *context);
	void clear_depth_buffer(struct RenderContext *context);
	void render_mesh(struct RenderContext *context, struct Mesh *mesh);

#ifdef __cplusplus
}
#endif

#endif
