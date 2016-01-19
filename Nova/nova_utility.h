#ifndef _NOVA_UTILITY_H_
#define _NOVA_UTILITY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nova_render.h"

	struct TextureMap *CreateTextureMapFromFile(char *file_name);
	void DestroyTextureMap(struct TextureMap *texture_map);

	struct Mesh *CreateMeshFromFile(char *file_name);
	void DestroyMesh(struct Mesh *mesh);

#ifdef __cplusplus
}
#endif

#endif
