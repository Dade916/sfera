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

#include "sdl/editaction.h"

#include "acceleretor/bvhaccel.h"
#include "renderer/cpu/singlecpurenderer.h"
#include "renderer/cpu/multicpurenderer.h"

//------------------------------------------------------------------------------
// MultiCPURenderer
//------------------------------------------------------------------------------

MultiCPURenderer::MultiCPURenderer(const GameLevel *level) :
	LevelRenderer(level), timeSinceLastCameraEdit(WallClockTime()),
		timeSinceLastNoCameraEdit(WallClockTime()) {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

	threadCount = boost::thread::hardware_concurrency();

	// Initialize all frame buffers
	for (size_t i = 0; i < threadCount; ++i) {
		passFrameBuffer.push_back(new FrameBuffer(width, height));
		filterFrameBuffer.push_back(new FrameBuffer(width, height));

		passFrameBuffer[i]->Clear();
		filterFrameBuffer[i]->Clear();
	}

	frameBuffer = new FrameBuffer(width, height);
	toneMapFrameBuffer = new FrameBuffer(width, height);

	frameBuffer->Clear();
	toneMapFrameBuffer->Clear();

	// Create synchronization barrier
	barrier = new boost::barrier(threadCount + 1);

	// Start all threads
	for (size_t i = 0; i < threadCount; ++i) {
		MultiCPURendererThread *t = new MultiCPURendererThread(i, this);
		renderThread.push_back(t);

		t->Start();
	}
}

MultiCPURenderer::~MultiCPURenderer() {
	for (size_t i = 0; i < threadCount; ++i) {
		renderThread[i]->Stop();
		delete renderThread[i];
	}

	for (size_t i = 0; i < threadCount; ++i) {
		delete passFrameBuffer[i];
		delete filterFrameBuffer[i];
	}

	delete frameBuffer;
	delete toneMapFrameBuffer;
}

void MultiCPURenderer::DrawFrame(const EditActionList &editActionList) {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

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

	accel = new BVHAccel(sphereList, treeType, isectCost, travCost, emptyBonus);

	//--------------------------------------------------------------------------
	// Render & filter
	//--------------------------------------------------------------------------

	barrier->wait();
	// Other threads do the rendering
	barrier->wait();

	//--------------------------------------------------------------------------
	// Merge all thread frames
	//--------------------------------------------------------------------------

	const float invThreadCount = 1.f / threadCount;
	for (size_t i = 0; i < threadCount; ++i) {
		for (unsigned int y = 0; y < height; ++y) {
			for (unsigned int x = 0; x < width; ++x) {
				Pixel p = *(passFrameBuffer[i]->GetPixel(x, y));
				p *= invThreadCount;
				passFrameBuffer[0]->AddPixel(x, y, p);
			}
		}
	}

	//--------------------------------------------------------------------------
	// Blend the new frame with the old one
	//--------------------------------------------------------------------------

	float k;
	if (editActionList.Has(CAMERA_EDIT)) {
		timeSinceLastCameraEdit = WallClockTime();

		const double dt = Min(WallClockTime() - timeSinceLastNoCameraEdit, 2.0);
		k = 1.f - dt / 2.f;
	} else {
		timeSinceLastNoCameraEdit = WallClockTime();

		const double dt = Min(WallClockTime() - timeSinceLastCameraEdit, 5.0);
		k = dt / 5.f;
	}

	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const float blendFactor = (1.f - k) * gameConfig.GetRendererGhostFactorCameraEdit() +
		k * gameConfig.GetRendererGhostFactorNoCameraEdit();

	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			const Pixel *p = passFrameBuffer[0]->GetPixel(x, y);
			frameBuffer->BlendPixel(x, y, *p, blendFactor);
		}
	}

	//--------------------------------------------------------------------------
	// Tone mapping
	//--------------------------------------------------------------------------

	gameLevel->toneMap->Map(frameBuffer, toneMapFrameBuffer);

	glDrawPixels(width, height, GL_RGB, GL_FLOAT, toneMapFrameBuffer->GetPixels());

	delete accel;
}

