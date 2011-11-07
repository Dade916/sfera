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

#ifndef _SFERA_PIXEL_H
#define _SFERA_PIXEL_H

#include "pixel/spectrum.h"

typedef struct {
	Spectrum radiance;
	float weight;
} SamplePixel;

class SampleFrameBuffer {
public:
	SampleFrameBuffer(const unsigned int w, const unsigned int h)
		: width(w), height(h) {
		pixels = new SamplePixel[width * height];

		Clear();
	}
	~SampleFrameBuffer() {
		delete[] pixels;
	}

	void Clear() {
		for (unsigned int i = 0; i < width * height; ++i) {
			pixels[i].radiance.r = 0.f;
			pixels[i].radiance.g = 0.f;
			pixels[i].radiance.b = 0.f;
			pixels[i].weight = 0.f;
		}
	};

	SamplePixel *GetPixels() const { return pixels; }

	void BlendPixel(const unsigned int x, const unsigned int y, const Spectrum& r, const float w) {
		assert (x >= 0);
		assert (x < width);
		assert (y >= 0);
		assert (y < height);

		SamplePixel *pixel = &pixels[x + y * width];
		pixel->radiance = (1.f - w) * pixel->radiance + w * r;
		pixel->weight = 1.f;
	}

	void AddPixel(const unsigned int x, const unsigned int y, const Spectrum& r, const float w) {
		assert (x >= 0);
		assert (x < width);
		assert (y >= 0);
		assert (y < height);

		SamplePixel *pixel = &pixels[x + y * width];
		pixel->radiance += r;
		pixel->weight += w;
	}

	void AddPixel(const unsigned int index, const Spectrum& r, const float w) {
		assert (index >= 0);
		assert (index < width * height);

		pixels[index].radiance += r;
		pixels[index].weight += w;
	}

	void SetPixel(const unsigned int x, const unsigned int y, const Spectrum& r, const float w) {
		assert (x >= 0);
		assert (x < width);
		assert (y >= 0);
		assert (y < height);

		SamplePixel *pixel = &pixels[x + y * width];
		pixel->radiance = r;
		pixel->weight = w;
	}

	void SetPixel(const unsigned int index, const Spectrum& r, const float w) {
		assert (index >= 0);
		assert (index < width * height);

		pixels[index].radiance = r;
		pixels[index].weight = w;
	}

	SamplePixel *GetPixel(const unsigned int x, const unsigned int y) const {
		assert (x >= 0);
		assert (x < width);
		assert (y >= 0);
		assert (y < height);

		return &pixels[x + y * width];
	}

	SamplePixel *GetPixel(const unsigned int index) const {
		assert (index >= 0);
		assert (index < width * height);

		return &pixels[index];
	}

	unsigned int GetWidth() const { return width; }
	unsigned int GetHeight() const { return height; }

private:
	const unsigned int width, height;

	SamplePixel *pixels;
};

typedef Spectrum Pixel;

class FrameBuffer {
public:
	FrameBuffer(const unsigned int w, const unsigned int h)
			: width(w), height(h) {
		pixels = new Pixel[width * height];

		Clear();
	}
	~FrameBuffer() {
		delete[] pixels;
	}

	void Clear() {
		for (unsigned int i = 0; i < width * height; ++i) {
			pixels[i].r = 0.f;
			pixels[i].g = 0.f;
			pixels[i].b = 0.f;
		}
	};

	Pixel *GetPixels() const { return pixels; }

	void SetPixel(const unsigned int x, const unsigned int y, const Spectrum& r) {
		assert (x >= 0);
		assert (x < width);
		assert (y >= 0);
		assert (y < height);

		pixels[x + y * width] = r;
	}

	void SetPixel(const unsigned int index, const Spectrum& r) {
		assert (index >= 0);
		assert (index < width * height);

		pixels[index] = r;
	}

	void AddPixel(const unsigned int x, const unsigned int y, const Spectrum& r) {
		assert (x >= 0);
		assert (x < width);
		assert (y >= 0);
		assert (y < height);

		pixels[x + y * width] += r;
	}

	void BlendPixel(const unsigned int x, const unsigned int y, const Spectrum& r, const float w) {
		assert (x >= 0);
		assert (x < width);
		assert (y >= 0);
		assert (y < height);

		Pixel *pixel = &pixels[x + y * width];
		*pixel = (1.f - w) * (*pixel) + w * r;
	}

	Pixel *GetPixel(const unsigned int x, const unsigned int y) const {
		assert (x >= 0);
		assert (x < width);
		assert (y >= 0);
		assert (y < height);

		return &pixels[x + y * width];
	}

	Pixel *GetPixel(const unsigned int index) const {
		assert (index >= 0);
		assert (index < width * height);

		return &pixels[index];
	}

	unsigned int GetWidth() const { return width; }
	unsigned int GetHeight() const { return height; }

	static void ApplyBoxFilter(Pixel *frameBuffer, Pixel *tmpFrameBuffer,
		const unsigned int width, const unsigned int height, const unsigned int radius);
	static void ApplyBlurLightFilter(Pixel *frameBuffer, Pixel *tmpFrameBuffer,
		const unsigned int width, const unsigned int height);
	static void ApplyBlurHeavyFilter(Pixel *frameBuffer, Pixel *tmpFrameBuffer,
		const unsigned int width, const unsigned int height);

private:
	static void ApplyBoxFilterX(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height, const unsigned int radius);
	static void ApplyBoxFilterY(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height, const unsigned int radius);
	static void ApplyBoxFilterXR1(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height);
	static void ApplyBoxFilterYR1(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height);
	static void ApplyBlurLightFilterXR1(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height);
	static void ApplyBlurLightFilterYR1(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height);
	static void ApplyBlurHeavyFilterXR1(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height);
	static void ApplyBlurHeavyFilterYR1(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height);

	const unsigned int width, height;

	Pixel *pixels;
};

#endif	/* _SFERA_PIXEL_H */
