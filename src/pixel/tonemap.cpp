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

#include "sfera.h"
#include "pixel/tonemap.h"
#include "pixel/framebuffer.h"

//------------------------------------------------------------------------------
// Tonemapping
//------------------------------------------------------------------------------

ToneMap::ToneMap(const float g) : gamma(g) {
	InitGammaTable();
}

void ToneMap::InitGammaTable() {
	float x = 0.f;
	const float dx = 1.f / GAMMA_TABLE_SIZE;
	for (unsigned int i = 0; i < GAMMA_TABLE_SIZE; ++i, x += dx)
		gammaTable[i] = powf(Clamp(x, 0.f, 1.f), 1.f / gamma);
}

//------------------------------------------------------------------------------
// Linear tonemapping
//------------------------------------------------------------------------------

void LinearToneMap::Map(FrameBuffer *src, FrameBuffer *dst) const {
	assert (src->GetWidth() == dst->GetWidth());
	assert (src->GetHeight() == dst->GetHeight());

	const unsigned int pixelCount = src->GetWidth() * src->GetHeight();
	const Pixel *samplePixels = src->GetPixels();
	Pixel *pixels = dst->GetPixels();

	for (unsigned int i = 0; i < pixelCount; ++i) {
		*pixels = Radiance2Pixel(scale * (*samplePixels));

		++samplePixels;
		++pixels;
	}
}

//------------------------------------------------------------------------------
// Reinhard02 tonemapping
//------------------------------------------------------------------------------

void Reinhard02ToneMap::Map(FrameBuffer *src, FrameBuffer *dst) const {
	assert (src->GetWidth() == dst->GetWidth());
	assert (src->GetHeight() == dst->GetHeight());

	const unsigned int pixelCount = src->GetWidth() * src->GetHeight();
	const Pixel *samplePixels = src->GetPixels();
	Pixel *pixels = dst->GetPixels();

	const float alpha = .1f;

	// Use the frame buffer as temporary storage and calculate the avarage luminance
	float Ywa = 0.f;
	for (unsigned int i = 0; i < pixelCount; ++i) {
		// Convert to XYZ color space
		pixels->r = 0.412453f * samplePixels->r + 0.357580f * samplePixels->g + 0.180423f * samplePixels->b;
		pixels->g = 0.212671f * samplePixels->r + 0.715160f * samplePixels->g + 0.072169f * samplePixels->b;
		pixels->b = 0.019334f * samplePixels->r + 0.119193f * samplePixels->g + 0.950227f * samplePixels->b;

		Ywa += pixels->g;

		++samplePixels;
		++pixels;
	}
	Ywa /= pixelCount;

	// Avoid division by zero
	if (Ywa == 0.f)
		Ywa = 1.f;

	const float Yw = preScale * alpha * burn;
	const float invY2 = 1.f / (Yw * Yw);
	const float pScale = postScale * preScale * alpha / Ywa;

	pixels = dst->GetPixels();
	for (unsigned int i = 0; i < pixelCount; ++i) {
		Spectrum xyz = *pixels;

		const float ys = xyz.g;
		xyz *= pScale * (1.f + ys * invY2) / (1.f + ys);

		// Convert back to RGB color space
		pixels->r =  3.240479f * xyz.r - 1.537150f * xyz.g - 0.498535f * xyz.b;
		pixels->g = -0.969256f * xyz.r + 1.875991f * xyz.g + 0.041556f * xyz.b;
		pixels->b =  0.055648f * xyz.r - 0.204043f * xyz.g + 1.057311f * xyz.b;

		// Gamma correction
		*pixels = Radiance2Pixel(*pixels);

		++pixels;
	}
}
