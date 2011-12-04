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

#ifndef _SFERA_OCLRENDERER_H
#define	_SFERA_OCLRENDERER_H

#if !defined(SFERA_DISABLE_OPENCL)

#include "gamelevel.h"
#include "renderer/levelrenderer.h"
#include "renderer/ocl/compiledscene.h"
#include "pixel/framebuffer.h"

#define WORKGROUP_SIZE 64

namespace ocl_kernels {

typedef struct {
	unsigned int s1, s2, s3;
} Seed;

typedef struct {
	// The task seed
	Seed seed;
} GPUTask;

}

class OCLRendererThread;

class OCLRenderer : public LevelRenderer {
public:
	OCLRenderer(GameLevel *level);
	~OCLRenderer();

	size_t DrawFrame();

	friend class OCLRendererThread;

protected:
	vector<OCLRendererThread *> renderThread;
	boost::barrier *barrier;

	CompiledScene *compiledScene;
	float blendFactor;
	unsigned int totSamplePerPass;

	double timeSinceLastCameraEdit, timeSinceLastNoCameraEdit;
};

class OCLRendererThread {
public:
	OCLRendererThread(const size_t threadIndex, OCLRenderer *renderer,
			cl::Device device);
	~OCLRendererThread();

	void Start();
	void Stop();

	friend class OCLRenderer;

protected:
	void DrawFrame();

private:
	static void OCLRenderThreadStaticImpl(OCLRendererThread *renderThread);
	void OCLRenderThreadImpl();

	void PrintMemUsage(const size_t size, const string &desc) const;
	void AllocOCLBufferRO(cl::Buffer **buff, const size_t size, const string &desc);
	void AllocOCLBufferRO(cl::Buffer **buff, void *src, const size_t size, const string &desc);
	void AllocOCLBufferRW(cl::Buffer **buff, const size_t size, const string &desc);
	void FreeOCLBuffer(cl::Buffer **buff);

	void UpdateBVHBuffer();
	void UpdateMaterialsBuffer();

	size_t index;
	OCLRenderer *renderer;
	boost::thread *renderThread;

	cl::Device dev;
	cl::Context *ctx;
	cl::CommandQueue *cmdQueue;

	cl::Kernel *kernelInit;
	cl::Kernel *kernelInitFrameBuffer;
	cl::Kernel *kernelPathTracing;
	cl::Kernel *kernelApplyBlurLightFilterXR1;
	cl::Kernel *kernelApplyBlurLightFilterYR1;
	cl::Kernel *kernelApplyBlurHeavyFilterXR1;
	cl::Kernel *kernelApplyBlurHeavyFilterYR1;
	cl::Kernel *kernelApplyBoxFilterXR1;
	cl::Kernel *kernelApplyBoxFilterYR1;

	cl::Buffer *passFrameBuffer;
	cl::Buffer *tmpFrameBuffer;

	cl::Buffer *bvhBuffer;
	cl::Buffer *gpuTaskBuffer;
	cl::Buffer *cameraBuffer;
	cl::Buffer *infiniteLightBuffer;
	cl::Buffer *matBuffer;
	cl::Buffer *matIndexBuffer;
	cl::Buffer *texMapBuffer;
	cl::Buffer *texMapRGBBuffer;
	cl::Buffer *texMapInstanceBuffer;
	cl::Buffer *bumpMapInstanceBuffer;

	size_t usedDeviceMemory;

	//--------------------------------------------------------------------------
	// Used only in Multi-GPUs case
	//--------------------------------------------------------------------------

	FrameBuffer *cpuFrameBuffer;

	//--------------------------------------------------------------------------
	// Used only by thread with index 0
	//--------------------------------------------------------------------------

	cl::Kernel *kernelBlendFrame;
	cl::Kernel *kernelToneMapLinear;
	cl::Kernel *kernelUpdatePixelBuffer;

	cl::Buffer *frameBuffer;
	cl::Buffer *toneMapFrameBuffer;

	GLuint pbo;
	cl::BufferGL *pboBuff;
};

#endif

#endif	/* _SFERA_OCLRENDERER_H */
