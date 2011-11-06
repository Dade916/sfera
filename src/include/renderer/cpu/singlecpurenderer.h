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

#ifndef _SFERA_CPURENDERER_H
#define	_SFERA_CPURENDERER_H

#include "utils/randomgen.h"
#include "renderer/levelrenderer.h"
#include "pixel/framebuffer.h"
#include "acceleretor/acceleretor.h"

class SingleCPURenderer : public LevelRenderer {
public:
	SingleCPURenderer(const GameLevel *level);
	~SingleCPURenderer();

	void DrawFrame(const EditActionList &editActionList);

private:
	static void ApplyBoxFilterX(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height, const unsigned int radius);
	static void ApplyBoxFilterY(const Pixel *src, Pixel *dst,
		const unsigned int width, const unsigned int height, const unsigned int radius);
	static void ApplyBoxFilter(Pixel *frameBuffer, Pixel *tmpFrameBuffer,
		const unsigned int width, const unsigned int height, const unsigned int radius);

	Spectrum SampleImage(const Accelerator &accel, const float u0, const float u1);

	RandomGenerator rnd;

	FrameBuffer *passFrameBuffer;
	FrameBuffer *filterFrameBuffer;
	FrameBuffer *frameBuffer;
	FrameBuffer *toneMapFrameBuffer;

	double timeSinceLastCameraEdit, timeSinceLastNoCameraEdit;
};

#endif	/* _SFERA_CPURENDERER_H */
