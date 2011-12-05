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
#include "renderer/cpu/multicpurenderer.h"

//------------------------------------------------------------------------------
// MultiCPURenderer
//------------------------------------------------------------------------------

MultiCPURenderer::MultiCPURenderer(GameLevel *level) : CPURenderer(level) {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

	threadCount = boost::thread::hardware_concurrency();

	// Initialize all frame buffers
	for (size_t i = 0; i < threadCount; ++i) {
		threadPassFrameBuffer.push_back(new FrameBuffer(width, height / threadCount));
		threadPassFrameBuffer[i]->Clear();
	}

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

	for (size_t i = 0; i < threadCount; ++i)
		delete threadPassFrameBuffer[i];
}

size_t MultiCPURenderer::DrawFrame() {
	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();


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
	// Rendering
	//----------------------------------------------------------------------

	barrier->wait();
	// Other threads do the rendering
	barrier->wait();

	delete accel;

	//--------------------------------------------------------------------------
	// Merge all thread frames
	//--------------------------------------------------------------------------

	for (unsigned int y = 0; y < height; ++y) {
		const Pixel *p = threadPassFrameBuffer[y % threadCount]->GetPixel(0, y / threadCount);

		for (unsigned int x = 0; x < width; ++x)
			passFrameBuffer->SetPixel(x, y, *p++);
	}

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

	return gameConfig.GetRendererSamplePerPass() * width * height;
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
		FrameBuffer *threadPassFrameBuffer = renderThread->renderer->threadPassFrameBuffer[index];

		const GameConfig &gameConfig(*(gameLevel->gameConfig));
		const unsigned int width = gameConfig.GetScreenWidth();
		const unsigned int height = gameConfig.GetScreenHeight();
		const unsigned int samplePerPass = gameConfig.GetRendererSamplePerPass();
		const size_t threadCount = renderThread->renderer->threadCount;
		const float sampleScale = 1.f / samplePerPass;

		while (!boost::this_thread::interruption_requested()) {
			renderThread->renderer->barrier->wait();

			//------------------------------------------------------------------
			// Render
			//------------------------------------------------------------------

			for (unsigned int i = 0; i < samplePerPass; ++i) {
				for (unsigned int y = index; y < height; y += threadCount) {
					for (unsigned int x = 0; x < width; ++x) {
						Spectrum s = renderThread->renderer->SampleImage(
								rnd, *(renderThread->renderer->accel), renderThread->renderer->cameraCopy,
								x + rnd.floatValue() - .5f, y + rnd.floatValue() - .5f) *
								sampleScale;

						if (i == 0)
							threadPassFrameBuffer->SetPixel(x, y / threadCount, s);
						else
							threadPassFrameBuffer->AddPixel(x, y / threadCount, s);
					}
				}
			}

			renderThread->renderer->barrier->wait();
		}
	} catch (boost::thread_interrupted) {
		SFERA_LOG("[RenderThread::" << renderThread->index << "] Render thread halted");
	}
}
