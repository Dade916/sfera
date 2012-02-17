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
//  PARAM_MAX_DIFFUSE_BOUNCE
//  PARAM_MAX_SPECULARGLOSSY_BOUNCE
//  PARAM_IL_SHIFT_U
//  PARAM_IL_SHIFT_V
//  PARAM_IL_GAIN_R
//  PARAM_IL_GAIN_G
//  PARAM_IL_GAIN_B
//  PARAM_IL_MAP_WIDTH
//  PARAM_IL_MAP_HEIGHT
//  PARAM_ENABLE_MAT_MATTE
//  PARAM_ENABLE_MAT_MIRROR
//  PARAM_ENABLE_MAT_GLASS
//  PARAM_ENABLE_MAT_METAL
//  PARAM_ENABLE_MAT_ALLOY
//  PARAM_HAS_TEXTUREMAPS
//  PARAM_HAS_BUMPMAPS
//  PARAM_GAMMA
//  PARAM_TM_LINEAR_SCALE
//  PARMA_MEM_TYPE

//#pragma OPENCL EXTENSION cl_amd_printf : enable

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef INV_PI
#define INV_PI  0.31830988618379067154f
#endif

#ifndef INV_TWOPI
#define INV_TWOPI  0.15915494309189533577f
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

typedef struct {
	float lensRadius;
	float focalDistance;
	float yon, hither;

	float rasterToCameraMatrix[4][4];
	float cameraToWorldMatrix[4][4];
} Camera;

//------------------------------------------------------------------------------

typedef struct {
	unsigned int rgbOffset;
	unsigned int width, height;
} TexMap;

typedef struct {
	unsigned int texMapIndex;
	float shiftU, shiftV;
	float scaleU, scaleV;
} TexMapInstance;

typedef struct {
	unsigned int texMapIndex;
	float shiftU, shiftV;
	float scaleU, scaleV;
	float scale;
} BumpMapInstance;

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
} MirrorParam;

typedef struct {
    float refl_r, refl_g, refl_b;
    float refrct_r, refrct_g, refrct_b;
    float ousideIor, ior;
    float R0;
} GlassParam;

typedef struct {
    float r, g, b;
    float exponent;
} MetalParam;

typedef struct {
    float diff_r, diff_g, diff_b;
    float refl_r, refl_g, refl_b;
    float exponent;
    float R0;
} AlloyParam;

typedef struct {
	unsigned int type;
	float emi_r, emi_g, emi_b;

	union {
		MatteParam matte;
		MirrorParam mirror;
        GlassParam glass;
        MetalParam metal;
        AlloyParam alloy;
	} param;
} Material;

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

