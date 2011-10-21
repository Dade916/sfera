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
	const size_t pixelsCount = gameLevel->gameConfig->GetScreenWidth() * gameLevel->gameConfig->GetScreenHeight();
	pixels = new float[pixelsCount * 3];

	float *p = pixels;
	for (size_t i = 0; i < pixelsCount * 3; ++i)
		*p++ = 0.f;
}

SingleCPURenderer::~SingleCPURenderer() {
	delete[] pixels;
}

void SingleCPURenderer::DrawFrame() {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

	const PerspectiveCamera &camera(*(gameLevel->camera));
	const vector<Sphere> &spheres(gameLevel->scene->spheres);

	Ray ray;
	float *p = pixels;
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

			if (hit) {
				*p++ = 1.f;
				*p++ = 1.f;
				*p++ = 1.f;
			} else {
				*p++ = 0.f;
				*p++ = 0.f;
				*p++ = 0.f;
			}
		}
	}

	glDrawPixels(gameLevel->gameConfig->GetScreenWidth(), gameLevel->gameConfig->GetScreenHeight(), GL_RGB, GL_FLOAT, pixels);
}
