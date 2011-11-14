/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of Sfera.                                           *
 *                                                                         *
 *   Sfera is free software; you can redistribute it and/or modify         *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Sfera is distributed in the hope that it will be useful,              *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 ***************************************************************************/
// List of symbols defined at compile time:
//  PARAM_SCREEN_WIDTH
//  PARAM_SCREEN_HEIGHT
//  PARAM_SCREEN_SAMPLEPERPASS
//  PARAM_RAY_EPSILON

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef INV_PI
#define INV_PI  0.31830988618379067154f
#endif

#ifndef INV_TWOPI
#define INV_TWOPI  0.15915494309189533577f
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct {
	float u, v;
} UV;

typedef struct {
	float r, g, b;
} Spectrum;

typedef struct {
	float x, y, z;
} Point;

typedef struct {
	float x, y, z;
} Vector;

typedef struct {
	Point o;
	Vector d;
	float mint, maxt;
} Ray;

typedef struct {
	Point center;
	float rad;
} Sphere;

typedef struct {
	Sphere bsphere;
	unsigned int primitiveIndex;
	unsigned int skipIndex;
} BVHAccelArrayNode;

//------------------------------------------------------------------------------

typedef struct {
	unsigned int s1, s2, s3;
} Seed;

typedef struct {
	// The task seed
	Seed seed;
} GPUTask;

typedef Spectrum Pixel;

//------------------------------------------------------------------------------

#define MAT_MATTE 0
#define MAT_MIRROR 1
#define MAT_GLASS 2
#define MAT_METAL 3
#define MAT_ALLOY 4

typedef struct {
    float r, g, b;
} MatteParam;

typedef struct {
    float r, g, b;
    int specularBounce;
} MirrorParam;

typedef struct {
    float refl_r, refl_g, refl_b;
    float refrct_r, refrct_g, refrct_b;
    float ousideIor, ior;
    float R0;
    int reflectionSpecularBounce, transmitionSpecularBounce;
} GlassParam;

typedef struct {
    float r, g, b;
    float exponent;
    int specularBounce;
} MetalParam;

typedef struct {
    float diff_r, diff_g, diff_b;
    float refl_r, refl_g, refl_b;
    float exponent;
    float R0;
    int specularBounce;
} AlloyParam;

typedef struct {
	unsigned int type;
	union {
		MatteParam matte;
		MirrorParam mirror;
        GlassParam glass;
        MetalParam metal;
        AlloyParam alloy;
	} param;
} Material;

//------------------------------------------------------------------------------

typedef struct {
	Point v0, v1, v2;
	Vector normal;
	float area;
	float gain_r, gain_g, gain_b;
} TriangleLight;

typedef struct {
	float shiftU, shiftV;
	Spectrum gain;
	uint width, height;
} InfiniteLight;

//------------------------------------------------------------------------------

typedef struct {
	uint rgbOffset, alphaOffset;
	uint width, height;
} TexMap;

typedef struct {
	float lensRadius;
	float focalDistance;
	float yon, hither;

	float rasterToCameraMatrix[4][4];
	float cameraToWorldMatrix[4][4];
} Camera;

//------------------------------------------------------------------------------
// Random number generator
// maximally equidistributed combined Tausworthe generator
//------------------------------------------------------------------------------

#define FLOATMASK 0x00ffffffu

uint TAUSWORTHE(const uint s, const uint a,
	const uint b, const uint c,
	const uint d) {
	return ((s&c)<<d) ^ (((s << a) ^ s) >> b);
}

uint LCG(const uint x) { return x * 69069; }

uint ValidSeed(const uint x, const uint m) {
	return (x < m) ? (x + m) : x;
}

void InitRandomGenerator(uint seed, Seed *s) {
	// Avoid 0 value
	seed = (seed == 0) ? (seed + 0xffffffu) : seed;

	s->s1 = ValidSeed(LCG(seed), 1);
	s->s2 = ValidSeed(LCG(s->s1), 7);
	s->s3 = ValidSeed(LCG(s->s2), 15);
}

unsigned long RndUintValue(Seed *s) {
	s->s1 = TAUSWORTHE(s->s1, 13, 19, 4294967294UL, 12);
	s->s2 = TAUSWORTHE(s->s2, 2, 25, 4294967288UL, 4);
	s->s3 = TAUSWORTHE(s->s3, 3, 11, 4294967280UL, 17);

	return ((s->s1) ^ (s->s2) ^ (s->s3));
}

