#include <math.h>
#include <memory.h>

#include "nova_math.h"

void VecSet(struct Vector *v, float x, float y, float z, float w)
{
	v->x = x;
	v->y = y;
	v->z = z;
	v->w = w;
}

void VecCopy(const struct Vector *v, struct Vector *d)
{
	memcpy(d, v, sizeof(struct Vector));
}

void VecAdd(const struct Vector *v1, const struct Vector *v2, struct Vector *r)
{
	r->x = v1->x + v2->x;
	r->y = v1->y + v2->y;
	r->z = v1->z + v2->z;
	r->w = v1->w + v2->w;
}

void VecSub(const struct Vector *v1, const struct Vector *v2, struct Vector *r)
{
	r->x = v1->x - v2->x;
	r->y = v1->y - v2->y;
	r->z = v1->z - v2->z;
	r->w = v1->w - v2->w;
}

void VecNormalize(const struct Vector *v, struct Vector *r)
{
	float len = sqrtf(VecDot4(v, v));
	r->x = v->x / len;
	r->y = v->y / len;
	r->z = v->z / len;
	r->w = v->w / len;
}

float VecDot3(const struct Vector *v1, const struct Vector *v2)
{
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

float VecDot4(const struct Vector *v1, const struct Vector *v2)
{
	return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z + v1->w * v2->w;
}

void VecPerp2(const struct Vector *v, struct Vector *r)
{
	r->x = -v->y;
	r->y = v->x;
	r->z = 0.0f;
	r->w = 0.0f;
}

void VecCross3(const struct Vector *v1, const struct Vector *v2, struct Vector *r)
{
	r->x = v1->y * v2->z - v1->z * v2->y;
	r->y = v1->z * v2->x - v1->x * v2->z;
	r->z = v1->x * v2->y - v1->y * v2->x;
	r->w = 0.0f;
}

void MatSetIdentity(struct Matrix *m)
{
	m->e[0][0] = 1.0f;	m->e[0][1] = 0.0f;	m->e[0][2] = 0.0f;	m->e[0][3] = 0.0f;
	m->e[1][0] = 0.0f;	m->e[1][1] = 1.0f;	m->e[1][2] = 0.0f;	m->e[1][3] = 0.0f;
	m->e[2][0] = 0.0f;	m->e[2][1] = 0.0f;	m->e[2][2] = 1.0f;	m->e[2][3] = 0.0f;
	m->e[3][0] = 0.0f;	m->e[3][1] = 0.0f;	m->e[3][2] = 0.0f;	m->e[3][3] = 1.0f;
}

void MatSetTranslate(struct Matrix *m, float x, float y, float z)
{
	m->e[0][0] = 1.0f;	m->e[0][1] = 0.0f;	m->e[0][2] = 0.0f;	m->e[0][3] = x;
	m->e[1][0] = 0.0f;	m->e[1][1] = 1.0f;	m->e[1][2] = 0.0f;	m->e[1][3] = y;
	m->e[2][0] = 0.0f;	m->e[2][1] = 0.0f;	m->e[2][2] = 1.0f;	m->e[2][3] = z;
	m->e[3][0] = 0.0f;	m->e[3][1] = 0.0f;	m->e[3][2] = 0.0f;	m->e[3][3] = 1.0f;
}

void MatSetRotX(struct Matrix *m, float a)
{
	m->e[0][0] = 1.0f;	m->e[0][1] = 0.0f;		m->e[0][2] = 0.0f;      m->e[0][3] = 0.0f;
	m->e[1][0] = 0.0f;	m->e[1][1] = cosf(a);	m->e[1][2] = -sinf(a);  m->e[1][3] = 0.0f;
	m->e[2][0] = 0.0f;	m->e[2][1] = sinf(a);	m->e[2][2] = cosf(a);   m->e[2][3] = 0.0f;
	m->e[3][0] = 0.0f;	m->e[3][1] = 0.0f;		m->e[3][2] = 0.0f;      m->e[3][3] = 1.0f;
}

void MatSetRotY(struct Matrix *m, float a)
{
	m->e[0][0] = cosf(a);   m->e[0][1] = 0.0f;	m->e[0][2] = sinf(a);	m->e[0][3] = 0.0f;
	m->e[1][0] = 0.0f;      m->e[1][1] = 1.0f;	m->e[1][2] = 0.0f;		m->e[1][3] = 0.0f;
	m->e[2][0] = -sinf(a);	m->e[2][1] = 0.0f;	m->e[2][2] = cosf(a);	m->e[2][3] = 0.0f;
	m->e[3][0] = 0.0f;  	m->e[3][1] = 0.0f;	m->e[3][2] = 0.0f;		m->e[3][3] = 1.0f;
}

void MatSetRotZ(struct Matrix *m, float a)
{
	m->e[0][0] = cosf(a);	m->e[0][1] = -sinf(a);	m->e[0][2] = 0.0f;	m->e[0][3] = 0.0f;
	m->e[1][0] = sinf(a);	m->e[1][1] = cosf(a);	  m->e[1][2] = 0.0f;	m->e[1][3] = 0.0f;
	m->e[2][0] = 0.0f;		m->e[2][1] = 0.0f;		  m->e[2][2] = 1.0f;	m->e[2][3] = 0.0f;
	m->e[3][0] = 0.0f;		m->e[3][1] = 0.0f;		  m->e[3][2] = 0.0f;	m->e[3][3] = 1.0f;
}

void MatSetPerspective(struct Matrix *m, float hFov, float vFov, float n, float f)
{
	float r = n * tanf(hFov * PI / 360.0f);
	float t = n * tanf(vFov * PI / 360.0f);

	m->e[0][0] = n / r;	m->e[0][1] = 0.0f;  m->e[0][2] = 0.0f;          m->e[0][3] = 0.0f;
	m->e[1][0] = 0.0f;	m->e[1][1] = n / t; m->e[1][2] = 0.0f;          m->e[1][3] = 0.0f;
	m->e[2][0] = 0.0f;	m->e[2][1] = 0.0f;	m->e[2][2] = 1 / (f - n);   m->e[2][3] = -n / (f - n);
	m->e[3][0] = 0.0f;	m->e[3][1] = 0.0f;	m->e[3][2] = -1.0f;         m->e[3][3] = 0.0f;
}

void MatCopy(const struct Matrix *m, struct Matrix *d)
{
	memcpy(d, m, sizeof(struct Matrix));
}

void MatVecMul(const struct Matrix *m, const struct Vector* v, struct Vector* r)
{
	r->x = m->e[0][0] * v->x + m->e[0][1] * v->y + m->e[0][2] * v->z + m->e[0][3] * v->w;
	r->y = m->e[1][0] * v->x + m->e[1][1] * v->y + m->e[1][2] * v->z + m->e[1][3] * v->w;
	r->z = m->e[2][0] * v->x + m->e[2][1] * v->y + m->e[2][2] * v->z + m->e[2][3] * v->w;
	r->w = m->e[3][0] * v->x + m->e[3][1] * v->y + m->e[3][2] * v->z + m->e[3][3] * v->w;
}

void MatMul(const struct Matrix *m1, const struct Matrix* m2, struct Matrix *r)
{
  for(int i = 0; i < 4; i++)
    for(int j = 0; j < 4; j++)
      r->e[i][j] = m1->e[i][0] * m2->e[0][j] + m1->e[i][1] * m2->e[1][j] + m1->e[i][2] * m2->e[2][j] + m1->e[i][3] * m2->e[3][j];
}

void MatTranspose(const struct Matrix *m, struct Matrix *r)
{
	r->e[0][0] = m->e[0][0]; r->e[0][1] = m->e[1][0]; r->e[0][2] = m->e[2][0]; r->e[0][3] = m->e[3][0];
	r->e[1][0] = m->e[0][1]; r->e[1][1] = m->e[1][1]; r->e[1][2] = m->e[2][1]; r->e[1][3] = m->e[3][1];
	r->e[2][0] = m->e[0][2]; r->e[2][1] = m->e[1][2]; r->e[2][2] = m->e[2][2]; r->e[2][3] = m->e[3][2];
	r->e[3][0] = m->e[0][3]; r->e[3][1] = m->e[1][3]; r->e[3][2] = m->e[2][3]; r->e[3][3] = m->e[3][3];
}

void MatTransposeInner(const struct Matrix *m, struct Matrix *r)
{
	r->e[0][0] = m->e[0][0]; r->e[0][1] = m->e[1][0]; r->e[0][2] = m->e[2][0]; r->e[0][3] = m->e[0][3];
	r->e[1][0] = m->e[0][1]; r->e[1][1] = m->e[1][1]; r->e[1][2] = m->e[2][1]; r->e[1][3] = m->e[1][3];
	r->e[2][0] = m->e[0][2]; r->e[2][1] = m->e[1][2]; r->e[2][2] = m->e[2][2]; r->e[2][3] = m->e[2][3];
	VecCopy((struct Vector *)m->e[3], (struct Vector *)r->e[3]);
}

void MatInvert(const struct Matrix *m, struct Matrix *r)
{

}

void MatInvertRigidTransform(const struct Matrix *m, struct Matrix *r)
{
	MatTransposeInner(m, r);
	r->e[0][3] = -m->e[0][3];
	r->e[0][3] = -m->e[1][3];
	r->e[0][3] = -m->e[2][3];
	VecCopy((struct Vector *)m->e[3], (struct Vector *)r->e[3]);
}