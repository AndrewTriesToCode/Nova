#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#include "nova_utility.h"

struct TextureMap *CreateTextureMapFromFile(char *file_name)
{
	struct TextureMap *texture = (struct TextureMap *)calloc(1, sizeof(struct TextureMap));
	if (texture == NULL)
		return NULL;

	FILE *file = fopen(file_name, "rb");
	if (file == NULL)
		return NULL;

	char char_buf[2];
	fread(char_buf, 1, 2, file);

	if (char_buf[0] == 'B' && char_buf[1] == 'M')
	{
		fseek(file, 10, SEEK_SET);
		int32_t pixel_offset;
		fread(&pixel_offset, 4, 1, file);

		int32_t dib_header_size;
		fread(&dib_header_size, 4, 1, file);

		fread(&texture->width, 4, 1, file);
		fread(&texture->height, 4, 1, file);

		int16_t bpp;
		fseek(file, 28, SEEK_SET);
		fread(&bpp, sizeof(bpp), 1, file);
		int i = 0;

		int row_padding = (texture->width * bpp / 8) % 4;

		texture->buffer = (int32_t *)malloc(texture->width * texture->height * 4);

		fseek(file, pixel_offset, SEEK_SET);

		for (int y = 0; y < texture->height; y++)
		{
			for (int x = 0; x < texture->width; x++)
			{
				texture->buffer[x + y * texture->height] = 0xffffffff;
				fread(&texture->buffer[x + y * texture->height], 3, 1, file);
			}

			fseek(file, row_padding, SEEK_CUR);
		}
	}

	fclose(file);

	return texture;
}

void DestroyTextureMap(struct TextureMap *texture)
{
	if (texture != NULL)
	{
		if (texture->buffer != NULL)
			free(texture->buffer);

		free(texture);
	}
}

struct Mesh *CreateMeshFromFile(char *file_name)
{
	int v_count = 0;
	int v_alloc = 1;
	struct Vertex *v_buffer = (struct Vertex *)malloc(v_alloc * sizeof(struct Vertex));

	int f_count = 0;
	int f_alloc = 1;
	struct Triangle *f_buffer = (struct Triangle *)malloc(f_alloc * sizeof(struct Triangle));

	int n_count = 0;
	int n_alloc = 1;
	struct Vector *n_buffer = (struct Vector *)malloc(n_alloc * sizeof(struct Vector));

	int uv_count = 0;
	int uv_alloc = 1;
	struct UVCoord *uv_buffer = (struct UVCoord *)malloc(uv_alloc * sizeof(struct UVCoord));

	FILE *file = fopen(file_name, "r");
	if (file == NULL)
		return NULL;

	struct Mesh *mesh = (struct Mesh *)calloc(1, sizeof(struct Mesh));
	if (mesh == NULL)
		return NULL;

	char line_buf[80];

