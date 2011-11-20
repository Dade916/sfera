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

#include "geometry/vector_normal.h"
#include "utils/mc.h"
#include "sdl/material.h"
#include "utils/randomgen.h"

Vector MetalMaterial::GlossyReflection(const Vector &wo, const float exponent,
	const Normal &shadeN, const float u0, const float u1) {
	const float phi = 2.f * M_PI * u0;
	const float cosTheta = powf(1.f - u1, exponent);
	const float sinTheta = sqrtf(1.f - cosTheta * cosTheta);
	const float x = cosf(phi) * sinTheta;
	const float y = sinf(phi) * sinTheta;
	const float z = cosTheta;

	const Vector dir = -wo;
	const float dp = Dot(shadeN, dir);
	const Vector w = dir - (2.f * dp) * Vector(shadeN);

	Vector u;
	if (fabsf(shadeN.x) > .1f) {
		const Vector a(0.f, 1.f, 0.f);
		u = Cross(a, w);
	} else {
		const Vector a(1.f, 0.f, 0.f);
		u = Cross(a, w);
	}
	u = Normalize(u);
	Vector v = Cross(w, u);

	return x * u + y * v + z * w;
}

Spectrum MatteMaterial::Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const {
	Vector dir = CosineSampleHemisphere(rnd.floatValue(), rnd.floatValue());
	if (dir.z < 0.f)
		dir.z = -dir.z;

	const float dp = dir.z;
	// Using 0.0001 instead of 0.0 to cut down fireflies
	if (dp <= 0.0001f) {
		*pdf = 0.f;
		return Spectrum();
	}

	*pdf = INV_PI;

	Vector v1, v2;
	CoordinateSystem(Vector(shadeN), &v1, &v2);

	dir = Vector(
			v1.x * dir.x + v2.x * dir.y + shadeN.x * dir.z,
			v1.y * dir.x + v2.y * dir.y + shadeN.y * dir.z,
			v1.z * dir.x + v2.z * dir.y + shadeN.z * dir.z);

	(*wi) = dir;

	diffuseBounce = true;

	return Kd * dp;
}

Spectrum MirrorMaterial::Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const {
	const Vector dir = -wo;
	const float dp = Dot(shadeN, dir);
	(*wi) = dir - (2.f * dp) * Vector(shadeN);

	diffuseBounce = false;
	*pdf = 1.f;

	return Kr;
}

Spectrum GlassMaterial::Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const {
	const Vector rayDir = -wo;
	const Vector reflDir = rayDir - (2.f * Dot(N, rayDir)) * Vector(N);

	// Ray from outside going in ?
	const bool into = (Dot(N, shadeN) > 0.f);

	const float nc = ousideIor;
	const float nt = ior;
	const float nnt = into ? (nc / nt) : (nt / nc);
	const float ddn = Dot(rayDir, shadeN);
	const float cos2t = 1.f - nnt * nnt * (1.f - ddn * ddn);

	diffuseBounce = false;

	// Total internal reflection
	if (cos2t < 0.f) {
		(*wi) = reflDir;
		*pdf = 1.f;		

		return Krefl;
	}

	const float kk = (into ? 1.f : -1.f) * (ddn * nnt + sqrtf(cos2t));
	const Vector nkk = kk * Vector(N);
	const Vector transDir = Normalize(nnt * rayDir - nkk);

	const float c = 1.f - (into ? -ddn : Dot(transDir, N));

	const float Re = R0 + (1.f - R0) * c * c * c * c * c;
	const float Tr = 1.f - Re;
	const float P = .25f + .5f * Re;

	if (Tr == 0.f) {
		if (Re == 0.f) {
			*pdf = 0.f;
			return Spectrum();
		} else {
			(*wi) = reflDir;
			*pdf = 1.f;

			return Krefl;
		}
	} else if (Re == 0.f) {
		(*wi) = transDir;
		*pdf = 1.f;

		return Krefrct;
	} else if (rnd.floatValue() < P) {
		(*wi) = reflDir;
		*pdf = P / Re;

		return Krefl / (*pdf);
	} else {
		(*wi) = transDir;
		*pdf = (1.f - P) / Tr;

		return Krefrct / (*pdf);
	}
}

Spectrum MetalMaterial::Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const {
	(*wi) = GlossyReflection(wo, exponent, shadeN, rnd.floatValue(), rnd.floatValue());

	if (Dot(*wi, shadeN) > 0.f) {
		diffuseBounce = false;
		*pdf = 1.f;

		return Kr;
	} else {
		*pdf = 0.f;

		return Spectrum();
	}
}

Spectrum AlloyMaterial::Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const {
	// Schilick's approximation
	const float c = 1.f - Dot(wo, shadeN);
	const float Re = R0 + (1.f - R0) * c * c * c * c * c;

	const float P = .25f + .5f * Re;

	if (rnd.floatValue() < P) {
		(*wi) = MetalMaterial::GlossyReflection(wo, exponent, shadeN,
				rnd.floatValue(), rnd.floatValue());
		*pdf = P / Re;
		diffuseBounce = false;

		return Krefl / (*pdf);
	} else {
		Vector dir = CosineSampleHemisphere(rnd.floatValue(), rnd.floatValue());
		if (dir.z < 0.f)
			dir.z = -dir.z;

		const float dp = dir.z;
		// Using 0.0001 instead of 0.0 to cut down fireflies
		if (dp <= 0.0001f) {
			*pdf = 0.f;
			return Spectrum();
		}

		*pdf = INV_PI;

		Vector v1, v2;
		CoordinateSystem(Vector(shadeN), &v1, &v2);

		dir = Vector(
				v1.x * dir.x + v2.x * dir.y + shadeN.x * dir.z,
				v1.y * dir.x + v2.y * dir.y + shadeN.y * dir.z,
				v1.z * dir.x + v2.z * dir.y + shadeN.z * dir.z);

		(*wi) = dir;

		const float iRe = 1.f - Re;
		*pdf *= (1.f - P) / iRe;
		diffuseBounce = true;

		return Kdiff * dp;
	}
}
