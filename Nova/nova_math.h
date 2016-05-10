#ifndef _NOVA_MATH_
#define _NOVA_MATH_

#ifdef __cplusplus
extern "C" {
#endif

#define PI 3.14159265358979323846f
    
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

struct Vector
{
	float x, y, z, w;
};

void VecSet(struct Vector* v, float x, float y, float z, float w);
void VecCopy(const struct Vector *v, struct Vector *d);
void VecAdd(const struct Vector *v1, const struct Vector *v2, struct Vector *r);
void VecSub(const struct Vector *v1, const struct Vector *v2, struct Vector *r);
void VecNormalize(const struct Vector *v, struct Vector *r);
float VecDot3(const struct Vector *v1, const struct Vector *v2);
float VecDot4(const struct Vector *v1, const struct Vector *v2);
void VecPerp2(const struct Vector *v, struct Vector *r);
void VecCross3(const struct Vector *v1, const struct Vector *v2, struct Vector *r);

struct Matrix
{
	float e[4][4];
};

void MatSetIdentity(struct Matrix *m);
void MatSetTranslate(struct Matrix *m, float x, float y, float z);
void MatSetRotX(struct Matrix *m, float a);
void MatSetRotY(struct Matrix *m, float a);
void MatSetRotZ(struct Matrix *m, float a);
void MatSetPerspective(struct Matrix *m, float hFov, float vFov, float n, float f);
void MatCopy(const struct Matrix *m, struct Matrix *d);
void MatVecMul(const struct Matrix *m, const struct Vector *v, struct Vector* r);
void MatMul(const struct Matrix *m1, const struct Matrix *m2, struct Matrix *r);
void MatTranspose(const struct Matrix *m, struct Matrix *r);
void MatTransposeInner(const struct Matrix *m, struct Matrix *r);
void MatInvert(const struct Matrix *m, struct Matrix *r);
void MatInvertRigid(const struct Matrix *m, struct Matrix *r);

#ifdef __cplusplus
}
#endif

#endif
