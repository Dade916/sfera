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
#include "sdl/editaction.h"
#include "acceleretor/bvhaccel.h"
#include "renderer/cpu/singlecpurenderer.h"

SingleCPURenderer::SingleCPURenderer(GameLevel *level) : CPURenderer(level),
		rnd(1) {
}

SingleCPURenderer::~SingleCPURenderer() {
}

size_t SingleCPURenderer::DrawFrame() {
	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();
	const unsigned int samplePerPass = gameConfig.GetRendererSamplePerPass();

	BVHAccel *accel;
	PerspectiveCamera cameraCopy;

	{
		boost::unique_lock<boost::mutex> lock(gameLevel->levelMutex);

		//----------------------------------------------------------------------
		// Build the Accelerator
		//----------------------------------------------------------------------

		accel = BuildAcceleretor();

		//----------------------------------------------------------------------
		// Copy the Camera
		//----------------------------------------------------------------------

		memcpy(&cameraCopy, gameLevel->camera, sizeof(PerspectiveCamera));
	}

	//----------------------------------------------------------------------
	// Render
	//----------------------------------------------------------------------

	const float sampleScale = 1.f / samplePerPass;
	for (unsigned int i = 0; i < samplePerPass; ++i) {
		for (unsigned int y = 0; y < height; ++y) {
			for (unsigned int x = 0; x < width; ++x) {
				Spectrum s = SampleImage(rnd, *accel, cameraCopy,
						x + rnd.floatValue() - .5f, y + rnd.floatValue() - .5f) *
						sampleScale;

				if (i == 0)
					passFrameBuffer->SetPixel(x, y, s);
				else
					passFrameBuffer->AddPixel(x, y, s);
			}
		}
	}

	delete accel;

	//--------------------------------------------------------------------------
	// Apply a filter: approximated by applying a box filter multiple times
	//--------------------------------------------------------------------------

	ApplyFilter();

	//--------------------------------------------------------------------------
	// Blend the new frame with the old one
	//--------------------------------------------------------------------------

	BlendFrame();

	//--------------------------------------------------------------------------
	// Tone mapping
	//--------------------------------------------------------------------------

	ApplyToneMapping();
	CopyFrame();


	return samplePerPass * width * height;
}