float RndFloatValue(Seed *s) {
	return (RndUintValue(s) & FLOATMASK) * (1.f / (FLOATMASK + 1UL));
}

//------------------------------------------------------------------------------

float Spectrum_Y(const Spectrum *s) {
	return 0.212671f * s->r + 0.715160f * s->g + 0.072169f * s->b;
}

float Dot(const Vector *v0, const Vector *v1) {
	return v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;
}

void Normalize(Vector *v) {
	const float il = 1.f / sqrt(Dot(v, v));

	v->x *= il;
	v->y *= il;
	v->z *= il;
}

void Cross(Vector *v3, const Vector *v1, const Vector *v2) {
	v3->x = (v1->y * v2->z) - (v1->z * v2->y);
	v3->y = (v1->z * v2->x) - (v1->x * v2->z),
	v3->z = (v1->x * v2->y) - (v1->y * v2->x);
}

int Mod(int a, int b) {
	if (b == 0)
		b = 1;

	a %= b;
	if (a < 0)
		a += b;

	return a;
}

float Lerp(float t, float v1, float v2) {
	return (1.f - t) * v1 + t * v2;
}

void ConcentricSampleDisk(const float u1, const float u2, float *dx, float *dy) {
	float r, theta;
	// Map uniform random numbers to $[-1,1]^2$
	float sx = 2.f * u1 - 1.f;
	float sy = 2.f * u2 - 1.f;
	// Map square to $(r,\theta)$
	// Handle degeneracy at the origin
	if (sx == 0.f && sy == 0.f) {
		*dx = 0.f;
		*dy = 0.f;
		return;
	}
	if (sx >= -sy) {
		if (sx > sy) {
			// Handle first region of disk
			r = sx;
			if (sy > 0.f)
				theta = sy / r;
			else
				theta = 8.f + sy / r;
		} else {
			// Handle second region of disk
			r = sy;
			theta = 2.f - sx / r;
		}
	} else {
		if (sx <= sy) {
			// Handle third region of disk
			r = -sx;
			theta = 4.f - sy / r;
		} else {
			// Handle fourth region of disk
			r = -sy;
			theta = 6.f + sx / r;
		}
	}
	theta *= M_PI / 4.f;
	*dx = r * cos(theta);
	*dy = r * sin(theta);
}

void CosineSampleHemisphere(Vector *ret, const float u1, const float u2) {
	ConcentricSampleDisk(u1, u2, &ret->x, &ret->y);
	ret->z = sqrt(max(0.f, 1.f - ret->x * ret->x - ret->y * ret->y));
}

void UniformSampleCone(Vector *ret, const float u1, const float u2, const float costhetamax,
	const Vector *x, const Vector *y, const Vector *z) {
	const float costheta = Lerp(u1, costhetamax, 1.f);
	const float sintheta = sqrt(1.f - costheta * costheta);
	const float phi = u2 * 2.f * M_PI;

	const float kx = cos(phi) * sintheta;
	const float ky = sin(phi) * sintheta;
	const float kz = costheta;

	ret->x = kx * x->x + ky * y->x + kz * z->x;
	ret->y = kx * x->y + ky * y->y + kz * z->y;
	ret->z = kx * x->z + ky * y->z + kz * z->z;
}

float UniformConePdf(float costhetamax) {
	return 1.f / (2.f * M_PI * (1.f - costhetamax));
}

void CoordinateSystem(const Vector *v1, Vector *v2, Vector *v3) {
	if (fabs(v1->x) > fabs(v1->y)) {
		float invLen = 1.f / sqrt(v1->x * v1->x + v1->z * v1->z);
		v2->x = -v1->z * invLen;
		v2->y = 0.f;
		v2->z = v1->x * invLen;
	} else {
		float invLen = 1.f / sqrt(v1->y * v1->y + v1->z * v1->z);
		v2->x = 0.f;
		v2->y = v1->z * invLen;
		v2->z = -v1->y * invLen;
	}

	Cross(v3, v1, v2);
}

float SphericalTheta(const Vector *v) {
	return acos(clamp(v->z, -1.f, 1.f));
}

float SphericalPhi(const Vector *v) {
	float p = atan2(v->y, v->x);
	return (p < 0.f) ? p + 2.f * M_PI : p;
}

//------------------------------------------------------------------------------
// GenerateCameraRay
//------------------------------------------------------------------------------

