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

#ifndef _SFERA_SDL_MATERIAL_H
#define	_SFERA_SDL_MATERIAL_H

#include <vector>

#include "geometry/vector.h"
#include "geometry/normal.h"
#include "pixel/spectrum.h"
#include "utils/utils.h"
#include "utils/randomgen.h"

class Scene;

enum MaterialType {
	MATTE, MIRROR, GLASS, METAL, ALLOY
};

class Material {
public:
	Material() { }
	virtual ~Material() { }

	virtual MaterialType GetType() const = 0;

	virtual Spectrum Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N,
		const Normal &shadeN, float *pdf, bool &diffuseBounce) const = 0;

	void SetEmission(const Spectrum &e) { emission = e; }
	const Spectrum &GetEmission() const { return emission; }

protected:
	Spectrum emission;
};

class MatteMaterial : public Material {
public:
	MatteMaterial(const Spectrum &col) {
		Kd = col;
	}

	MaterialType GetType() const { return MATTE; }

	Spectrum Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const;

	const Spectrum &GetKd() const { return Kd; }

private:
	Spectrum Kd;
};

class MirrorMaterial : public Material {
public:
	MirrorMaterial(const Spectrum &refl) {
		Kr = refl;
	}

	MaterialType GetType() const { return MIRROR; }

	Spectrum Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const;

	const Spectrum &GetKr() const { return Kr; }

private:
	Spectrum Kr;
};

class GlassMaterial : public Material {
public:
	GlassMaterial(const Spectrum &refl, const Spectrum &refrct,
			const float outsideIorFact,	const float iorFact) {
		Krefl = refl;
		Krefrct = refrct;
		ousideIor = outsideIorFact;
		ior = iorFact;

		const float nc = ousideIor;
		const float nt = ior;
		const float a = nt - nc;
		const float b = nt + nc;
		R0 = a * a / (b * b);
	}

	MaterialType GetType() const { return GLASS; }

	Spectrum Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const;

	const Spectrum &GetKrefl() const { return Krefl; }
	const Spectrum &GetKrefrct() const { return Krefrct; }
	const float GetOutsideIOR() const { return ousideIor; }
	const float GetIOR() const { return ior; }
	const float GetR0() const { return R0; }

private:
	Spectrum Krefl, Krefrct;
	float ousideIor, ior;
	float R0;
};

class MetalMaterial : public Material {
public:
	MetalMaterial(const Spectrum &refl, const float exp) {
		Kr = refl;
		exponent = 1.f / (exp + 1.f);
	}

	MaterialType GetType() const { return METAL; }

	Spectrum Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const;

	const Spectrum &GetKr() const { return Kr; }
	float GetExp() const { return exponent; }

	static Vector GlossyReflection(const Vector &wo, const float exponent,
		const Normal &shadeN, const float u0, const float u1);

private:
	Spectrum Kr;
	float exponent;
};

class AlloyMaterial : public Material {
public:
	AlloyMaterial(const Spectrum &col, const Spectrum &refl,
			const float exp, const float schlickTerm) {
		Krefl = refl;
		Kdiff = col;
		KdiffOverPI = Kdiff * INV_PI;
		R0 = schlickTerm;
		exponent = 1.f / (exp + 1.f);
	}

	MaterialType GetType() const { return ALLOY; }

	Spectrum Sample_f(RandomGenerator &rnd,
		const Vector &wo, Vector *wi, const Normal &N, const Normal &shadeN,
		float *pdf, bool &diffuseBounce) const;

	const Spectrum &GetKrefl() const { return Krefl; }
	const Spectrum &GetKd() const { return Kdiff; }
	float GetExp() const { return exponent; }
	float GetR0() const { return R0; }

private:
	Spectrum Krefl;
	Spectrum Kdiff, KdiffOverPI;
	float exponent;
	float R0;
};

#endif	/* _SFERA_SDL_MATERIAL_H */
