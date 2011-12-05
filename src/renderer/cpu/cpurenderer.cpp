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
#include "renderer/cpu/cpurenderer.h"

CPURenderer::CPURenderer(GameLevel *level) :
	LevelRenderer(level), timeSinceLastCameraEdit(WallClockTime()),
		timeSinceLastNoCameraEdit(WallClockTime()) {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

	passFrameBuffer = new FrameBuffer(width, height);
	tmpFrameBuffer = new FrameBuffer(width, height);
	frameBuffer = new FrameBuffer(width, height);
	toneMapFrameBuffer = new FrameBuffer(width, height);

	passFrameBuffer->Clear();
	tmpFrameBuffer->Clear();
	frameBuffer->Clear();
	toneMapFrameBuffer->Clear();
}

CPURenderer::~CPURenderer() {
	delete passFrameBuffer;
	delete tmpFrameBuffer;
	delete frameBuffer;
	delete toneMapFrameBuffer;
}

BVHAccel *CPURenderer::BuildAcceleretor() {
	//--------------------------------------------------------------------------
	// Build the Accelerator
	//--------------------------------------------------------------------------

	const int treeType = 4; // Tree type to generate (2 = binary, 4 = quad, 8 = octree)
	const int isectCost = 80;
	const int travCost = 10;
	const float emptyBonus = 0.5f;

	const vector<GameSphere> &spheres(gameLevel->scene->spheres);
	vector<const Sphere *> sphereList(gameLevel->scene->spheres.size() + GAMEPLAYER_PUPPET_SIZE);
	for (size_t s = 0; s < spheres.size(); ++s)
		sphereList[s] = &(spheres[s].sphere);
	for (size_t s = 0; s < GAMEPLAYER_PUPPET_SIZE; ++s) {
		const Sphere *puppet = &(gameLevel->player->puppet[s]);

		sphereList[s + spheres.size()] = puppet;
	}

	return new BVHAccel(sphereList, treeType, isectCost, travCost, emptyBonus);
}

Spectrum CPURenderer::SampleImage(
		RandomGenerator &rnd,
		const Accelerator &accel, const PerspectiveCamera &camera,
		const float screenX, const float screenY) {
	Ray ray;
	camera.GenerateRay(
		screenX, screenY,
		gameLevel->gameConfig->GetScreenWidth(), gameLevel->gameConfig->GetScreenHeight(),
		&ray, rnd.floatValue(), rnd.floatValue());

	const Scene &scene(*(gameLevel->scene));
	Spectrum throughput(1.f, 1.f, 1.f);
	Spectrum radiance(0.f, 0.f, 0.f);

	unsigned int diffuseBounces = 0;
	const unsigned int maxDiffuseBounces = gameLevel->maxPathDiffuseBounces;
	unsigned int specularGlossyBounces = 0;
	const unsigned int maxSpecularGlossyBounces = gameLevel->maxPathSpecularGlossyBounces;

	const vector<GameSphere> &spheres(scene.spheres);
	for(;;) {
		// Check for intersection with objects
		Sphere *hitSphere;
		unsigned int sphereIndex;
		if (accel.Intersect(&ray, &hitSphere, &sphereIndex)) {
			const Material *hitMat;
			const TexMapInstance *texMap;
			const BumpMapInstance *bumpMap;

			if (sphereIndex >= spheres.size()) {
				// I'm hitting the puppet
				const unsigned int puppetIndex = sphereIndex - spheres.size();
				
				hitMat = gameLevel->player->puppetMaterial[puppetIndex];
				texMap = NULL;
				bumpMap = NULL;
			} else {
				hitMat = scene.sphereMaterials[sphereIndex];
				texMap = scene.sphereTexMaps[sphereIndex];
				bumpMap = scene.sphereBumpMaps[sphereIndex];
			}

			const Point hitPoint(ray(ray.maxt));
			Normal N(Normalize(hitPoint - hitSphere->center));

			// Apply bump mapping
			Normal shadeN;
			if (bumpMap)
				shadeN = bumpMap->SphericalMap(Vector(N), N);
			else
				shadeN = N;

			// Check if I have to flip the normal
			shadeN = (Dot(Vector(N), ray.d) > 0.f) ? (-shadeN) : shadeN;

			radiance += throughput * hitMat->GetEmission();

			Vector wi;
			float pdf;
			bool diffuseBounce;
			Spectrum f = hitMat->Sample_f(rnd, -ray.d, &wi, N, shadeN, &pdf, diffuseBounce);
			if ((pdf <= 0.f) || f.Black())
				return radiance;

			if (diffuseBounce) {
				++diffuseBounces;

				if (diffuseBounces > maxDiffuseBounces)
					return radiance;
			} else {
				++specularGlossyBounces;

				if (specularGlossyBounces > maxSpecularGlossyBounces)
					return radiance;
			}

			// Apply texture map
			if (texMap)
				f *= texMap->SphericalMap(Vector(N));

			throughput *= f;

			ray = Ray(hitPoint, wi);
		} else
			return radiance + throughput * scene.infiniteLight->Le(ray.d);
	}
}