void GenerateCameraRay(
		Seed *seed,
		__global Camera *camera,
		const uint pixelIndex,
		Ray *ray) {
	const float scrSampleX = RndFloatValue(seed);
	const float scrSampleY = RndFloatValue(seed);

	const float screenX = pixelIndex % PARAM_SCREEN_WIDTH + scrSampleX - .5f;
	const float screenY = pixelIndex / PARAM_SCREEN_WIDTH + scrSampleY - .5f;

	Point Pras;
	Pras.x = screenX;
	Pras.y = PARAM_SCREEN_HEIGHT - screenY - 1.f;
	Pras.z = 0;

	Point orig;
	// RasterToCamera(Pras, &orig);

	const float iw = 1.f / (camera->rasterToCameraMatrix[3][0] * Pras.x + camera->rasterToCameraMatrix[3][1] * Pras.y + camera->rasterToCameraMatrix[3][2] * Pras.z + camera->rasterToCameraMatrix[3][3]);
	orig.x = (camera->rasterToCameraMatrix[0][0] * Pras.x + camera->rasterToCameraMatrix[0][1] * Pras.y + camera->rasterToCameraMatrix[0][2] * Pras.z + camera->rasterToCameraMatrix[0][3]) * iw;
	orig.y = (camera->rasterToCameraMatrix[1][0] * Pras.x + camera->rasterToCameraMatrix[1][1] * Pras.y + camera->rasterToCameraMatrix[1][2] * Pras.z + camera->rasterToCameraMatrix[1][3]) * iw;
	orig.z = (camera->rasterToCameraMatrix[2][0] * Pras.x + camera->rasterToCameraMatrix[2][1] * Pras.y + camera->rasterToCameraMatrix[2][2] * Pras.z + camera->rasterToCameraMatrix[2][3]) * iw;

	Vector dir;
	dir.x = orig.x;
	dir.y = orig.y;
	dir.z = orig.z;

	const float hither = camera->hither;

	Normalize(&dir);

	// CameraToWorld(*ray, ray);
	Point torig;
	const float iw2 = 1.f / (camera->cameraToWorldMatrix[3][0] * orig.x + camera->cameraToWorldMatrix[3][1] * orig.y + camera->cameraToWorldMatrix[3][2] * orig.z + camera->cameraToWorldMatrix[3][3]);
	torig.x = (camera->cameraToWorldMatrix[0][0] * orig.x + camera->cameraToWorldMatrix[0][1] * orig.y + camera->cameraToWorldMatrix[0][2] * orig.z + camera->cameraToWorldMatrix[0][3]) * iw2;
	torig.y = (camera->cameraToWorldMatrix[1][0] * orig.x + camera->cameraToWorldMatrix[1][1] * orig.y + camera->cameraToWorldMatrix[1][2] * orig.z + camera->cameraToWorldMatrix[1][3]) * iw2;
	torig.z = (camera->cameraToWorldMatrix[2][0] * orig.x + camera->cameraToWorldMatrix[2][1] * orig.y + camera->cameraToWorldMatrix[2][2] * orig.z + camera->cameraToWorldMatrix[2][3]) * iw2;

	Vector tdir;
	tdir.x = camera->cameraToWorldMatrix[0][0] * dir.x + camera->cameraToWorldMatrix[0][1] * dir.y + camera->cameraToWorldMatrix[0][2] * dir.z;
	tdir.y = camera->cameraToWorldMatrix[1][0] * dir.x + camera->cameraToWorldMatrix[1][1] * dir.y + camera->cameraToWorldMatrix[1][2] * dir.z;
	tdir.z = camera->cameraToWorldMatrix[2][0] * dir.x + camera->cameraToWorldMatrix[2][1] * dir.y + camera->cameraToWorldMatrix[2][2] * dir.z;

	ray->o = torig;
	ray->d = tdir;
	ray->mint = PARAM_RAY_EPSILON;
	ray->maxt = (camera->yon - hither) / dir.z;

	/*printf(\"(%f, %f, %f) (%f, %f, %f) [%f, %f]\\n\",
		ray->o.x, ray->o.y, ray->o.z, ray->d.x, ray->d.y, ray->d.z,
		ray->mint, ray->maxt);*/
}

//------------------------------------------------------------------------------
// BVH intersect
//------------------------------------------------------------------------------

