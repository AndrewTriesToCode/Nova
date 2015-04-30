#ifndef _NOVA_GEOMETRY_
#define _NOVA_GEOMETRY_

#ifdef __cplusplus
extern "C" {
#endif

#include "nova_math.h"

	struct Vertex
	{
		struct Vector pos;
	};

	struct Tri
	{
		int v0, v1, v2;
		int n0, n1, n2;
	};

	struct Mesh
	{
		struct Vertex *verts;
		int num_verts;
		struct Tri *tris;
		int num_tris;
		struct Vector* normals;
		int num_normals;
	};

	struct Mesh *CreateMeshFromFile(char *file_name);
	void DestroyMesh(struct Mesh *mesh);

#ifdef __cplusplus
}
#endif

#endif