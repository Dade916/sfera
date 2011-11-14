#include "renderer/ocl/kernels/kernels.h"
string KernelSource_kernel_core = 
"/***************************************************************************\n"
" *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt)                  *\n"
" *                                                                         *\n"
" *   This file is part of Sfera.                                           *\n"
" *                                                                         *\n"
" *   Sfera is free software; you can redistribute it and/or modify         *\n"
" *   it under the terms of the GNU General Public License as published by  *\n"
" *   the Free Software Foundation; either version 3 of the License, or     *\n"
" *   (at your option) any later version.                                   *\n"
" *                                                                         *\n"
" *   Sfera is distributed in the hope that it will be useful,              *\n"
" *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *\n"
" *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *\n"
" *   GNU General Public License for more details.                          *\n"
" *                                                                         *\n"
" *   You should have received a copy of the GNU General Public License     *\n"
" *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *\n"
" *                                                                         *\n"
" ***************************************************************************/\n"
"// List of symbols defined at compile time:\n"
"//  PARAM_SCREEN_WIDTH\n"
"//  PARAM_SCREEN_HEIGHT\n"
"//  PARAM_SCREEN_SAMPLEPERPASS\n"
"//  PARAM_RAY_EPSILON\n"
"//  PARAM_MAX_DIFFUSE_BOUNCE\n"
"//  PARAM_MAX_SPECULARGLOSSY_BOUNCE\n"
"//  PARAM_IL_SHIFT_U\n"
"//  PARAM_IL_SHIFT_V\n"
"//  PARAM_IL_GAIN_R\n"
"//  PARAM_IL_GAIN_G\n"
"//  PARAM_IL_GAIN_B\n"
"//  PARAM_IL_MAP_WIDTH\n"
"//  PARAM_IL_MAP_HEIGHT\n"
"\n"
"#ifndef M_PI\n"
"#define M_PI 3.14159265358979323846f\n"
"#endif\n"
"\n"
"#ifndef INV_PI\n"
"#define INV_PI  0.31830988618379067154f\n"
"#endif\n"
"\n"
"#ifndef INV_TWOPI\n"
"#define INV_TWOPI  0.15915494309189533577f\n"
"#endif\n"
"\n"
"#ifndef TRUE\n"
"#define TRUE 1\n"
"#endif\n"
"\n"
"#ifndef FALSE\n"
"#define FALSE 0\n"
"#endif\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Types\n"
"//------------------------------------------------------------------------------\n"
"\n"
"typedef struct {\n"
"	float u, v;\n"
"} UV;\n"
"\n"
"typedef struct {\n"
"	float r, g, b;\n"
"} Spectrum;\n"
"\n"
"typedef struct {\n"
"	float x, y, z;\n"
"} Point;\n"
"\n"
"typedef struct {\n"
"	float x, y, z;\n"
"} Vector;\n"
"\n"
"typedef struct {\n"
"	Point o;\n"
"	Vector d;\n"
"	float mint, maxt;\n"
"} Ray;\n"
"\n"
"typedef struct {\n"
"	Point center;\n"
"	float rad;\n"
"} Sphere;\n"
"\n"
"typedef struct {\n"
"	Sphere bsphere;\n"
"	unsigned int primitiveIndex;\n"
"	unsigned int skipIndex;\n"
"} BVHAccelArrayNode;\n"
"\n"
"//------------------------------------------------------------------------------\n"
"\n"
"typedef struct {\n"
"	unsigned int s1, s2, s3;\n"
"} Seed;\n"
"\n"
"typedef struct {\n"
"	// The task seed\n"
"	Seed seed;\n"
"} GPUTask;\n"
"\n"
"typedef Spectrum Pixel;\n"
"\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#define MAT_MATTE 0\n"
"#define MAT_MIRROR 1\n"
"#define MAT_GLASS 2\n"
"#define MAT_METAL 3\n"
"#define MAT_ALLOY 4\n"
"\n"
"typedef struct {\n"
"    float r, g, b;\n"
"} MatteParam;\n"
"\n"
"typedef struct {\n"
"    float r, g, b;\n"
"} MirrorParam;\n"
"\n"
"typedef struct {\n"
"    float refl_r, refl_g, refl_b;\n"
"    float refrct_r, refrct_g, refrct_b;\n"
"    float ousideIor, ior;\n"
"    float R0;\n"
"} GlassParam;\n"
"\n"
"typedef struct {\n"
"    float r, g, b;\n"
"    float exponent;\n"
"} MetalParam;\n"
"\n"
"typedef struct {\n"
"    float diff_r, diff_g, diff_b;\n"
"    float refl_r, refl_g, refl_b;\n"
"    float exponent;\n"
"    float R0;\n"
"} AlloyParam;\n"
"\n"
"typedef struct {\n"
"	unsigned int type;\n"
"	union {\n"
"		MatteParam matte;\n"
"		MirrorParam mirror;\n"
"        GlassParam glass;\n"
"        MetalParam metal;\n"
"        AlloyParam alloy;\n"
"	} param;\n"
"} Material;\n"
"\n"
"//------------------------------------------------------------------------------\n"
"\n"
"typedef struct {\n"
"	uint rgbOffset, alphaOffset;\n"
"	uint width, height;\n"
"} TexMap;\n"
"\n"
"typedef struct {\n"
"	float lensRadius;\n"
"	float focalDistance;\n"
"	float yon, hither;\n"
"\n"
"	float rasterToCameraMatrix[4][4];\n"
"	float cameraToWorldMatrix[4][4];\n"
"} Camera;\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Random number generator\n"
"// maximally equidistributed combined Tausworthe generator\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#define FLOATMASK 0x00ffffffu\n"
"\n"
"uint TAUSWORTHE(const uint s, const uint a,\n"
"	const uint b, const uint c,\n"
"	const uint d) {\n"
"	return ((s&c)<<d) ^ (((s << a) ^ s) >> b);\n"
"}\n"
"\n"
"uint LCG(const uint x) { return x * 69069; }\n"
"\n"
"uint ValidSeed(const uint x, const uint m) {\n"
"	return (x < m) ? (x + m) : x;\n"
"}\n"
"\n"
"void InitRandomGenerator(uint seed, Seed *s) {\n"
"	// Avoid 0 value\n"
"	seed = (seed == 0) ? (seed + 0xffffffu) : seed;\n"
"\n"
"	s->s1 = ValidSeed(LCG(seed), 1);\n"
"	s->s2 = ValidSeed(LCG(s->s1), 7);\n"
"	s->s3 = ValidSeed(LCG(s->s2), 15);\n"
"}\n"
"\n"
"unsigned long RndUintValue(Seed *s) {\n"
"	s->s1 = TAUSWORTHE(s->s1, 13, 19, 4294967294UL, 12);\n"
"	s->s2 = TAUSWORTHE(s->s2, 2, 25, 4294967288UL, 4);\n"
"	s->s3 = TAUSWORTHE(s->s3, 3, 11, 4294967280UL, 17);\n"
"\n"
"	return ((s->s1) ^ (s->s2) ^ (s->s3));\n"
"}\n"
"\n"
"float RndFloatValue(Seed *s) {\n"
"	return (RndUintValue(s) & FLOATMASK) * (1.f / (FLOATMASK + 1UL));\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"\n"
"float Spectrum_Y(const Spectrum *s) {\n"
"	return 0.212671f * s->r + 0.715160f * s->g + 0.072169f * s->b;\n"
"}\n"
"\n"
"float Dot(const Vector *v0, const Vector *v1) {\n"
"	return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;\n"
"}\n"
"\n"
"void Normalize(Vector *v) {\n"
"	const float il = 1.f / sqrt(Dot(v, v));\n"
"\n"
"	v->x *= il;\n"
"	v->y *= il;\n"
"	v->z *= il;\n"
"}\n"
"\n"
"void Cross(Vector *v3, const Vector *v1, const Vector *v2) {\n"
"	v3->x = (v1->y * v2->z) - (v1->z * v2->y);\n"
"	v3->y = (v1->z * v2->x) - (v1->x * v2->z),\n"
"	v3->z = (v1->x * v2->y) - (v1->y * v2->x);\n"
"}\n"
"\n"
"int Mod(int a, int b) {\n"
"	if (b == 0)\n"
"		b = 1;\n"
"\n"
"	a %= b;\n"
"	if (a < 0)\n"
"		a += b;\n"
"\n"
"	return a;\n"
"}\n"
"\n"
"float Lerp(float t, float v1, float v2) {\n"
"	return (1.f - t) * v1 + t * v2;\n"
"}\n"
"\n"
"void ConcentricSampleDisk(const float u1, const float u2, float *dx, float *dy) {\n"
"	float r, theta;\n"
"	// Map uniform random numbers to $[-1,1]^2$\n"
"	float sx = 2.f * u1 - 1.f;\n"
"	float sy = 2.f * u2 - 1.f;\n"
"	// Map square to $(r,\theta)$\n"
"	// Handle degeneracy at the origin\n"
"	if (sx == 0.f && sy == 0.f) {\n"
"		*dx = 0.f;\n"
"		*dy = 0.f;\n"
"		return;\n"
"	}\n"
"	if (sx >= -sy) {\n"
"		if (sx > sy) {\n"
"			// Handle first region of disk\n"
"			r = sx;\n"
"			if (sy > 0.f)\n"
"				theta = sy / r;\n"
"			else\n"
"				theta = 8.f + sy / r;\n"
"		} else {\n"
"			// Handle second region of disk\n"
"			r = sy;\n"
"			theta = 2.f - sx / r;\n"
"		}\n"
"	} else {\n"
"		if (sx <= sy) {\n"
"			// Handle third region of disk\n"
"			r = -sx;\n"
"			theta = 4.f - sy / r;\n"
"		} else {\n"
"			// Handle fourth region of disk\n"
"			r = -sy;\n"
"			theta = 6.f + sx / r;\n"
"		}\n"
"	}\n"
"	theta *= M_PI / 4.f;\n"
"	*dx = r * cos(theta);\n"
"	*dy = r * sin(theta);\n"
"}\n"
"\n"
"void CosineSampleHemisphere(Vector *ret, const float u1, const float u2) {\n"
"	ConcentricSampleDisk(u1, u2, &ret->x, &ret->y);\n"
"	ret->z = sqrt(max(0.f, 1.f - ret->x * ret->x - ret->y * ret->y));\n"
"}\n"
"\n"
"void UniformSampleCone(Vector *ret, const float u1, const float u2, const float costhetamax,\n"
"	const Vector *x, const Vector *y, const Vector *z) {\n"
"	const float costheta = Lerp(u1, costhetamax, 1.f);\n"
"	const float sintheta = sqrt(1.f - costheta * costheta);\n"
"	const float phi = u2 * 2.f * M_PI;\n"
"\n"
"	const float kx = cos(phi) * sintheta;\n"
"	const float ky = sin(phi) * sintheta;\n"
"	const float kz = costheta;\n"
"\n"
"	ret->x = kx * x->x + ky * y->x + kz * z->x;\n"
"	ret->y = kx * x->y + ky * y->y + kz * z->y;\n"
"	ret->z = kx * x->z + ky * y->z + kz * z->z;\n"
"}\n"
"\n"
"float UniformConePdf(float costhetamax) {\n"
"	return 1.f / (2.f * M_PI * (1.f - costhetamax));\n"
"}\n"
"\n"
"void CoordinateSystem(const Vector *v1, Vector *v2, Vector *v3) {\n"
"	if (fabs(v1->x) > fabs(v1->y)) {\n"
"		float invLen = 1.f / sqrt(v1->x * v1->x + v1->z * v1->z);\n"
"		v2->x = -v1->z * invLen;\n"
"		v2->y = 0.f;\n"
"		v2->z = v1->x * invLen;\n"
"	} else {\n"
"		float invLen = 1.f / sqrt(v1->y * v1->y + v1->z * v1->z);\n"
"		v2->x = 0.f;\n"
"		v2->y = v1->z * invLen;\n"
"		v2->z = -v1->y * invLen;\n"
"	}\n"
"\n"
"	Cross(v3, v1, v2);\n"
"}\n"
"\n"
"float SphericalTheta(const Vector *v) {\n"
"	return acos(clamp(v->z, -1.f, 1.f));\n"
"}\n"
"\n"
"float SphericalPhi(const Vector *v) {\n"
"	float p = atan2(v->y, v->x);\n"
"	return (p < 0.f) ? p + 2.f * M_PI : p;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Texture maps\n"
"//------------------------------------------------------------------------------\n"
"\n"
"void TexMap_GetTexel(__global Spectrum *pixels, const uint width, const uint height,\n"
"		const int s, const int t, Spectrum *col) {\n"
"	const uint u = Mod(s, width);\n"
"	const uint v = Mod(t, height);\n"
"\n"
"	const unsigned index = v * width + u;\n"
"\n"
"	col->r = pixels[index].r;\n"
"	col->g = pixels[index].g;\n"
"	col->b = pixels[index].b;\n"
"}\n"
"\n"
"void TexMap_GetColor(__global Spectrum *pixels, const uint width, const uint height,\n"
"		const float u, const float v, Spectrum *col) {\n"
"	const float s = u * width - 0.5f;\n"
"	const float t = v * height - 0.5f;\n"
"\n"
"	const int s0 = (int)floor(s);\n"
"	const int t0 = (int)floor(t);\n"
"\n"
"	const float ds = s - s0;\n"
"	const float dt = t - t0;\n"
"\n"
"	const float ids = 1.f - ds;\n"
"	const float idt = 1.f - dt;\n"
"\n"
"	Spectrum c0, c1, c2, c3;\n"
"	TexMap_GetTexel(pixels, width, height, s0, t0, &c0);\n"
"	TexMap_GetTexel(pixels, width, height, s0, t0 + 1, &c1);\n"
"	TexMap_GetTexel(pixels, width, height, s0 + 1, t0, &c2);\n"
"	TexMap_GetTexel(pixels, width, height, s0 + 1, t0 + 1, &c3);\n"
"\n"
"	const float k0 = ids * idt;\n"
"	const float k1 = ids * dt;\n"
"	const float k2 = ds * idt;\n"
"	const float k3 = ds * dt;\n"
"\n"
"	col->r = k0 * c0.r + k1 * c1.r + k2 * c2.r + k3 * c3.r;\n"
"	col->g = k0 * c0.g + k1 * c1.g + k2 * c2.g + k3 * c3.g;\n"
"	col->b = k0 * c0.b + k1 * c1.b + k2 * c2.b + k3 * c3.b;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// InfiniteLight_Le\n"
"//------------------------------------------------------------------------------\n"
"\n"
"void InfiniteLight_Le(__global Spectrum *infiniteLightMap, Spectrum *le, const Vector *dir) {\n"
"	const float u = 1.f - SphericalPhi(dir) * INV_TWOPI +  PARAM_IL_SHIFT_U;\n"
"	const float v = SphericalTheta(dir) * INV_PI + PARAM_IL_SHIFT_V;\n"
"\n"
"	TexMap_GetColor(infiniteLightMap, PARAM_IL_MAP_WIDTH, PARAM_IL_MAP_HEIGHT, u, v, le);\n"
"\n"
"	le->r *= PARAM_IL_GAIN_R;\n"
"	le->g *= PARAM_IL_GAIN_G;\n"
"	le->b *= PARAM_IL_GAIN_B;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// GenerateCameraRay\n"
"//------------------------------------------------------------------------------\n"
"\n"
"void GenerateCameraRay(\n"
"		Seed *seed,\n"
"		__global Camera *camera,\n"
"		const uint pixelIndex,\n"
"		Ray *ray) {\n"
"	const float scrSampleX = RndFloatValue(seed);\n"
"	const float scrSampleY = RndFloatValue(seed);\n"
"\n"
"	const float screenX = pixelIndex % PARAM_SCREEN_WIDTH + scrSampleX - .5f;\n"
"	const float screenY = pixelIndex / PARAM_SCREEN_WIDTH + scrSampleY - .5f;\n"
"\n"
"	Point Pras;\n"
"	Pras.x = screenX;\n"
"	Pras.y = PARAM_SCREEN_HEIGHT - screenY - 1.f;\n"
"	Pras.z = 0;\n"
"\n"
"	Point orig;\n"
"	// RasterToCamera(Pras, &orig);\n"
"\n"
"	const float iw = 1.f / (camera->rasterToCameraMatrix[3][0] * Pras.x + camera->rasterToCameraMatrix[3][1] * Pras.y + camera->rasterToCameraMatrix[3][2] * Pras.z + camera->rasterToCameraMatrix[3][3]);\n"
"	orig.x = (camera->rasterToCameraMatrix[0][0] * Pras.x + camera->rasterToCameraMatrix[0][1] * Pras.y + camera->rasterToCameraMatrix[0][2] * Pras.z + camera->rasterToCameraMatrix[0][3]) * iw;\n"
"	orig.y = (camera->rasterToCameraMatrix[1][0] * Pras.x + camera->rasterToCameraMatrix[1][1] * Pras.y + camera->rasterToCameraMatrix[1][2] * Pras.z + camera->rasterToCameraMatrix[1][3]) * iw;\n"
"	orig.z = (camera->rasterToCameraMatrix[2][0] * Pras.x + camera->rasterToCameraMatrix[2][1] * Pras.y + camera->rasterToCameraMatrix[2][2] * Pras.z + camera->rasterToCameraMatrix[2][3]) * iw;\n"
"\n"
"	Vector dir;\n"
"	dir.x = orig.x;\n"
"	dir.y = orig.y;\n"
"	dir.z = orig.z;\n"
"\n"
"	const float hither = camera->hither;\n"
"\n"
"	Normalize(&dir);\n"
"\n"
"	// CameraToWorld(*ray, ray);\n"
"	Point torig;\n"
"	const float iw2 = 1.f / (camera->cameraToWorldMatrix[3][0] * orig.x + camera->cameraToWorldMatrix[3][1] * orig.y + camera->cameraToWorldMatrix[3][2] * orig.z + camera->cameraToWorldMatrix[3][3]);\n"
"	torig.x = (camera->cameraToWorldMatrix[0][0] * orig.x + camera->cameraToWorldMatrix[0][1] * orig.y + camera->cameraToWorldMatrix[0][2] * orig.z + camera->cameraToWorldMatrix[0][3]) * iw2;\n"
"	torig.y = (camera->cameraToWorldMatrix[1][0] * orig.x + camera->cameraToWorldMatrix[1][1] * orig.y + camera->cameraToWorldMatrix[1][2] * orig.z + camera->cameraToWorldMatrix[1][3]) * iw2;\n"
"	torig.z = (camera->cameraToWorldMatrix[2][0] * orig.x + camera->cameraToWorldMatrix[2][1] * orig.y + camera->cameraToWorldMatrix[2][2] * orig.z + camera->cameraToWorldMatrix[2][3]) * iw2;\n"
"\n"
"	Vector tdir;\n"
"	tdir.x = camera->cameraToWorldMatrix[0][0] * dir.x + camera->cameraToWorldMatrix[0][1] * dir.y + camera->cameraToWorldMatrix[0][2] * dir.z;\n"
"	tdir.y = camera->cameraToWorldMatrix[1][0] * dir.x + camera->cameraToWorldMatrix[1][1] * dir.y + camera->cameraToWorldMatrix[1][2] * dir.z;\n"
"	tdir.z = camera->cameraToWorldMatrix[2][0] * dir.x + camera->cameraToWorldMatrix[2][1] * dir.y + camera->cameraToWorldMatrix[2][2] * dir.z;\n"
"\n"
"	ray->o = torig;\n"
"	ray->d = tdir;\n"
"	ray->mint = PARAM_RAY_EPSILON;\n"
"	ray->maxt = (camera->yon - hither) / dir.z;\n"
"\n"
"	/*printf(\"(%f, %f, %f) (%f, %f, %f) [%f, %f]\\n\",\n"
"		ray->o.x, ray->o.y, ray->o.z, ray->d.x, ray->d.y, ray->d.z,\n"
"		ray->mint, ray->maxt);*/\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// BVH intersect\n"
"//------------------------------------------------------------------------------\n"
"\n"
"bool Sphere_IntersectP(__global BVHAccelArrayNode *bvhNode, const Ray *ray, float *hitT) {\n"
"	const Point center = bvhNode->bsphere.center;\n"
"	const float rad = bvhNode->bsphere.rad;\n"
"\n"
"	Vector op;\n"
"	op.x = center.x - ray->o.x;\n"
"	op.y = center.y - ray->o.y;\n"
"	op.z = center.z - ray->o.z;\n"
"	const float b = Dot(&op, &ray->d);\n"
"\n"
"	float det = b * b - Dot(&op, &op) + rad * rad;\n"
"	if (det < 0.f)\n"
"		return false;\n"
"	else\n"
"		det = sqrt(det);\n"
"\n"
"	float t = b - det;\n"
"	if ((t > ray->mint) && ((t < ray->maxt)))\n"
"		*hitT = t;\n"
"	else {\n"
"		t = b + det;\n"
"\n"
"		if ((t > ray->mint) && ((t < ray->maxt)))\n"
"			*hitT = t;\n"
"		else\n"
"			*hitT = INFINITY;\n"
"	}\n"
"\n"
"	return true;\n"
"}\n"
"\n"
"bool BVH_Intersect(\n"
"		Ray *ray,\n"
"		uint *primitiveIndex,\n"
"		__global BVHAccelArrayNode *bvhTree) {\n"
"	unsigned int currentNode = 0; // Root Node\n"
"	unsigned int stopNode = bvhTree[0].skipIndex; // Non-existent\n"
"	*primitiveIndex = 0xffffffffu;\n"
"\n"
"	while (currentNode < stopNode) {\n"
"		float hitT;\n"
"		if (Sphere_IntersectP(&bvhTree[currentNode], ray, &hitT)) {\n"
"			if ((bvhTree[currentNode].primitiveIndex != 0xffffffffu) && (hitT < ray->maxt)){\n"
"				ray->maxt = hitT;\n"
"				*primitiveIndex = bvhTree[currentNode].primitiveIndex;\n"
"				// Continue testing for closer intersections\n"
"			}\n"
"\n"
"			currentNode++;\n"
"		} else\n"
"			currentNode = bvhTree[currentNode].skipIndex;\n"
"	}\n"
"\n"
"	return (*primitiveIndex) != 0xffffffffu;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Init Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel void Init(\n"
"		__global GPUTask *tasks\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"\n"
"	// Initialize the task\n"
"	__global GPUTask *task = &tasks[gid];\n"
"\n"
"	// Initialize random number generator\n"
"	Seed seed;\n"
"	InitRandomGenerator(gid + 1, &seed);\n"
"\n"
"	// Save the seed\n"
"	task->seed.s1 = seed.s1;\n"
"	task->seed.s2 = seed.s2;\n"
"	task->seed.s3 = seed.s3;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// InitFB Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel void InitFB(\n"
"		__global Pixel *frameBuffer\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)\n"
"		return;\n"
"\n"
"	__global Pixel *p = &frameBuffer[gid];\n"
"	p->r = 0.f;\n"
"	p->g = 0.f;\n"
"	p->b = 0.f;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// PathTracing Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"__kernel void PathTracing(\n"
"		__global GPUTask *tasks,\n"
"		__global BVHAccelArrayNode *bvhRoot,\n"
"		__global Camera *camera,\n"
"		__global Spectrum *infiniteLightMap,\n"
"		__global Pixel *frameBuffer\n"
"		) {\n"
"	const size_t gid = get_global_id(0);\n"
"	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)\n"
"		return;\n"
"\n"
"	__global GPUTask *task = &tasks[gid];\n"
"	const uint pixelIndex = gid;\n"
"\n"
"	// Read the seed\n"
"	Seed seed;\n"
"	seed.s1 = task->seed.s1;\n"
"	seed.s2 = task->seed.s2;\n"
"	seed.s3 = task->seed.s3;\n"
"\n"
"	Ray ray;\n"
"	GenerateCameraRay(&seed, camera, pixelIndex, &ray);\n"
"\n"
"	Spectrum throughput;\n"
"	throughput.r = 1.f;\n"
"	throughput.g = 1.f;\n"
"	throughput.b = 1.f;\n"
"	Spectrum radiance;\n"
"	radiance.r = 0.f;\n"
"	radiance.g = 0.f;\n"
"	radiance.b = 0.f;\n"
"\n"
"	uint diffuseBounces = 0;\n"
"	uint specularGlossyBounces = 0;\n"
"\n"
"	Pixel pixelRadiance;\n"
"	for(;;) {\n"
"		uint sphereIndex;\n"
"		if (BVH_Intersect(&ray,&sphereIndex, bvhRoot)) {\n"
"			pixelRadiance.r = 1.f;\n"
"			pixelRadiance.g = 1.f;\n"
"			pixelRadiance.b = 1.f;\n"
"		} else {\n"
"			Spectrum iLe;\n"
"			InfiniteLight_Le(infiniteLightMap, &iLe, &ray.d);\n"
"\n"
"			pixelRadiance.r = radiance.r + throughput.r * iLe.r;\n"
"			pixelRadiance.g = radiance.g + throughput.g * iLe.g;\n"
"			pixelRadiance.b = radiance.b + throughput.b * iLe.b;\n"
"		}\n"
"		break;\n"
"	}\n"
"\n"
"	__global Pixel *p = &frameBuffer[pixelIndex];\n"
"	p->r += pixelRadiance.r * (1.f / PARAM_SCREEN_SAMPLEPERPASS);\n"
"	p->g += pixelRadiance.g * (1.f / PARAM_SCREEN_SAMPLEPERPASS);\n"
"	p->b += pixelRadiance.b * (1.f / PARAM_SCREEN_SAMPLEPERPASS);\n"
"\n"
"	// Save the seed\n"
"	task->seed.s1 = seed.s1;\n"
"	task->seed.s2 = seed.s2;\n"
"	task->seed.s3 = seed.s3;\n"
"}\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// UpdatePixelBuffer Kernel\n"
"//------------------------------------------------------------------------------\n"
"\n"
"uint Radiance2PixelUInt(const float x) {\n"
"	return (uint)(pow(clamp(x, 0.f, 1.f), 1.f / 2.2f) * 255.f + .5f);\n"
"}\n"
"\n"
"__kernel void UpdatePixelBuffer(\n"
"		__global Pixel *frameBuffer,\n"
"		__global uint *pbo) {\n"
"	const int gid = get_global_id(0);\n"
"	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)\n"
"		return;\n"
"\n"
"	__global Pixel *p = &frameBuffer[gid];\n"
"\n"
"	const uint r = Radiance2PixelUInt(p->r);\n"
"	const uint g = Radiance2PixelUInt(p->g);\n"
"	const uint b = Radiance2PixelUInt(p->b);\n"
"	pbo[gid] = r | (g << 8) | (b << 16);\n"
"}\n"
;