bool Sphere_IntersectP(__global BVHAccelArrayNode *bvhNode, const Ray *ray, float *hitT) {
	const Point center = bvhNode->bsphere.center;
	const float rad = bvhNode->bsphere.rad;

	Vector op;
	op.x = center.x - ray->o.x;
	op.y = center.y - ray->o.y;
	op.z = center.z - ray->o.z;
	const float b = Dot(&op, &ray->d);

	float det = b * b - Dot(&op, &op) + rad * rad;
	if (det < 0.f)
		return false;
	else
		det = sqrt(det);

	float t = b - det;
	if ((t > ray->mint) && ((t < ray->maxt)))
		*hitT = t;
	else {
		t = b + det;

		if ((t > ray->mint) && ((t < ray->maxt)))
			*hitT = t;
		else
			*hitT = INFINITY;
	}

	return true;
}

bool BVH_Intersect(
		Ray *ray,
		uint *primitiveIndex,
		__global BVHAccelArrayNode *bvhTree) {
	unsigned int currentNode = 0; // Root Node
	unsigned int stopNode = bvhTree[0].skipIndex; // Non-existent
	*primitiveIndex = 0xffffffffu;

	while (currentNode < stopNode) {
		float hitT;
		if (Sphere_IntersectP(&bvhTree[currentNode], ray, &hitT)) {
			if ((bvhTree[currentNode].primitiveIndex != 0xffffffffu) && (hitT < ray->maxt)){
				ray->maxt = hitT;
				*primitiveIndex = bvhTree[currentNode].primitiveIndex;
				// Continue testing for closer intersections
			}

			currentNode++;
		} else
			currentNode = bvhTree[currentNode].skipIndex;
	}

	return (*primitiveIndex) != 0xffffffffu;
}

//------------------------------------------------------------------------------
// Init Kernel
//------------------------------------------------------------------------------

__kernel void Init(
		__global GPUTask *tasks
		) {
	const size_t gid = get_global_id(0);

	// Initialize the task
	__global GPUTask *task = &tasks[gid];

	// Initialize random number generator
	Seed seed;
	InitRandomGenerator(gid + 1, &seed);

	// Save the seed
	task->seed.s1 = seed.s1;
	task->seed.s2 = seed.s2;
	task->seed.s3 = seed.s3;
}

//------------------------------------------------------------------------------
// InitFB Kernel
//------------------------------------------------------------------------------

__kernel void InitFB(
		__global Pixel *frameBuffer
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)
		return;

	__global Pixel *p = &frameBuffer[gid];
	p->r = 0.f;
	p->g = 0.f;
	p->b = 0.f;
}

//------------------------------------------------------------------------------
// PathTracing Kernel
//------------------------------------------------------------------------------

__kernel void PathTracing(
		__global GPUTask *tasks,
		__global BVHAccelArrayNode *bvhRoot,
		__global Camera *camera,
		__global Pixel *frameBuffer
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)
		return;

	__global GPUTask *task = &tasks[gid];
	const uint pixelIndex = gid;

	// Read the seed
	Seed seed;
	seed.s1 = task->seed.s1;
	seed.s2 = task->seed.s2;
	seed.s3 = task->seed.s3;

	Ray ray;
	GenerateCameraRay(&seed, camera, pixelIndex, &ray);

	uint sphereIndex;
	Pixel c;
	if (BVH_Intersect(&ray,&sphereIndex, bvhRoot)) {
		c.r = 1.f;
		c.g = 1.f;
		c.b = 1.f;
	} else {
		c.r = 0.f;
		c.g = 0.f;
		c.b = 0.f;
	}

	__global Pixel *p = &frameBuffer[pixelIndex];
	*p = c;

	// Save the seed
	task->seed.s1 = seed.s1;
	task->seed.s2 = seed.s2;
	task->seed.s3 = seed.s3;
}

//------------------------------------------------------------------------------
// UpdatePixelBuffer Kernel
//------------------------------------------------------------------------------

uint Radiance2PixelUInt(const float x) {
	return (uint)(pow(clamp(x, 0.f, 1.f), 1.f / 2.2f) * 255.f + .5f);
}

__kernel void UpdatePixelBuffer(
		__global Pixel *frameBuffer,
		__global uint *pbo) {
	const int gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)
		return;

	__global Pixel *p = &frameBuffer[gid];

	const uint r = Radiance2PixelUInt(p->r);
	const uint g = Radiance2PixelUInt(p->g);
	const uint b = Radiance2PixelUInt(p->b);
	pbo[gid] = r | (g << 8) | (b << 16);
}
