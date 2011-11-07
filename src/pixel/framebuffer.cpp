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

#include "pixel/framebuffer.h"

void FrameBuffer::ApplyBoxFilterX(const Pixel *src, Pixel *dest,
	const unsigned int width, const unsigned int height, const unsigned int radius) {
    const float scale = 1.0f / (float)((radius << 1) + 1);

    // Do left edge
    Pixel t = src[0] * radius;
    for (unsigned int x = 0; x < (radius + 1); ++x)
        t += src[x];
    dest[0] = t * scale;

    for (unsigned int x = 1; x < (radius + 1); ++x) {
        t += src[x + radius];
        t -= src[0];
        dest[x] = t * scale;
    }

    // Main loop
    for (unsigned int x = (radius + 1); x < width - radius; ++x) {
        t += src[x + radius];
        t -= src[x - radius - 1];
        dest[x] = t * scale;
    }

    // Do right edge
    for (unsigned int x = width - radius; x < width; ++x) {
        t += src[width - 1];
        t -= src[x - radius - 1];
        dest[x] = t * scale;
    }
}

void FrameBuffer::ApplyBoxFilterY(const Pixel *src, Pixel *dst,
	const unsigned int width, const unsigned int height, const unsigned int radius) {
    const float scale = 1.0f / (float)((radius << 1) + 1);

    // Do left edge
    Pixel t = src[0] * radius;
    for (unsigned int y = 0; y < (radius + 1); ++y) {
        t += src[y * width];
    }
    dst[0] = t * scale;

    for (unsigned int y = 1; y < (radius + 1); ++y) {
        t += src[(y + radius) * width];
        t -= src[0];
        dst[y * width] = t * scale;
    }

    // Main loop
    for (unsigned int y = (radius + 1); y < (height - radius); ++y) {
        t += src[(y + radius) * width];
        t -= src[((y - radius) * width) - width];
        dst[y * width] = t * scale;
    }

    // Do right edge
    for (unsigned int y = height - radius; y < height; ++y) {
        t += src[(height - 1) * width];
        t -= src[((y - radius) * width) - width];
        dst[y * width] = t * scale;
    }
}

void FrameBuffer::ApplyBoxFilterXR1(const Pixel *src, Pixel *dest,
	const unsigned int width, const unsigned int height) {
    const float scale = 1.f / 3.f;

    // Do left edge
    Pixel t = 2.f * src[0];
	t += src[1];
    dest[0] = t * scale;

	t += src[2];
	t -= src[0];
	dest[1] = t * scale;

    // Main loop
    for (unsigned int x = 2; x < width - 1; ++x) {
        t += src[x + 1];
        t -= src[x - 2];
        dest[x] = t * scale;
    }

    // Do right edge
	t += src[width - 1];
	t -= src[width - 3];
	dest[width - 1] = t * scale;
}

void FrameBuffer::ApplyBoxFilterYR1(const Pixel *src, Pixel *dst,
	const unsigned int width, const unsigned int height) {
    const float scale = 1.f / 3.f;

    // Do left edge
	Pixel t = 2.f * src[0];
	t += src[width];
	dst[0] = t * scale;

	t += src[2 * width];
	t -= src[0];
	dst[width] = t * scale;

    // Main loop
    for (unsigned int y = 2; y < height - 1; ++y) {
        t += src[(y + 1) * width];
        t -= src[((y - 1) * width) - width];
        dst[y * width] = t * scale;
    }

    // Do right edge
	t += src[(height - 1) * width];
	t -= src[((height - 2) * width) - width];
	dst[(height - 1) * width] = t * scale;
}

void FrameBuffer::ApplyBlurLightFilterXR1(const Pixel *src, Pixel *dst,
	const unsigned int width, const unsigned int height) {
	const float aF = .15f;
	const float bF = 1.f;
	const float cF = .15f;

	// Do left edge
	Pixel a = 0.f;
	Pixel b = src[0];
	Pixel c = src[1];

	const float leftTotF = bF + cF;
	const float bLeftK = bF / leftTotF;
	const float cLeftK = cF / leftTotF;
	dst[0] = bLeftK  * b + cLeftK * c;

    // Main loop
	const float totF = aF + bF + cF;
	const float aK = aF / totF;
	const float bK = bF / totF;
	const float cK = cF / totF;

	for (unsigned int x = 1; x < width - 1; ++x) {
		a = b;
		b = c;
		c = src[x + 1];

		dst[x] = aK * a + bK * b + cK * c;
    }

    // Do right edge
	const float rightTotF = aF + bF;
	const float aRightK = aF / rightTotF;
	const float bRightK = bF / rightTotF;
	a = b;
	b = c;
	dst[width - 1] = aRightK * a + bRightK * b;
}