	while (fgets(line_buf, 80, file))
	{
		float x, y, z;
		int v0, v1, v2;
		int n0, n1, n2;
		int t0, t1, t2;

		switch (line_buf[0])
		{
		case 'v':
			switch (line_buf[1])
			{
			case ' ':
				v_count++;

				if (v_count > v_alloc)
				{
					v_alloc *= 2;
					v_buffer = (struct Vertex *)realloc(v_buffer, v_alloc * sizeof(struct Vertex));
				}

				sscanf(line_buf, "v %f %f %f", &x, &y, &z);

				v_buffer[v_count - 1].pos.x = x;
				v_buffer[v_count - 1].pos.y = y;
				v_buffer[v_count - 1].pos.z = z;
				v_buffer[v_count - 1].pos.w = 1.0f;

				break;

			case 'n':
				n_count++;

				if (n_count > n_alloc)
				{
					n_alloc *= 2;
					n_buffer = (struct Vector *)realloc(n_buffer, n_alloc * sizeof(struct Vector));
				}

				sscanf(line_buf, "vn %f %f %f", &x, &y, &z);

				n_buffer[n_count - 1].x = x;
				n_buffer[n_count - 1].y = y;
				n_buffer[n_count - 1].z = z;
				n_buffer[n_count - 1].w = 0.0f;

				VecNormalize(&n_buffer[n_count - 1], &n_buffer[n_count - 1]);

				break;

			case 't':
				uv_count++;

				if (uv_count > uv_alloc)
				{
					uv_alloc *= 2;
					uv_buffer = (struct UVCoord *)realloc(uv_buffer, uv_alloc * sizeof(struct UVCoord));
				}

				sscanf(line_buf, "vt %f %f", &x, &y);

				uv_buffer[uv_count - 1].u = x;
				uv_buffer[uv_count - 1].v = y;
				break;
			}

			break;

		case 'f':
			f_count++;

			if (f_count > f_alloc)
			{
				f_alloc *= 2;
				f_buffer = (struct Triangle *)realloc(f_buffer, f_alloc * sizeof(struct Triangle));
			}

			int match = sscanf(line_buf, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v0, &t0, &n0, &v1, &t1, &n1, &v2, &t2, &n2);
			bool vns = true;
			bool vts = true;

			if (match < 3)
			{
				match = sscanf(line_buf, "f %d//%d %d//%d %d//%d", &v0, &n0, &v1, &n1, &v2, &n2);
				vts = false;
			}

			if (match < 3)
			{
				match = sscanf(line_buf, "f %d %d %d", &v0, &v1, &v2);
				vns = false;
			}

			v0--;
			v1--;
			v2--;

			f_buffer[f_count - 1].v0 = v0;
			f_buffer[f_count - 1].v1 = v1;
			f_buffer[f_count - 1].v2 = v2;

			if (vts)
			{
				t0--;
				t1--;
				t2--;

				f_buffer[f_count - 1].uv0 = t0;
				f_buffer[f_count - 1].uv1 = t1;
				f_buffer[f_count - 1].uv2 = t2;
			}

			if (vns)
			{
				n0--;
				n1--;
				n2--;

				f_buffer[f_count - 1].n0 = n0;
				f_buffer[f_count - 1].n1 = n1;
				f_buffer[f_count - 1].n2 = n2;
			}

			/* Assuming ccw winding. */
			struct Vector vec0, vec1, vec2;
			VecSub(&v_buffer[v1].pos, &v_buffer[v0].pos, &vec0);
			VecSub(&v_buffer[v2].pos, &v_buffer[v0].pos, &vec1);
			VecCross3(&vec0, &vec1, &vec2);
			VecNormalize(&vec2, &f_buffer[f_count - 1].normal);

			break;
		}
	}

	if (v_count > 0)
	{
		mesh->vertices = (struct Vertex *)malloc(v_count * sizeof(struct Vertex));
		mesh->num_vertices = v_count;
		memcpy(mesh->vertices, v_buffer, v_count * sizeof(struct Vertex));
	}
	if (v_buffer != NULL)
		free(v_buffer);

	if (f_count > 0)
	{
		mesh->triangles = (struct Triangle *)malloc(f_count * sizeof(struct Triangle));
		mesh->num_triangles = f_count;
		memcpy(mesh->triangles, f_buffer, f_count * sizeof(struct Triangle));
	}
	if (f_buffer != NULL)
		free(f_buffer);

	if (n_count > 0)
	{
		mesh->normals = (struct Vector *)malloc(n_count * sizeof(struct Vector));
		mesh->num_normals = n_count;
		memcpy(mesh->normals, n_buffer, n_count * sizeof(struct Vector));
	}

	if (n_buffer != NULL)
		free(n_buffer);

	if (uv_count > 0)
	{
		mesh->uvcoords = (struct UVCoord *)malloc(uv_count * sizeof(struct UVCoord));
		mesh->num_uvcoords = uv_count;
		memcpy(mesh->uvcoords, uv_buffer, uv_count * sizeof(struct UVCoord));
	}

	if (uv_buffer != NULL)
		free(uv_buffer);

	fclose(file);

	return mesh;
}

void DestroyMesh(struct Mesh *mesh)
{
	if (mesh != NULL)
	{
		if (mesh->vertices != NULL)
			free(mesh->vertices);

		if (mesh->triangles != NULL)
			free(mesh->triangles);

		if (mesh->normals != NULL)
			free(mesh->normals);

		if (mesh->uvcoords != NULL)
			free(mesh->uvcoords);

		free(mesh);
		mesh = NULL;
	}
}