float Spectrum_Filter(const Spectrum *s) {
	return max(max(s->r, s->g), s->b);
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
// Texture maps
//------------------------------------------------------------------------------

void TexMap_GetTexel(__global Spectrum *pixels, const uint width, const uint height,
		const int s, const int t, Spectrum *col) {
	const uint u = Mod(s, width);
	const uint v = Mod(t, height);

	const unsigned index = v * width + u;

	col->r = pixels[index].r;
	col->g = pixels[index].g;
	col->b = pixels[index].b;
}

void TexMap_GetColor(__global Spectrum *pixels, const uint width, const uint height,
		const float u, const float v, Spectrum *col) {
	const float s = u * width - 0.5f;
	const float t = v * height - 0.5f;

	const int s0 = (int)floor(s);
	const int t0 = (int)floor(t);

	const float ds = s - s0;
	const float dt = t - t0;

	const float ids = 1.f - ds;
	const float idt = 1.f - dt;

	Spectrum c0, c1, c2, c3;
	TexMap_GetTexel(pixels, width, height, s0, t0, &c0);
	TexMap_GetTexel(pixels, width, height, s0, t0 + 1, &c1);
	TexMap_GetTexel(pixels, width, height, s0 + 1, t0, &c2);
	TexMap_GetTexel(pixels, width, height, s0 + 1, t0 + 1, &c3);

	const float k0 = ids * idt;
	const float k1 = ids * dt;
	const float k2 = ds * idt;
	const float k3 = ds * dt;

	col->r = k0 * c0.r + k1 * c1.r + k2 * c2.r + k3 * c3.r;
	col->g = k0 * c0.g + k1 * c1.g + k2 * c2.g + k3 * c3.g;
	col->b = k0 * c0.b + k1 * c1.b + k2 * c2.b + k3 * c3.b;
}

//------------------------------------------------------------------------------
// InfiniteLight_Le
//------------------------------------------------------------------------------

void InfiniteLight_Le(__global Spectrum *infiniteLightMap, Spectrum *le, const Vector *dir) {
	const float u = 1.f - SphericalPhi(dir) * INV_TWOPI +  PARAM_IL_SHIFT_U;
	const float v = SphericalTheta(dir) * INV_PI + PARAM_IL_SHIFT_V;

	TexMap_GetColor(infiniteLightMap, PARAM_IL_MAP_WIDTH, PARAM_IL_MAP_HEIGHT, u, v, le);

	le->r *= PARAM_IL_GAIN_R;
	le->g *= PARAM_IL_GAIN_G;
	le->b *= PARAM_IL_GAIN_B;
}

//------------------------------------------------------------------------------
// GenerateCameraRay
//------------------------------------------------------------------------------

void GenerateCameraRay(
		Seed *seed,
		PARAM_MEM_TYPE Camera *camera,
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

bool Sphere_IntersectP(PARAM_MEM_TYPE BVHAccelArrayNode *bvhNode, const Ray *ray, float *hitT) {
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
		PARAM_MEM_TYPE Sphere **hitSphere,
		uint *primitiveIndex,
		PARAM_MEM_TYPE BVHAccelArrayNode *bvhTree) {
	unsigned int currentNode = 0; // Root Node
	unsigned int stopNode = bvhTree[0].skipIndex; // Non-existent
	*primitiveIndex = 0xffffffffu;

	while (currentNode < stopNode) {
		float hitT;
		if (Sphere_IntersectP(&bvhTree[currentNode], ray, &hitT)) {
			if ((bvhTree[currentNode].primitiveIndex != 0xffffffffu) && (hitT < ray->maxt)){
				ray->maxt = hitT;
				*hitSphere = &bvhTree[currentNode].bsphere;
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
// Materials
//------------------------------------------------------------------------------

void Matte_Sample_f(const PARAM_MEM_TYPE MatteParam *mat, const Vector *wo, Vector *wi,
		float *pdf, Spectrum *f, const Vector *shadeN,
		Seed *seed,
		bool *diffuseBounce) {
	Vector dir;
	CosineSampleHemisphere(&dir, RndFloatValue(seed), RndFloatValue(seed));
	const float dp = dir.z;
	// Using 0.0001 instead of 0.0 to cut down fireflies
	if (dp <= 0.0001f) {
		*pdf = 0.f;
		return;
	}

	*pdf = INV_PI;

	Vector v1, v2;
	CoordinateSystem(shadeN, &v1, &v2);

	wi->x = v1.x * dir.x + v2.x * dir.y + shadeN->x * dir.z;
	wi->y = v1.y * dir.x + v2.y * dir.y + shadeN->y * dir.z;
	wi->z = v1.z * dir.x + v2.z * dir.y + shadeN->z * dir.z;

	f->r = mat->r * dp;
	f->g = mat->g * dp;
	f->b = mat->b * dp;

	*diffuseBounce = true;
}

void Mirror_Sample_f(const PARAM_MEM_TYPE MirrorParam *mat, const Vector *wo, Vector *wi,
		float *pdf, Spectrum *f, const Vector *shadeN,
		bool *diffuseBounce) {
    const float k = 2.f * Dot(shadeN, wo);
	wi->x = k * shadeN->x - wo->x;
	wi->y = k * shadeN->y - wo->y;
	wi->z = k * shadeN->z - wo->z;

	*pdf = 1.f;

	f->r = mat->r;
	f->g = mat->g;
	f->b = mat->b;

	*diffuseBounce = false;
}

void Glass_Sample_f(const PARAM_MEM_TYPE GlassParam *mat,
    const Vector *wo, Vector *wi, float *pdf, Spectrum *f, const Vector *N, const Vector *shadeN,
    Seed *seed,
	bool *diffuseBounce) {
    Vector reflDir;
    const float k = 2.f * Dot(N, wo);
    reflDir.x = k * N->x - wo->x;
    reflDir.y = k * N->y - wo->y;
    reflDir.z = k * N->z - wo->z;

    // Ray from outside going in ?
    const bool into = (Dot(N, shadeN) > 0.f);

    const float nc = mat->ousideIor;
    const float nt = mat->ior;
    const float nnt = into ? (nc / nt) : (nt / nc);
    const float ddn = -Dot(wo, shadeN);
    const float cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn);

	*diffuseBounce = false;

    // Total internal reflection
    if (cos2t < 0.f) {
        *wi = reflDir;
        *pdf = 1.f;

        f->r = mat->refl_r;
        f->g = mat->refl_g;
        f->b = mat->refl_b;
    } else {
        const float kk = (into ? 1.f : -1.f) * (ddn * nnt + sqrt(cos2t));
        Vector nkk = *N;
        nkk.x *= kk;
        nkk.y *= kk;
        nkk.z *= kk;

        Vector transDir;
        transDir.x = -nnt * wo->x - nkk.x;
        transDir.y = -nnt * wo->y - nkk.y;
        transDir.z = -nnt * wo->z - nkk.z;
        Normalize(&transDir);

        const float c = min(1.f, 1.f - (into ? -ddn : Dot(&transDir, N)));

        const float R0 = mat->R0;
        const float Re = R0 + (1.f - R0) * c * c * c * c * c;
        const float Tr = 1.f - Re;
        const float P = .25f + .5f * Re;

        if (Tr == 0.f) {
            if (Re == 0.f)
                *pdf = 0.f;
            else {
                *wi = reflDir;
                *pdf = 1.f;

                f->r = mat->refl_r;
                f->g = mat->refl_g;
                f->b = mat->refl_b;
            }
        } else if (Re == 0.f) {
            *wi = transDir;
            *pdf = 1.f;

            f->r = mat->refrct_r;
            f->g = mat->refrct_g;
            f->b = mat->refrct_b;
        } else if (RndFloatValue(seed) < P) {
            *wi = reflDir;
            *pdf = P / Re;

            f->r = mat->refl_r / (*pdf);
            f->g = mat->refl_g / (*pdf);
            f->b = mat->refl_b / (*pdf);
        } else {
            *wi = transDir;
            *pdf = (1.f - P) / Tr;

            f->r = mat->refrct_r / (*pdf);
            f->g = mat->refrct_g / (*pdf);
            f->b = mat->refrct_b / (*pdf);
        }
    }
}

void GlossyReflection(const Vector *wo, Vector *wi, const float exponent,
		const Vector *shadeN,
		const float u0, const float u1) {
    const float phi = 2.f * M_PI * u0;
    const float cosTheta = pow(1.f - u1, exponent);
    const float sinTheta = sqrt(1.f - cosTheta * cosTheta);
    const float x = cos(phi) * sinTheta;
    const float y = sin(phi) * sinTheta;
    const float z = cosTheta;

    Vector w;
    const float RdotShadeN = Dot(shadeN, wo);
	w.x = (2.f * RdotShadeN) * shadeN->x - wo->x;
	w.y = (2.f * RdotShadeN) * shadeN->y - wo->y;
	w.z = (2.f * RdotShadeN) * shadeN->z - wo->z;

    Vector u, a;
    if (fabs(shadeN->x) > .1f) {
        a.x = 0.f;
        a.y = 1.f;
    } else {
        a.x = 1.f;
        a.y = 0.f;
    }
    a.z = 0.f;
    Cross(&u, &a, &w);
    Normalize(&u);
    Vector v;
    Cross(&v, &w, &u);

    wi->x = x * u.x + y * v.x + z * w.x;
    wi->y = x * u.y + y * v.y + z * w.y;
    wi->z = x * u.z + y * v.z + z * w.z;
}

void Metal_Sample_f(const PARAM_MEM_TYPE MetalParam *mat, const Vector *wo, Vector *wi,
		float *pdf, Spectrum *f, const Vector *shadeN,
		Seed *seed,
		bool *diffuseBounce) {
	GlossyReflection(wo, wi, mat->exponent, shadeN, RndFloatValue(seed), RndFloatValue(seed));

	f->r = mat->r;
	f->g = mat->g;
	f->b = mat->b;

	*diffuseBounce = true;

	*pdf =  (Dot(wi, shadeN) > 0.f) ? 1.f : 0.f;
}

void Alloy_Sample_f(const PARAM_MEM_TYPE AlloyParam *mat, const Vector *wo, Vector *wi,
		float *pdf, Spectrum *f, const Vector *shadeN,
		Seed *seed,
		bool *diffuseBounce) {
    // Schilick's approximation
    const float c = 1.f - Dot(wo, shadeN);
    const float R0 = mat->R0;
    const float Re = R0 + (1.f - R0) * c * c * c * c * c;

    const float P = .25f + .5f * Re;

	const float u0 = RndFloatValue(seed);
	const float u1 = RndFloatValue(seed);

    if (RndFloatValue(seed) <= P) {
        GlossyReflection(wo, wi, mat->exponent, shadeN, u0, u1);
        *pdf = P / Re;

        f->r = mat->refl_r / (*pdf);
        f->g = mat->refl_g / (*pdf);
        f->b = mat->refl_b / (*pdf);

		*diffuseBounce = true;
    } else {
        Vector dir;
        CosineSampleHemisphere(&dir, u0, u1);
		const float dp = dir.z;
		// Using 0.0001 instead of 0.0 to cut down fireflies
		if (dp <= 0.0001f) {
			*pdf = 0.f;
			return;
		}

        *pdf = INV_PI;

        Vector v1, v2;
        CoordinateSystem(shadeN, &v1, &v2);

        wi->x = v1.x * dir.x + v2.x * dir.y + shadeN->x * dir.z;
        wi->y = v1.y * dir.x + v2.y * dir.y + shadeN->y * dir.z;
        wi->z = v1.z * dir.x + v2.z * dir.y + shadeN->z * dir.z;

		const float iRe = 1.f - Re;
		const float k = (1.f - P) / iRe;
		*pdf *= k;

		const float dpk = dp / k;
		f->r = mat->diff_r * dpk;
		f->g = mat->diff_g * dpk;
		f->b = mat->diff_b * dpk;

		*diffuseBounce = false;
	}
}

//------------------------------------------------------------------------------
// Init Kernel
//------------------------------------------------------------------------------

__kernel void Init(
		__global GPUTask *tasks
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)
		return;

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
		PARAM_MEM_TYPE BVHAccelArrayNode *bvhRoot,
		PARAM_MEM_TYPE Camera *camera,
		__global Spectrum *infiniteLightMap,
		__global Pixel *frameBuffer,
		PARAM_MEM_TYPE Material *mats,
		__global uint *sphereMats
#if defined(PARAM_HAS_TEXTUREMAPS)
		, PARAM_MEM_TYPE TexMap *texMaps
		, __global Spectrum *texMapRGB
		, PARAM_MEM_TYPE TexMapInstance *sphereTexMaps
#if defined (PARAM_HAS_BUMPMAPS)
		, PARAM_MEM_TYPE BumpMapInstance *sphereBumpMaps
#endif
#endif
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

	Spectrum radiance;
	radiance.r = 0.f;
	radiance.g = 0.f;
	radiance.b = 0.f;

	Ray ray;
	GenerateCameraRay(&seed, camera, pixelIndex, &ray);

	Spectrum throughput;
	throughput.r = 1.f;
	throughput.g = 1.f;
	throughput.b = 1.f;

	uint diffuseBounces = 0;
	uint specularGlossyBounces = 0;

	for(;;) {
		PARAM_MEM_TYPE Sphere *hitSphere;
		uint sphereIndex;
		if (BVH_Intersect(&ray, &hitSphere, &sphereIndex, bvhRoot)) {
			const PARAM_MEM_TYPE Material *hitPointMat = &mats[sphereMats[sphereIndex]];
#if defined(PARAM_HAS_TEXTUREMAPS)
			const PARAM_MEM_TYPE TexMapInstance *hitTexMapInst = &sphereTexMaps[sphereIndex];
#if defined (PARAM_HAS_BUMPMAPS)
			const PARAM_MEM_TYPE BumpMapInstance *hitBumpMapInst = &sphereBumpMaps[sphereIndex];
#endif
#endif

			Point hitPoint;
			hitPoint.x = ray.o.x + ray.maxt * ray.d.x;
			hitPoint.y = ray.o.y + ray.maxt * ray.d.y;
			hitPoint.z = ray.o.z + ray.maxt * ray.d.z;

			Vector N;
			N.x = hitPoint.x - hitSphere->center.x;
			N.y = hitPoint.y - hitSphere->center.y;
			N.z = hitPoint.z - hitSphere->center.z;
			Normalize(&N);

			Vector shadeN = N;

#if defined (PARAM_HAS_BUMPMAPS)
			const uint bumpMapIndex = hitBumpMapInst->texMapIndex;
			if (bumpMapIndex != 0xffffffffu) {
				const float u0 = SphericalPhi(&N) * INV_TWOPI * hitBumpMapInst->scaleU + hitBumpMapInst->shiftU;
				const float v0 = SphericalTheta(&N) * INV_PI * hitBumpMapInst->scaleV + hitBumpMapInst->shiftV;

				const PARAM_MEM_TYPE TexMap *tm = &texMaps[bumpMapIndex];
				const unsigned int width = tm->width;
				const unsigned int height = tm->height;

				const float du = 1.f / width;
				const float dv = 1.f / height;

				Spectrum col0;
				TexMap_GetColor(&texMapRGB[tm->rgbOffset], width, height, u0, v0, &col0);
				const float b0 = Spectrum_Filter(&col0);

				Spectrum colu;
				TexMap_GetColor(&texMapRGB[tm->rgbOffset], width, height, u0 + du, v0, &colu);
				const float bu = Spectrum_Filter(&colu);

				Spectrum colv;
				TexMap_GetColor(&texMapRGB[tm->rgbOffset], width, height, u0, v0 + dv, &colv);
				const float bv = Spectrum_Filter(&colv);

				const float scale = hitBumpMapInst->scale;
				Vector bump;
				bump.x = scale * (bu - b0);
				bump.y = scale * (bv - b0);
				bump.z = 1.f;

				Vector v1, v2;
				CoordinateSystem(&N, &v1, &v2);

				shadeN.x = v1.x * bump.x + v2.x * bump.y + N.x * bump.z;
				shadeN.y = v1.y * bump.x + v2.y * bump.y + N.y * bump.z;
				shadeN.z = v1.z * bump.x + v2.z * bump.y + N.z * bump.z;
				Normalize(&shadeN);
			}
#endif

			// Check if I have to flip the normal
			const bool flipNormal = (Dot(&N, &ray.d) > 0.f);
			if (flipNormal) {
				shadeN.x *= -1.f;
				shadeN.y *= -1.f;
				shadeN.z *= -1.f;
			}

			uint matType = hitPointMat->type;
			radiance.r += throughput.r * hitPointMat->emi_r;
			radiance.g += throughput.g * hitPointMat->emi_g;
			radiance.b += throughput.b * hitPointMat->emi_b;

			Vector wo;
			wo.x = -ray.d.x;
			wo.y = -ray.d.y;
			wo.z = -ray.d.z;
			Vector wi;
			float materialPdf;
			Spectrum f;
			bool diffuseBounce;
			switch (matType) {

#if defined(PARAM_ENABLE_MAT_MATTE)
				case MAT_MATTE:
					Matte_Sample_f(&hitPointMat->param.matte, &wo, &wi, &materialPdf, &f, &shadeN, &seed, &diffuseBounce);
					break;
#endif

#if defined(PARAM_ENABLE_MAT_MIRROR)
				case MAT_MIRROR:
					Mirror_Sample_f(&hitPointMat->param.mirror, &wo, &wi, &materialPdf, &f, &shadeN, &diffuseBounce);
					break;
#endif

#if defined(PARAM_ENABLE_MAT_GLASS)
				case MAT_GLASS:
					Glass_Sample_f(&hitPointMat->param.glass, &wo, &wi, &materialPdf, &f, &N, &shadeN, &seed, &diffuseBounce);
					break;
#endif

#if defined(PARAM_ENABLE_MAT_METAL)
				case MAT_METAL:
					Metal_Sample_f(&hitPointMat->param.metal, &wo, &wi, &materialPdf, &f, &shadeN, &seed, &diffuseBounce);
					break;
#endif

#if defined(PARAM_ENABLE_MAT_ALLOY)
				case MAT_ALLOY:
					Alloy_Sample_f(&hitPointMat->param.alloy, &wo, &wi, &materialPdf, &f, &shadeN, &seed, &diffuseBounce);
					break;
#endif

				default:
					// Huston, we have a problem...
					diffuseBounce = false;
					materialPdf = 0.f;
					break;
			}

			if (materialPdf == 0.f)
				break;

			if (diffuseBounce) {
				++diffuseBounces;

				if (diffuseBounces > PARAM_MAX_DIFFUSE_BOUNCE)
					break;
			} else {
				++specularGlossyBounces;

				if (specularGlossyBounces > PARAM_MAX_SPECULARGLOSSY_BOUNCE)
					break;
			}

#if defined(PARAM_HAS_TEXTUREMAPS)
			const uint texMapIndex = hitTexMapInst->texMapIndex;

			if (texMapIndex != 0xffffffffu) {
				const float tu = SphericalPhi(&N) * INV_TWOPI * hitTexMapInst->scaleU + hitTexMapInst->shiftU;
				const float tv = SphericalTheta(&N) * INV_PI * hitTexMapInst->scaleV + hitTexMapInst->shiftV;

				PARAM_MEM_TYPE TexMap *tm = &texMaps[texMapIndex];
				Spectrum texCol;
				TexMap_GetColor(&texMapRGB[tm->rgbOffset], tm->width, tm->height, tu, tv, &texCol);

				f.r *= texCol.r;
				f.g *= texCol.g;
				f.b *= texCol.b;
			}
#endif

			throughput.r *= f.r;
			throughput.g *= f.g;
			throughput.b *= f.b;

			ray.o = hitPoint;
			ray.d = wi;
			ray.mint = PARAM_RAY_EPSILON;
			ray.maxt = INFINITY;
		} else {
			Spectrum iLe;
			InfiniteLight_Le(infiniteLightMap, &iLe, &ray.d);

			radiance.r += throughput.r * iLe.r;
			radiance.g += throughput.g * iLe.g;
			radiance.b += throughput.b * iLe.b;
			break;
		}
	}

	/*if ((radiance.r < 0.f) || (radiance.g < 0.f) || (radiance.b < 0.f) ||
			isnan(radiance.r) || isnan(radiance.g) || isnan(radiance.b))
		printf(\"Error radiance: [%f, %f, %f]\\n\", radiance.r, radiance.g, radiance.b);*/

	__global Pixel *p = &frameBuffer[pixelIndex];
	p->r += radiance.r * (1.f / PARAM_SCREEN_SAMPLEPERPASS);
	p->g += radiance.g * (1.f / PARAM_SCREEN_SAMPLEPERPASS);
	p->b += radiance.b * (1.f / PARAM_SCREEN_SAMPLEPERPASS);

	// Save the seed
	task->seed.s1 = seed.s1;
	task->seed.s2 = seed.s2;
	task->seed.s3 = seed.s3;
}

//------------------------------------------------------------------------------
// Image filtering kernels
//------------------------------------------------------------------------------

void ApplyBlurFilterXR1(
		__global Pixel *src,
		__global Pixel *dst,
		const float aF,
		const float bF,
		const float cF
		) {
	// Do left edge
	Pixel a;
	Pixel b = src[0];
	Pixel c = src[1];

	const float leftTotF = bF + cF;
	const float bLeftK = bF / leftTotF;
	const float cLeftK = cF / leftTotF;
	dst[0].r = bLeftK  * b.r + cLeftK * c.r;
	dst[0].g = bLeftK  * b.g + cLeftK * c.g;
	dst[0].b = bLeftK  * b.b + cLeftK * c.b;

    // Main loop
	const float totF = aF + bF + cF;
	const float aK = aF / totF;
	const float bK = bF / totF;
	const float cK = cF / totF;

	for (unsigned int x = 1; x < PARAM_SCREEN_WIDTH - 1; ++x) {
		a = b;
		b = c;
		c = src[x + 1];

		// AMD OpenCL have some problem to run this code
		dst[x].r = aK * a.r + bK * b.r + cK * c.r;
		dst[x].g = aK * a.g + bK * b.g + cK * c.g;
		dst[x].b = aK * a.b + bK * b.b + cK * c.b;
    }

    // Do right edge
	const float rightTotF = aF + bF;
	const float aRightK = aF / rightTotF;
	const float bRightK = bF / rightTotF;
	a = b;
	b = c;
	dst[PARAM_SCREEN_WIDTH - 1].r = aRightK * a.r + bRightK * b.r;
	dst[PARAM_SCREEN_WIDTH - 1].g = aRightK * a.g + bRightK * b.g;
	dst[PARAM_SCREEN_WIDTH - 1].b = aRightK * a.b + bRightK * b.b;

}

void ApplyBlurFilterYR1(
		__global Pixel *src,
		__global Pixel *dst,
		const float aF,
		const float bF,
		const float cF
		) {
	// Do left edge
	Pixel a;
	Pixel b = src[0];
	Pixel c = src[PARAM_SCREEN_WIDTH];

	const float leftTotF = bF + cF;
	const float bLeftK = bF / leftTotF;
	const float cLeftK = cF / leftTotF;
	dst[0].r = bLeftK  * b.r + cLeftK * c.r;
	dst[0].g = bLeftK  * b.g + cLeftK * c.g;
	dst[0].b = bLeftK  * b.b + cLeftK * c.b;

    // Main loop
	const float totF = aF + bF + cF;
	const float aK = aF / totF;
	const float bK = bF / totF;
	const float cK = cF / totF;

    for (unsigned int y = 1; y < PARAM_SCREEN_HEIGHT - 1; ++y) {
		const unsigned index = y * PARAM_SCREEN_WIDTH;

		a = b;
		b = c;
		c = src[index + PARAM_SCREEN_WIDTH];

		// AMD OpenCL have some problem to run this code
		dst[index].r = aK * a.r + bK * b.r + cK * c.r;
		dst[index].g = aK * a.g + bK * b.g + cK * c.g;
		dst[index].b = aK * a.b + bK * b.b + cK * c.b;
    }

    // Do right edge
	const float rightTotF = aF + bF;
	const float aRightK = aF / rightTotF;
	const float bRightK = bF / rightTotF;
	a = b;
	b = c;
	dst[(PARAM_SCREEN_HEIGHT - 1) * PARAM_SCREEN_WIDTH].r = aRightK * a.r + bRightK * b.r;
	dst[(PARAM_SCREEN_HEIGHT - 1) * PARAM_SCREEN_WIDTH].g = aRightK * a.g + bRightK * b.g;
	dst[(PARAM_SCREEN_HEIGHT - 1) * PARAM_SCREEN_WIDTH].b = aRightK * a.b + bRightK * b.b;
}

__kernel void ApplyBlurLightFilterXR1(
		__global Pixel *src,
		__global Pixel *dst
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_HEIGHT)
		return;

	src += gid * PARAM_SCREEN_WIDTH;
	dst += gid * PARAM_SCREEN_WIDTH;

	const float aF = .15f;
	const float bF = 1.f;
	const float cF = .15f;

	ApplyBlurFilterXR1(src, dst, aF, bF, cF);
}

__kernel void ApplyBlurLightFilterYR1(
		__global Pixel *src,
		__global Pixel *dst
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH)
		return;

	src += gid;
	dst += gid;

	const float aF = .15f;
	const float bF = 1.f;
	const float cF = .15f;

	ApplyBlurFilterYR1(src, dst, aF, bF, cF);
}

__kernel void ApplyBlurHeavyFilterXR1(
		__global Pixel *src,
		__global Pixel *dst
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_HEIGHT)
		return;

	src += gid * PARAM_SCREEN_WIDTH;
	dst += gid * PARAM_SCREEN_WIDTH;

	const float aF = .35f;
	const float bF = 1.f;
	const float cF = .35f;

	ApplyBlurFilterXR1(src, dst, aF, bF, cF);
}

__kernel void ApplyBlurHeavyFilterYR1(
		__global Pixel *src,
		__global Pixel *dst
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH)
		return;

	src += gid;
	dst += gid;

	const float aF = .35f;
	const float bF = 1.f;
	const float cF = .35f;

	ApplyBlurFilterYR1(src, dst, aF, bF, cF);
}

