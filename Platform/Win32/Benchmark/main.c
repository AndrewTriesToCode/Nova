#include "../../../Nova/nova_render.h"
#include "../../../Nova/nova_utility.h"

int main(char **args, int argc)
{
	struct Mesh *mesh;
	mesh = CreateMeshFromFile("../../../models/f16/f16.obj");

	if (mesh != NULL)
		DestroyMesh(mesh);

	return 1;
}