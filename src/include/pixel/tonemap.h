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

#ifndef _SFERA_TONEMAP_H
#define _SFERA_TONEMAP_H

#include "utils/utils.h"
#include "pixel/framebuffer.h"

//------------------------------------------------------------------------------
// Tonemapping
//------------------------------------------------------------------------------

#define GAMMA_TABLE_SIZE 1024

typedef enum {
	TONEMAP_LINEAR, TONEMAP_REINHARD02
} ToneMapType;

class ToneMap {
public:
	ToneMap(const float gamma);
	virtual ~ToneMap() { }

	float GetGamma() const { return gamma; }

	virtual ToneMapType GetType() const = 0;
	virtual void Map(FrameBuffer *src, FrameBuffer *dst) const = 0;

protected:
	void InitGammaTable();

	float Radiance2PixelFloat(const float x) const {
		// Very slow !
		//return powf(Clamp(x, 0.f, 1.f), 1.f / 2.2f);

		const unsigned int index = Min<unsigned int>(
			Floor2UInt(GAMMA_TABLE_SIZE * Clamp(x, 0.f, 1.f)),
				GAMMA_TABLE_SIZE - 1);
		return gammaTable[index];
	}

	Spectrum Radiance2Pixel(const Spectrum& c) const {
		return Spectrum(Radiance2PixelFloat(c.r), Radiance2PixelFloat(c.g), Radiance2PixelFloat(c.b));
	}

	float gamma;
	float gammaTable[GAMMA_TABLE_SIZE];
};

class LinearToneMap : public ToneMap {
public:
	LinearToneMap(const float gamma, const float s) : ToneMap(gamma) {
		scale = s;
	}

	ToneMapType GetType() const { return TONEMAP_LINEAR; }
	void Map(FrameBuffer *src, FrameBuffer *dst) const;

	float scale;
};

class Reinhard02ToneMap : public ToneMap {
public:
	Reinhard02ToneMap(const float gamma, const float preS,
			const float postS, const float b) : ToneMap(gamma) {
		preScale = preS;
		postScale = postS;
		burn = b;
	}

	ToneMapType GetType() const { return TONEMAP_REINHARD02; }
	void Map(FrameBuffer *src, FrameBuffer *dst) const;

	float preScale, postScale, burn;
};

#endif	/* _SFERA_TONEMAP_H */
