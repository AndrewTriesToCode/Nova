#ifndef _NOVA_UTILITY_H_
#define _NOVA_UTILITY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nova_render.h"

	struct Mesh *CreateMeshFromFile(char *file_name);
	void DestroyMesh(struct Mesh *mesh);

	void DestroyTextureMap(struct TextureMap *texture);

#ifdef __cplusplus
}
#endif

#endif
