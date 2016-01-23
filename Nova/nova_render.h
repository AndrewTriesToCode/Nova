#ifndef _NOVA_RENDER_H_
#define _NOVA_RENDER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include "nova_math.h"

#define BYTES_PER_PIXEL 4

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

	void set_screen_size(int width, int height);
	void set_hfov(float fov);
	uint32_t *get_pixel_buffer();
	void clear_pixel_buffer();
	void clear_depth_buffer();
	void set_pixel(int x, int y, uint32_t rgba);
	uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	uint32_t frgba(float r, float g, float b, float a);
	uint32_t sample_texture_map_nearest_neighbor(struct TextureMap *texture_map, float u, float v);
	void render_mesh_scanline(const struct Mesh *mesh, const struct Matrix *mat);
	void render_mesh_bary(const struct Mesh *mesh, const struct Matrix *mat);
	void render_mesh_bary2(const struct Mesh *mesh, const struct Matrix *mat);

#ifdef __cplusplus
}
#endif

#endif