void FrameBuffer::ApplyBlurLightFilterYR1(const Pixel *src, Pixel *dst,
	const unsigned int width, const unsigned int height) {
	const float aF = .15f;
	const float bF = 1.f;
	const float cF = .15f;

	// Do left edge
	Pixel a = 0.f;
	Pixel b = src[0];
	Pixel c = src[width];

	const float leftTotF = bF + cF;
	const float bLeftK = bF / leftTotF;
	const float cLeftK = cF / leftTotF;
	dst[0] = bLeftK  * b + cLeftK * c;

    // Main loop
	const float totF = aF + bF + cF;
	const float aK = aF / totF;
	const float bK = bF / totF;
	const float cK = cF / totF;

    for (unsigned int y = 1; y < height - 1; ++y) {
		a = b;
		b = c;
		c = src[(y + 1) * width];

		dst[y * width] = aK * a + bK * b + cK * c;
    }

    // Do right edge
	const float rightTotF = aF + bF;
	const float aRightK = aF / rightTotF;
	const float bRightK = bF / rightTotF;
	a = b;
	b = c;
	dst[(height - 1) * width] = aRightK * a + bRightK * b;
}

void FrameBuffer::ApplyBlurHeavyFilterXR1(const Pixel *src, Pixel *dst,
	const unsigned int width, const unsigned int height) {
	const float aF = .35f;
	const float bF = 1.f;
	const float cF = .35f;

	// Do left edge
	Pixel a = 0.f;
	Pixel b = src[0];
	Pixel c = src[1];

	const float leftTotF = bF + cF;
	const float bLeftK = bF / leftTotF;
	const float cLeftK = cF / leftTotF;
	dst[0] = bLeftK  * b + cLeftK * c;

    // Main loop
	const float totF = aF + bF + cF;
	const float aK = aF / totF;
	const float bK = bF / totF;
	const float cK = cF / totF;

	for (unsigned int x = 1; x < width - 1; ++x) {
		a = b;
		b = c;
		c = src[x + 1];

		dst[x] = aK * a + bK * b + cK * c;
    }

    // Do right edge
	const float rightTotF = aF + bF;
	const float aRightK = aF / rightTotF;
	const float bRightK = bF / rightTotF;
	a = b;
	b = c;
	dst[width - 1] = aRightK * a + bRightK * b;
}

void FrameBuffer::ApplyBlurHeavyFilterYR1(const Pixel *src, Pixel *dst,
	const unsigned int width, const unsigned int height) {
	const float aF = .35f;
	const float bF = 1.f;
	const float cF = .35f;

	// Do left edge
	Pixel a = 0.f;
	Pixel b = src[0];
	Pixel c = src[width];

	const float leftTotF = bF + cF;
	const float bLeftK = bF / leftTotF;
	const float cLeftK = cF / leftTotF;
	dst[0] = bLeftK  * b + cLeftK * c;

    // Main loop
	const float totF = aF + bF + cF;
	const float aK = aF / totF;
	const float bK = bF / totF;
	const float cK = cF / totF;

    for (unsigned int y = 1; y < height - 1; ++y) {
		a = b;
		b = c;
		c = src[(y + 1) * width];

		dst[y * width] = aK * a + bK * b + cK * c;
    }

    // Do right edge
	const float rightTotF = aF + bF;
	const float aRightK = aF / rightTotF;
	const float bRightK = bF / rightTotF;
	a = b;
	b = c;
	dst[(height - 1) * width] = aRightK * a + bRightK * b;
}

void FrameBuffer::ApplyBoxFilter(Pixel *frameBuffer, Pixel *tmpFrameBuffer,
	const unsigned int width, const unsigned int height, const unsigned int radius) {
	if (radius == 1) {
		for (unsigned int i = 0; i < height; ++i)
			ApplyBoxFilterXR1(&frameBuffer[i * width], &tmpFrameBuffer[i * width], width, height);

		for (unsigned int i = 0; i < width; ++i)
			ApplyBoxFilterYR1(&tmpFrameBuffer[i], &frameBuffer[i], width, height);
	} else {
		for (unsigned int i = 0; i < height; ++i)
			ApplyBoxFilterX(&frameBuffer[i * width], &tmpFrameBuffer[i * width], width, height, radius);

		for (unsigned int i = 0; i < width; ++i)
			ApplyBoxFilterY(&tmpFrameBuffer[i], &frameBuffer[i], width, height, radius);
	}
}

void FrameBuffer::ApplyBlurLightFilter(Pixel *frameBuffer, Pixel *tmpFrameBuffer,
	const unsigned int width, const unsigned int height) {
	for (unsigned int i = 0; i < height; ++i)
		ApplyBlurLightFilterXR1(&frameBuffer[i * width], &tmpFrameBuffer[i * width], width, height);

	for (unsigned int i = 0; i < width; ++i)
		ApplyBlurLightFilterYR1(&tmpFrameBuffer[i], &frameBuffer[i], width, height);
}

void FrameBuffer::ApplyBlurHeavyFilter(Pixel *frameBuffer, Pixel *tmpFrameBuffer,
	const unsigned int width, const unsigned int height) {
	for (unsigned int i = 0; i < height; ++i)
		ApplyBlurHeavyFilterXR1(&frameBuffer[i * width], &tmpFrameBuffer[i * width], width, height);

	for (unsigned int i = 0; i < width; ++i)
		ApplyBlurHeavyFilterYR1(&tmpFrameBuffer[i], &frameBuffer[i], width, height);
}
