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

#ifndef _SFERA_SDL_LIGHT_H
#define	_SFERA_SDL_LIGHT_H

#include "geometry/vector.h"
#include "pixel/spectrum.h"
#include "sdl/texmap.h"

class InfiniteLight {
public:
	InfiniteLight(TexMapInstance *tx);
	~InfiniteLight() { }

	void SetGain(const Spectrum &g) {
		gain = g;
	}

	Spectrum GetGain() const {
		return gain;
	}

	void SetShift(const float su, const float sv) {
		shiftU = su;
		shiftV = sv;
	}

	float GetShiftU() const { return shiftU; }
	float GetShiftV() const { return shiftV; }

	const TexMapInstance *GetTexture() const { return tex; }

	Spectrum Le(const Vector &dir) const;

protected:
	TexMapInstance *tex;
	float shiftU, shiftV;
	Spectrum gain;
};

#endif	/* _SFERA_SDL_LIGHT_H */
