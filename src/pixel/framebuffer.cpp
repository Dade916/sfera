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

void FrameBuffer::ApplyBoxFilter(Pixel *frameBuffer, Pixel *tmpFrameBuffer,
	const unsigned int width, const unsigned int height, const unsigned int radius) {
	for (unsigned int i = 0; i < height; ++i)
		ApplyBoxFilterX(&frameBuffer[i * width], &tmpFrameBuffer[i * width], width, height, radius);

	for (unsigned int i = 0; i < width; ++i)
		ApplyBoxFilterY(&tmpFrameBuffer[i], &frameBuffer[i], width, height, radius);
}