//------------------------------------------------------------------------------
// MultiCPURendererThread
//------------------------------------------------------------------------------

MultiCPURendererThread::MultiCPURendererThread(const size_t threadIndex, MultiCPURenderer *multiCPURenderer) :
	rnd(threadIndex + 1) {
	index = threadIndex;
	renderer = multiCPURenderer;
	renderThread = NULL;
}

MultiCPURendererThread::~MultiCPURendererThread() {
}

void MultiCPURendererThread::Start() {
	renderThread = new boost::thread(boost::bind(MultiCPURendererThread::MultiCPURenderThreadImpl, this));
}

void MultiCPURendererThread::Stop() {
	if (renderThread) {
		renderThread->interrupt();
		renderThread->join();
		delete renderThread;
		renderThread = NULL;
	}
}

void MultiCPURendererThread::MultiCPURenderThreadImpl(MultiCPURendererThread *renderThread) {
	try {
		const size_t index = renderThread->index;
		RandomGenerator &rnd(renderThread->rnd);
		const GameLevel *gameLevel = renderThread->renderer->gameLevel;
		FrameBuffer *passFrameBuffer = renderThread->renderer->passFrameBuffer[index];
		FrameBuffer *filterFrameBuffer = renderThread->renderer->filterFrameBuffer[index];

		const GameConfig &gameConfig(*(gameLevel->gameConfig));
		const unsigned int width = gameConfig.GetScreenWidth();
		const unsigned int height = gameConfig.GetScreenHeight();
		const unsigned int samplePerPass = gameConfig.GetRendererSamplePerPass();

		while (!boost::this_thread::interruption_requested()) {
			renderThread->renderer->barrier->wait();

			//--------------------------------------------------------------------------
			// Render
			//--------------------------------------------------------------------------

			const float sampleScale = 1.f / samplePerPass;
			for (unsigned int i = 0; i < samplePerPass; ++i) {
				for (unsigned int y = 0; y < height; ++y) {
					for (unsigned int x = 0; x < width; ++x) {
						Spectrum s = SingleCPURenderer::SampleImage(gameLevel, rnd, *(renderThread->renderer->accel),
								width, height,
								x + rnd.floatValue() - .5f, y + rnd.floatValue() - .5f) *
								sampleScale;

						if (i == 0)
							passFrameBuffer->SetPixel(x, y, s);
						else
							passFrameBuffer->AddPixel(x, y, s);
					}
				}
			}

			//--------------------------------------------------------------------------
			// Apply a gaussian filter: approximated by applying a box filter multiple times
			//--------------------------------------------------------------------------

			switch (gameConfig.GetRendererFilterType()) {
				case NO_FILTER:
					break;
				case BLUR_LIGHT: {
					const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
					for (unsigned int i = 0; i < filterPassCount; ++i)
						FrameBuffer::ApplyBlurLightFilter(passFrameBuffer->GetPixels(), filterFrameBuffer->GetPixels(),
								width, height);
					break;
				}
				case BLUR_HEAVY: {
					const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
					for (unsigned int i = 0; i < filterPassCount; ++i)
						FrameBuffer::ApplyBlurHeavyFilter(passFrameBuffer->GetPixels(), filterFrameBuffer->GetPixels(),
								width, height);
					break;
				}
				case BOX: {
					const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
					for (unsigned int i = 0; i < filterPassCount; ++i)
						FrameBuffer::ApplyBoxFilter(passFrameBuffer->GetPixels(), filterFrameBuffer->GetPixels(),
								width, height, gameConfig.GetRendererFilterRaidus());
					break;
				}
			}

			renderThread->renderer->barrier->wait();
		}
	} catch (boost::thread_interrupted) {
		SFERA_LOG("[RenderThread::" << renderThread->index << "] Render thread halted");
	}
}
