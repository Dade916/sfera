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

#ifndef _SFERA_SDL_TEXMAP_H
#define	_SFERA_SDL_TEXMAP_H

#include <string>
#include <vector>
#include <map>

#include "geometry/vector.h"
#include "geometry/normal.h"
#include "geometry/uv.h"
#include "pixel/spectrum.h"

class TextureMap {
public:
	TextureMap(const std::string &fileName);
	TextureMap(Spectrum *cols, const unsigned int w, const unsigned int h);

	~TextureMap();

	const Spectrum GetColor(const UV &uv) const {
		const float s = uv.u * width - 0.5f;
		const float t = uv.v * height - 0.5f;

		const int s0 = Floor2Int(s);
		const int t0 = Floor2Int(t);

		const float ds = s - s0;
		const float dt = t - t0;

		const float ids = 1.f - ds;
		const float idt = 1.f - dt;

		return ids * idt * GetTexel(s0, t0) +
				ids * dt * GetTexel(s0, t0 + 1) +
				ds * idt * GetTexel(s0 + 1, t0) +
				ds * dt * GetTexel(s0 + 1, t0 + 1);
	};

	const UV &GetDuDv() const {
		return DuDv;
	}

	unsigned int GetWidth() const { return width; }
	unsigned int GetHeight() const { return height; }
	const Spectrum *GetPixels() const { return pixels; };

private:
	const Spectrum &GetTexel(const int s, const int t) const {
		const unsigned int u = Mod<int>(s, width);
		const unsigned int v = Mod<int>(t, height);

		const unsigned index = v * width + u;
		assert (index >= 0);
		assert (index < width * height);

		return pixels[index];
	}

	unsigned int width, height;
	Spectrum *pixels;
	UV DuDv;
};

class TexMapInstance {
public:
	TexMapInstance(const TextureMap *tm, const float tu, const float tv,
			const float su, const float sv) : texMap(tm),
			shiftU(su), shiftV(sv), scaleU(su), scaleV(sv) { }

	const TextureMap *GetTexMap() const { return texMap; }
	float GetShiftU() const { return shiftU; }
	float GetShiftV() const { return shiftV; }
	float GetScaleU() const { return scaleU; }
	float GetScaleV() const { return scaleV; }

	Spectrum SphericalMap(const Vector &dir) const {
		const UV uv(SphericalPhi(dir) * INV_TWOPI * scaleU + shiftU,
				SphericalTheta(dir) * INV_PI  * scaleV + shiftV);
		return texMap->GetColor(uv);
	}

private:
	const TextureMap *texMap;
	float shiftU, shiftV;
	float scaleU, scaleV;
};

class BumpMapInstance {
public:
	BumpMapInstance(const TextureMap *tm, const float tu, const float tv,
			const float su, const float sv, const float s) : texMap(tm),
			shiftU(su), shiftV(sv), scaleU(su), scaleV(sv), scale(s) { }

	const TextureMap *GetTexMap() const { return texMap; }
	float GetShiftU() const { return shiftU; }
	float GetShiftV() const { return shiftV; }
	float GetScaleU() const { return scaleU; }
	float GetScaleV() const { return scaleV; }
	float GetScale() const { return scale; }

	Normal SphericalMap(const Vector &dir, const Normal &N) const {
		const UV uv(SphericalPhi(dir) * INV_TWOPI * scaleU + shiftU,
				SphericalTheta(dir) * INV_PI  * scaleV + shiftV);

		const UV &dudv = texMap->GetDuDv();

		const float b0 = texMap->GetColor(uv).Filter();

		const UV uvdu(uv.u + dudv.u, uv.v);
		const float bu = texMap->GetColor(uvdu).Filter();

		const UV uvdv(uv.u, uv.v + dudv.v);
		const float bv = texMap->GetColor(uvdv).Filter();

		const Vector bump(scale * (bu - b0), scale * (bv - b0), 1.f);

		Vector v1, v2;
		CoordinateSystem(Vector(N), &v1, &v2);
		return Normalize(Normal(
				v1.x * bump.x + v2.x * bump.y + N.x * bump.z,
				v1.y * bump.x + v2.y * bump.y + N.y * bump.z,
				v1.z * bump.x + v2.z * bump.y + N.z * bump.z));
	}

private:
	const TextureMap *texMap;
	float shiftU, shiftV;
	float scaleU, scaleV;
	const float scale;
};

class TextureMapCache {
public:
	TextureMapCache();
	~TextureMapCache();

	TexMapInstance *GetTexMapInstance(const std::string &fileName,
		const float shiftU, const float shiftV, const float scaleU, const float scaleV);
	BumpMapInstance *GetBumpMapInstance(const std::string &fileName,
		const float shiftU, const float shiftV, const float scaleU, const float scaleV,
		const float scale);

	void GetTexMaps(std::vector<TextureMap *> &tms);
	unsigned int GetSize()const { return maps.size(); }
  
private:
	TextureMap *GetTextureMap(const std::string &fileName);

	std::map<std::string, TextureMap *> maps;
	std::vector<TexMapInstance *> texInstances;
	std::vector<BumpMapInstance *> bumpInstances;
};

#endif	/* _SFERA_SDL_TEXMAP_H */
