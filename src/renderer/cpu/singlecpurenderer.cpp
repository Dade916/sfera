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

Spectrum SingleCPURenderer::SampleImage(const float u0, const float u1) {
	Ray ray;
	gameLevel->camera->GenerateRay(
		u0, u1,
		sampleFrameBuffer->GetWidth(), sampleFrameBuffer->GetHeight(),
		&ray, rnd.floatValue(), rnd.floatValue());

	const Scene &scene(*(gameLevel->scene));
	Spectrum throughput(1.f, 1.f, 1.f);
	unsigned int depth = 0;

	for(;;) {
		// Check for intersection
		bool hit = false;
		const vector<Sphere> &spheres(scene.spheres);
		size_t sphereIndex;
		for (size_t s = 0; s < spheres.size(); ++s) {
			const Sphere &sphere(spheres[s]);
			if (sphere.Intersect(&ray)) {
				hit = true;
				sphereIndex = s;
				break;
			}
		}

		if (hit) {
			if (depth > 3)
				return Spectrum();

			const Sphere &sphere(spheres[sphereIndex]);
			const Point hitPoint(ray(ray.maxt));
			const Normal N(Normalize(hitPoint - sphere.center));

			const Material &mat(*(scene.sphereMaterials[sphereIndex]));
			Vector wi;
			float pdf;
			bool specularBounce;
			const Spectrum f = mat.Sample_f(-ray.d, &wi, N, N, rnd.floatValue(), rnd.floatValue(), rnd.floatValue(),
					&pdf, specularBounce);
			if ((pdf <= 0.f) || f.Black())
				return Spectrum();
			throughput *= f / pdf;

			ray = Ray(hitPoint, wi);
			++depth;
		} else
			return throughput * scene.infiniteLight->Le(ray.d);
	}
}

void SingleCPURenderer::DrawFrame() {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

	// Render
	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			Spectrum s = SampleImage(x + rnd.floatValue() - .5f, y + rnd.floatValue() - .5f);

			//sampleFrameBuffer->SetPixel(x, y, s, 1.f);
			sampleFrameBuffer->AddPixel(x, y, s, 1.f);
		}
	}

	// Tone mapping
	gameLevel->toneMap->Map(sampleFrameBuffer, frameBuffer);

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, frameBuffer->GetPixels());
}
