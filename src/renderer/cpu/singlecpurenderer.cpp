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

#include "renderer/cpu/singlecpurenderer.h"

SingleCPURenderer::SingleCPURenderer(const GameLevel *level) :
	LevelRenderer(level), rnd(1) {
	sampleFrameBuffer = new SampleFrameBuffer(
			gameLevel->gameConfig->GetScreenWidth(),
			gameLevel->gameConfig->GetScreenHeight());
	frameBuffer = new FrameBuffer(
			gameLevel->gameConfig->GetScreenWidth(),
			gameLevel->gameConfig->GetScreenHeight());

	sampleFrameBuffer->Clear();
	frameBuffer->Clear();
}

SingleCPURenderer::~SingleCPURenderer() {
	delete sampleFrameBuffer;
	delete frameBuffer;
}

void SingleCPURenderer::DrawFrame() {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

	// Render

	const PerspectiveCamera &camera(*(gameLevel->camera));
	const vector<Sphere> &spheres(gameLevel->scene->spheres);

	Ray ray;
	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			const float rx = x + rnd.floatValue() - .5f;
			const float ry = y + rnd.floatValue() - .5f;
			camera.GenerateRay(rx, ry, width, height,
					&ray, rnd.floatValue(), rnd.floatValue());

			bool hit = false;
			for (size_t s = 0; s < spheres.size(); ++s) {
				if (spheres[s].Intersect(&ray)) {
					hit = true;
					break;
				}
			}

			if (hit)
				sampleFrameBuffer->SetPixel(x, y, Spectrum(1.f, 1.f, 1.f), 1.f);
			else {
				const InfiniteLight &infiniteLight(*(gameLevel->scene->infiniteLight));

				const Spectrum Le = infiniteLight.Le(ray.d);

				sampleFrameBuffer->SetPixel(x, y, Le, 1.f);
			}
		}
	}

	// Tone mapping
	gameLevel->toneMap->Map(sampleFrameBuffer, frameBuffer);

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, frameBuffer->GetPixels());
}
