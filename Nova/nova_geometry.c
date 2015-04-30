#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <stdbool.h>

#include "nova_geometry.h"


struct Mesh *CreateMeshFromFile(char *file_name)
{
	FILE *file;
	char line_buf[80];
	struct Mesh *mesh;

	int v_count = 0;
	int v_alloc = 1;
	struct Vertex *v_buffer = (struct Vertex *)malloc(v_alloc * sizeof(struct Vertex));

	int f_count = 0;
	int f_alloc = 1;
	struct Tri *f_buffer = (struct Tri *)malloc(f_alloc * sizeof(struct Tri));

	bool calc_normals = true;
	int n_count = 0;
	int n_alloc = 1;
	struct Vector *n_buffer = (struct Vector *)malloc(n_alloc * sizeof(struct Vector));

	file = fopen(file_name, "r");
	if (file == NULL)
		return NULL;

	mesh = (struct Mesh *)calloc(1, sizeof(struct Mesh));
	if (mesh == NULL)
		return NULL;

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
				calc_normals = false;

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
			}

			break;

		case 't':

			break;

		case 'f':
			f_count++;

			if (f_count > f_alloc)
			{
				f_alloc *= 2;
				f_buffer = (struct Tri *)realloc(f_buffer, f_alloc * sizeof(struct Tri));				
			}

			if (calc_normals)
			{
				n_count++;
				if (n_count > n_alloc)
				{
					n_alloc *= 2;
					n_buffer = (struct Vector *)realloc(n_buffer, n_alloc * sizeof(struct Vector));
				}
			}

			int match = sscanf(line_buf, "f %d/%d/%d %d/%d/%d %d/%d/%d", &v0, &t0, &n0, &v1, &t1, &n1, &v2, &t2, &n2);

			if (match < 3)
				match = sscanf(line_buf, "f %d//%d %d//%d %d//%d", &v0, &n0, &v1, &n1, &v2, &n2);

			if (match < 3)
				match = sscanf(line_buf, "f %d %d %d", &v0, &v1, &v2);

			f_buffer[f_count - 1].v0 = v0 - 1;
			f_buffer[f_count - 1].v1 = v1 - 1;
			f_buffer[f_count - 1].v2 = v2 - 1;

			if (calc_normals)
			{
				/* Assuming ccw winding. */
				//Sub(&v_buffer[k].pos, &v_buffer[j].pos, &v1);
				//Sub(&v_buffer[i].pos, &v_buffer[j].pos, &v2);
				//Cross3(&v1, &v2, &n_buffer[n_count - 1]);
				//f_buffer[f_count - 1].n0 = n_count - 1;
				//f_buffer[f_count - 1].n1 = n_count - 1;
				//f_buffer[f_count - 1].n2 = n_count - 1;
				//Normalize(&mesh->tris[mesh->num_tris].n, &mesh->tris[mesh->num_tris].n);
			}
			else
			{
				f_buffer[f_count - 1].n0 = n0 - 1;
				f_buffer[f_count - 1].n1 = n1 - 1;
				f_buffer[f_count - 1].n2 = n2 - 1;
			}

			break;
		}
	}

	if (v_count > 0)
	{
		mesh->verts = (struct Vertex *)malloc(v_count * sizeof(struct Vertex));
		mesh->num_verts = v_count;
		memcpy(mesh->verts, v_buffer, v_count * sizeof(struct Vertex));
	}
	if (v_buffer != NULL)
		free(v_buffer);

	if (f_count > 0)
	{
		mesh->tris = (struct Tri *)malloc(f_count * sizeof(struct Tri));
		mesh->num_tris = f_count;
		memcpy(mesh->tris, f_buffer, f_count * sizeof(struct Tri));
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

	fclose(file);

	return mesh;
}

void DestroyMesh(struct Mesh *mesh)
{
	if (mesh)
	{
		if (mesh->verts)
			free(mesh->verts);

		if (mesh->tris)
			free(mesh->tris);

		if (mesh->normals)
			free(mesh->normals);

		free(mesh);
		mesh = NULL;
	}
}