void CPURenderer::ApplyFilter() {
	//--------------------------------------------------------------------------
	// Apply a filter: approximated by applying a box filter multiple times
	//--------------------------------------------------------------------------

	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();

	switch (gameConfig.GetRendererFilterType()) {
		case NO_FILTER:
			break;
		case BLUR_LIGHT: {
			const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
			for (unsigned int i = 0; i < filterPassCount; ++i)
				FrameBuffer::ApplyBlurLightFilter(passFrameBuffer->GetPixels(), tmpFrameBuffer->GetPixels(),
						width, height);
			break;
		}
		case BLUR_HEAVY: {
			const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
			for (unsigned int i = 0; i < filterPassCount; ++i)
				FrameBuffer::ApplyBlurHeavyFilter(passFrameBuffer->GetPixels(), tmpFrameBuffer->GetPixels(),
						width, height);
			break;
		}
		case BOX: {
			const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
			for (unsigned int i = 0; i < filterPassCount; ++i)
				FrameBuffer::ApplyBoxFilter(passFrameBuffer->GetPixels(), tmpFrameBuffer->GetPixels(),
						width, height, gameConfig.GetRendererFilterRaidus());
			break;
		}
	}
}

void CPURenderer::BlendFrame() {
	//--------------------------------------------------------------------------
	// Blend the new frame with the old one
	//--------------------------------------------------------------------------

	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();

	const float ghostTimeLength = gameConfig.GetRendererGhostFactorTime();
	float k;
	if (gameLevel->camera->IsChangedSinceLastUpdate()) {
		timeSinceLastCameraEdit = WallClockTime();

		const double dt = Min<double>(WallClockTime() - timeSinceLastNoCameraEdit, ghostTimeLength);
		k = 1.f - dt / ghostTimeLength;
	} else {
		timeSinceLastNoCameraEdit = WallClockTime();

		const double dt = Min<double>(WallClockTime() - timeSinceLastCameraEdit, ghostTimeLength);
		k = dt / ghostTimeLength;
	}

	const float blendFactor = (1.f - k) * gameConfig.GetRendererGhostFactorCameraEdit() +
		k * gameConfig.GetRendererGhostFactorNoCameraEdit();

	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			const Pixel *p = passFrameBuffer->GetPixel(x, y);
			frameBuffer->BlendPixel(x, y, *p, blendFactor);
		}
	}
}

void CPURenderer::ApplyToneMapping() {
	//--------------------------------------------------------------------------
	// Tone mapping
	//--------------------------------------------------------------------------

	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();

	gameLevel->toneMap->Map(frameBuffer, toneMapFrameBuffer);

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, toneMapFrameBuffer->GetPixels());
}

void CPURenderer::CopyFrame() {
	//--------------------------------------------------------------------------
	// Copy the frame
	//--------------------------------------------------------------------------

	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, toneMapFrameBuffer->GetPixels());
}