__kernel void ApplyBoxFilterXR1(
		__global Pixel *src,
		__global Pixel *dst
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_HEIGHT)
		return;

	src += gid * PARAM_SCREEN_WIDTH;
	dst += gid * PARAM_SCREEN_WIDTH;

	const float aF = .35f;
	const float bF = 1.f;
	const float cF = .35f;

	ApplyBlurFilterXR1(src, dst, aF, bF, cF);
}

__kernel void ApplyBoxFilterYR1(
		__global Pixel *src,
		__global Pixel *dst
		) {
	const size_t gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH)
		return;

	src += gid;
	dst += gid;

	const float aF = 1.f / 3.f;
	const float bF = 1.f / 3.f;
	const float cF = 1.f / 3.f;

	ApplyBlurFilterYR1(src, dst, aF, bF, cF);
}

//------------------------------------------------------------------------------
// BlendBuffer Kernel
//------------------------------------------------------------------------------

__kernel void BlendFrame(
		__global Pixel *src,
		__global Pixel *dst,
		const float blendFactorSrc) {
	const int gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)
		return;

	const Pixel sp = src[gid];
	const Pixel dp = dst[gid];
	__global Pixel *p = &dst[gid];

	const float blendFactorDst = 1.f - blendFactorSrc;
	p->r = blendFactorDst * dp.r + blendFactorSrc * sp.r;
	p->g = blendFactorDst * dp.g + blendFactorSrc * sp.g;
	p->b = blendFactorDst * dp.b + blendFactorSrc * sp.b;
}

//------------------------------------------------------------------------------
// Linear Tone Map Kernel
//------------------------------------------------------------------------------

__kernel void ToneMapLinear(
		__global Pixel *src,
		__global Pixel *dst) {
	const int gid = get_global_id(0);
	if (gid >= PARAM_SCREEN_WIDTH * PARAM_SCREEN_HEIGHT)
		return;

	const Pixel sp = src[gid];
	__global Pixel *dp = &dst[gid];

	dp->r = PARAM_TM_LINEAR_SCALE * sp.r;
	dp->g = PARAM_TM_LINEAR_SCALE * sp.g;
	dp->b = PARAM_TM_LINEAR_SCALE * sp.b;
}

//------------------------------------------------------------------------------
// UpdatePixelBuffer Kernel
//------------------------------------------------------------------------------

uint Radiance2PixelUInt(const float x) {
	return (uint)(pow(clamp(x, 0.f, 1.f), 1.f / PARAM_GAMMA) * 255.f + .5f);
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
