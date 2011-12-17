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

#if !defined(SFERA_DISABLE_OPENCL)

#if defined(WIN32)
#include <windows.h>
#endif

#include "sfera.h"
#include "sdl/editaction.h"
#include "renderer/ocl/oclrenderer.h"
#include "renderer/ocl/kernels/kernels.h"
#include "acceleretor/bvhaccel.h"
#include "utils/oclutils.h"

#if !defined(WIN32) && !defined(__APPLE__)
#include <GL/glx.h>
#endif

//------------------------------------------------------------------------------
// OCLRenderer
//------------------------------------------------------------------------------

OCLRenderer::OCLRenderer(GameLevel *level) : LevelRenderer(level) {
	compiledScene = new CompiledScene(level);

	timeSinceLastCameraEdit = WallClockTime();
	timeSinceLastNoCameraEdit = timeSinceLastCameraEdit;

	//--------------------------------------------------------------------------
	// OpenCL setup
	//--------------------------------------------------------------------------

	vector<cl::Device> selectedDevices;

	// Scan all platforms and devices available
	VECTOR_CLASS<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	const string &selectString = gameLevel->gameConfig->GetOpenCLDeviceSelect();
	size_t selectIndex = 0;
	for (size_t i = 0; i < platforms.size(); ++i) {
		SFERA_LOG("[OCLRenderer] OpenCL Platform " << i << ": " << platforms[i].getInfo<CL_PLATFORM_VENDOR>());

		// Get the list of devices available on the platform
		VECTOR_CLASS<cl::Device> devices;
		platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);

		for (size_t j = 0; j < devices.size(); ++j) {
			SFERA_LOG("[OCLRenderer]   OpenCL device " << j << ": " << devices[j].getInfo<CL_DEVICE_NAME>());
			SFERA_LOG("[OCLRenderer]     Type: " << OCLDeviceTypeString(devices[j].getInfo<CL_DEVICE_TYPE>()));
			SFERA_LOG("[OCLRenderer]     Units: " << devices[j].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
			SFERA_LOG("[OCLRenderer]     Global memory: " << devices[j].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / 1024 << "Kbytes");
			SFERA_LOG("[OCLRenderer]     Local memory: " << devices[j].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 1024 << "Kbytes");
			SFERA_LOG("[OCLRenderer]     Local memory type: " << OCLLocalMemoryTypeString(devices[j].getInfo<CL_DEVICE_LOCAL_MEM_TYPE>()));
			SFERA_LOG("[OCLRenderer]     Constant memory: " << devices[j].getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>() / 1024 << "Kbytes");

			bool selected = false;
			if (!gameLevel->gameConfig->GetOpenCLUseOnlyGPUs() || (devices[j].getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU)) {
				if (selectString.length() == 0)
					selected = true;
				else {
					if (selectString.length() <= selectIndex)
						throw runtime_error("OpenCL select devices string (opencl.devices.select) has the wrong length");

					if (selectString.at(selectIndex) == '1')
						selected = true;
				}
			}

			if (selected) {
				selectedDevices.push_back(devices[j]);
				SFERA_LOG("[OCLRenderer]     SELECTED");
			} else
				SFERA_LOG("[OCLRenderer]     NOT SELECTED");

			++selectIndex;
		}
	}

	if (selectedDevices.size() == 0)
		throw runtime_error("Unable to find a OpenCL GPU device");

	// Create synchronization barrier
	barrier = new boost::barrier(selectedDevices.size() + 1);

	totSamplePerPass = 0;
	for (size_t i = 0; i < selectedDevices.size(); ++i)
		totSamplePerPass += gameLevel->gameConfig->GetOpenCLDeviceSamplePerPass(i);

	renderThread.resize(selectedDevices.size(), NULL);
	for (size_t i = 0; i < selectedDevices.size(); ++i) {
		OCLRendererThread *rt = new OCLRendererThread(i, this, selectedDevices[i]);
		renderThread[i] = rt;
	}

	for (size_t i = 0; i < renderThread.size(); ++i)
		renderThread[i]->Start();
}

OCLRenderer::~OCLRenderer() {
	for (size_t i = 0; i < renderThread.size(); ++i) {
		renderThread[i]->Stop();
		delete renderThread[i];
	}
}

size_t OCLRenderer::DrawFrame() {
	const GameConfig &gameConfig(*(gameLevel->gameConfig));

	//--------------------------------------------------------------------------
	// Recompile the scene
	//--------------------------------------------------------------------------

	{
		//const double t1 = WallClockTime();

		boost::unique_lock<boost::mutex> lock(gameLevel->levelMutex);
		compiledScene->Recompile(gameLevel->editActionList);
		gameLevel->editActionList.Reset();

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

		blendFactor = (1.f - k) * gameConfig.GetRendererGhostFactorCameraEdit() +
			k * gameConfig.GetRendererGhostFactorNoCameraEdit();

		//SFERA_LOG("Mutex time: " << ((WallClockTime() - t1) * 1000.0));
	}

	//--------------------------------------------------------------------------
	// Render
	//--------------------------------------------------------------------------

	barrier->wait();
	// Other threads do the rendering
	barrier->wait();

	//--------------------------------------------------------------------------
	// Blend frames, tone mapping and copy the OpenCL frame buffer to OpenGL one
	//--------------------------------------------------------------------------

	renderThread[0]->DrawFrame();

	return totSamplePerPass *
			gameConfig.GetScreenWidth() *
			gameConfig.GetScreenHeight();
}

//------------------------------------------------------------------------------
// OCLRendererThread
//------------------------------------------------------------------------------

OCLRendererThread::OCLRendererThread(const size_t threadIndex, OCLRenderer *renderer,
			cl::Device device) : index(threadIndex), renderer(renderer),
		dev(device), usedDeviceMemory(0) {
	const GameLevel &gameLevel(*(renderer->gameLevel));
	const unsigned int width = gameLevel.gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel.gameConfig->GetScreenHeight();

	const CompiledScene &compiledScene(*(renderer->compiledScene));

	if (renderer->renderThread.size() > 1)
		cpuFrameBuffer = new FrameBuffer(width, height);
	else
		cpuFrameBuffer = NULL;

	//--------------------------------------------------------------------------
	// OpenCL setup
	//--------------------------------------------------------------------------

	// Allocate a context with the selected device

	VECTOR_CLASS<cl::Device> devices;
	devices.push_back(dev);
	cl::Platform platform = dev.getInfo<CL_DEVICE_PLATFORM>();

	// The first thread uses OpenCL/OpenGL interoperability
	if (index == 0) {
#if defined (__APPLE__)
		CGLContextObj kCGLContext = CGLGetCurrentContext();
		CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
		cl_context_properties cps[] = {
			CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
			0
		};
#else
#ifdef WIN32
		cl_context_properties cps[] = {
			CL_GL_CONTEXT_KHR, (intptr_t)wglGetCurrentContext(),
			CL_WGL_HDC_KHR, (intptr_t)wglGetCurrentDC(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
			0
		};
#else
		cl_context_properties cps[] = {
			CL_GL_CONTEXT_KHR, (intptr_t)glXGetCurrentContext(),
			CL_GLX_DISPLAY_KHR, (intptr_t)glXGetCurrentDisplay(),
			CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
			0
		};
#endif
#endif

		ctx = new cl::Context(devices, cps);
	} else
		ctx = new cl::Context(devices);

	// Allocate the queue for this device
	cmdQueue = new cl::CommandQueue(*ctx, dev);

	//--------------------------------------------------------------------------
	// Allocate the buffers
	//--------------------------------------------------------------------------

	passFrameBuffer = NULL;
	tmpFrameBuffer = NULL;
	frameBuffer = NULL;
	toneMapFrameBuffer = NULL;
	bvhBuffer = NULL;
	gpuTaskBuffer = NULL;
	cameraBuffer = NULL;
	infiniteLightBuffer = NULL;
	matBuffer = NULL;
	matIndexBuffer = NULL;
	texMapBuffer = NULL;
	texMapRGBBuffer = NULL;
	texMapInstanceBuffer = NULL;
	bumpMapInstanceBuffer = NULL;

	AllocOCLBufferRW(&passFrameBuffer, sizeof(Pixel) * width * height, "Pass FrameBuffer");
	AllocOCLBufferRW(&tmpFrameBuffer, sizeof(Pixel) * width * height, "Temporary FrameBuffer");
	if (index == 0) {
		AllocOCLBufferRW(&frameBuffer, sizeof(Pixel) * width * height, "FrameBuffer");
		AllocOCLBufferRW(&toneMapFrameBuffer, sizeof(Pixel) * width * height, "ToneMap FrameBuffer");
	}
	AllocOCLBufferRW(&gpuTaskBuffer, sizeof(ocl_kernels::GPUTask) * width * height, "GPUTask");
	AllocOCLBufferRO(&cameraBuffer, sizeof(compiledscene::Camera), "Camera");
	AllocOCLBufferRO(&infiniteLightBuffer, (void *)(gameLevel.scene->infiniteLight->GetTexture()->GetTexMap()->GetPixels()),
			sizeof(Spectrum) * gameLevel.scene->infiniteLight->GetTexture()->GetTexMap()->GetWidth() *
			gameLevel.scene->infiniteLight->GetTexture()->GetTexMap()->GetHeight(), "Inifinite Light");

	AllocOCLBufferRO(&matBuffer, (void *)(&compiledScene.mats[0]),
			sizeof(compiledscene::Material) * compiledScene.mats.size(), "Materials");
	AllocOCLBufferRO(&matIndexBuffer, (void *)(&compiledScene.sphereMats[0]),
			sizeof(unsigned int) * compiledScene.sphereMats.size(), "Material Indices");

	if (compiledScene.texMaps.size() > 0) {
		AllocOCLBufferRO(&texMapBuffer, (void *)(&compiledScene.texMaps[0]),
				sizeof(compiledscene::TexMap) * compiledScene.texMaps.size(), "Texture Maps");

		AllocOCLBufferRO(&texMapRGBBuffer, (void *)(compiledScene.rgbTexMem),
				sizeof(Spectrum) * compiledScene.totRGBTexMem, "Texture Map Images");

		AllocOCLBufferRO(&texMapInstanceBuffer, (void *)(&compiledScene.sphereTexs[0]),
				sizeof(compiledscene::TexMapInstance) * compiledScene.sphereTexs.size(), "Texture Map Instances");

		if (compiledScene.sphereBumps.size() > 0)
			AllocOCLBufferRO(&bumpMapInstanceBuffer, (void *)(&compiledScene.sphereBumps[0]),
					sizeof(compiledscene::BumpMapInstance) * compiledScene.sphereBumps.size(), "Bump Map Instances");
	}

	SFERA_LOG("[OCLRenderer] Total OpenCL device memory used: " << fixed << setprecision(2) << usedDeviceMemory / (1024 * 1024) << "Mbytes");

	if (index == 0) {
		//--------------------------------------------------------------------------
		// Create pixel buffer object for display
		//--------------------------------------------------------------------------

		glGenBuffersARB(1, &pbo);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
		glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, width * height *
				sizeof(GLubyte) * 4, 0, GL_STREAM_DRAW_ARB);
		glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
		pboBuff = new cl::BufferGL(*ctx, CL_MEM_READ_WRITE, pbo);
	}

	//--------------------------------------------------------------------------
	// Compile the kernel source
	//--------------------------------------------------------------------------

	// Set #define symbols
	stringstream ss;
	ss.precision(6);
	ss << scientific <<
			" -D PARAM_SCREEN_WIDTH=" << width <<
			" -D PARAM_SCREEN_HEIGHT=" << height <<
			" -D PARAM_SCREEN_SAMPLEPERPASS=" << renderer->totSamplePerPass <<
			" -D PARAM_RAY_EPSILON=" << EPSILON << "f" <<
			" -D PARAM_MAX_DIFFUSE_BOUNCE=" << gameLevel.maxPathDiffuseBounces <<
			" -D PARAM_MAX_SPECULARGLOSSY_BOUNCE=" << gameLevel.maxPathSpecularGlossyBounces <<
			" -D PARAM_IL_SHIFT_U=" << gameLevel.scene->infiniteLight->GetShiftU() << "f" <<
			" -D PARAM_IL_SHIFT_V=" << gameLevel.scene->infiniteLight->GetShiftV() << "f" <<
			" -D PARAM_IL_GAIN_R=" << gameLevel.scene->infiniteLight->GetGain().r << "f" <<
			" -D PARAM_IL_GAIN_G=" << gameLevel.scene->infiniteLight->GetGain().g << "f" <<
			" -D PARAM_IL_GAIN_B=" << gameLevel.scene->infiniteLight->GetGain().b << "f" <<
			" -D PARAM_IL_MAP_WIDTH=" << gameLevel.scene->infiniteLight->GetTexture()->GetTexMap()->GetWidth() <<
			" -D PARAM_IL_MAP_HEIGHT=" << gameLevel.scene->infiniteLight->GetTexture()->GetTexMap()->GetHeight() <<
			" -D PARAM_GAMMA=" << gameLevel.toneMap->GetGamma() << "f" <<
			" -D PARAM_MEM_TYPE=" << gameLevel.gameConfig->GetOpenCLMemType();

	if (compiledScene.enable_MAT_MATTE)
		ss << " -D PARAM_ENABLE_MAT_MATTE";
	if (compiledScene.enable_MAT_MIRROR)
		ss << " -D PARAM_ENABLE_MAT_MIRROR";
	if (compiledScene.enable_MAT_GLASS)
		ss << " -D PARAM_ENABLE_MAT_GLASS";
	if (compiledScene.enable_MAT_METAL)
		ss << " -D PARAM_ENABLE_MAT_METAL";
	if (compiledScene.enable_MAT_ALLOY)
		ss << " -D PARAM_ENABLE_MAT_ALLOY";

	if (texMapBuffer) {
		ss << " -D PARAM_HAS_TEXTUREMAPS";

		if (compiledScene.sphereBumps.size() > 0)
			ss << " -D PARAM_HAS_BUMPMAPS";
	}

	switch (gameLevel.toneMap->GetType()) {
		case TONEMAP_REINHARD02:
			ss << " -D PARAM_TM_LINEAR_SCALE=1.0f";
			break;
		case TONEMAP_LINEAR: {
			LinearToneMap *tm = (LinearToneMap *)gameLevel.toneMap;
			ss << " -D PARAM_TM_LINEAR_SCALE=" << tm->scale << "f";
			break;
		}
		default:
			assert (false);

	}

#if defined(__APPLE__)
	ss << " -D __APPLE__";
#endif

	SFERA_LOG("[OCLRenderer] Defined symbols: " << ss.str());
	SFERA_LOG("[OCLRenderer] Compiling kernels");

	cl::Program::Sources source(1, std::make_pair(KernelSource_kernel_core.c_str(), KernelSource_kernel_core.length()));
	cl::Program program = cl::Program(*ctx, source);
	try {
		VECTOR_CLASS<cl::Device> buildDevice;
		buildDevice.push_back(dev);
		program.build(buildDevice, ss.str().c_str());
	} catch (cl::Error err) {
		cl::STRING_CLASS strError = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(dev);
		SFERA_LOG("[OCLRenderer] Kernel compilation error:\n" << strError.c_str());

		throw err;
	}

	kernelInit = new cl::Kernel(program, "Init");
	kernelInit->setArg(0, *gpuTaskBuffer);
	cmdQueue->enqueueNDRangeKernel(*kernelInit, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));

	kernelInitFrameBuffer = new cl::Kernel(program, "InitFB");
	if (index == 0) {
		kernelInitFrameBuffer->setArg(0, *frameBuffer);
		cmdQueue->enqueueNDRangeKernel(*kernelInitFrameBuffer, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));
	}
	kernelInitFrameBuffer->setArg(0, *passFrameBuffer);

	kernelPathTracing = new cl::Kernel(program, "PathTracing");
	unsigned int argIndex = 0;
	kernelPathTracing->setArg(argIndex++, *gpuTaskBuffer);
	argIndex++;
	kernelPathTracing->setArg(argIndex++, *cameraBuffer);
	kernelPathTracing->setArg(argIndex++, *infiniteLightBuffer);
	kernelPathTracing->setArg(argIndex++, *passFrameBuffer);
	kernelPathTracing->setArg(argIndex++, *matBuffer);
	kernelPathTracing->setArg(argIndex++, *matIndexBuffer);
	if (texMapBuffer) {
		kernelPathTracing->setArg(argIndex++, *texMapBuffer);
		kernelPathTracing->setArg(argIndex++, *texMapRGBBuffer);
		kernelPathTracing->setArg(argIndex++, *texMapInstanceBuffer);
		if (compiledScene.sphereBumps.size() > 0)
			kernelPathTracing->setArg(argIndex++, *bumpMapInstanceBuffer);
	}

	kernelApplyBlurLightFilterXR1 = new cl::Kernel(program, "ApplyBlurLightFilterXR1");
	kernelApplyBlurLightFilterXR1->setArg(0, *passFrameBuffer);
	kernelApplyBlurLightFilterXR1->setArg(1, *tmpFrameBuffer);

	kernelApplyBlurLightFilterYR1 = new cl::Kernel(program, "ApplyBlurLightFilterYR1");
	kernelApplyBlurLightFilterYR1->setArg(0, *tmpFrameBuffer);
	kernelApplyBlurLightFilterYR1->setArg(1, *passFrameBuffer);

	kernelApplyBlurHeavyFilterXR1 = new cl::Kernel(program, "ApplyBlurHeavyFilterXR1");
	kernelApplyBlurHeavyFilterXR1->setArg(0, *passFrameBuffer);
	kernelApplyBlurHeavyFilterXR1->setArg(1, *tmpFrameBuffer);

	kernelApplyBlurHeavyFilterYR1 = new cl::Kernel(program, "ApplyBlurHeavyFilterYR1");
	kernelApplyBlurHeavyFilterYR1->setArg(0, *tmpFrameBuffer);
	kernelApplyBlurHeavyFilterYR1->setArg(1, *passFrameBuffer);

	kernelApplyBoxFilterXR1 = new cl::Kernel(program, "ApplyBoxFilterXR1");
	kernelApplyBoxFilterXR1->setArg(0, *passFrameBuffer);
	kernelApplyBoxFilterXR1->setArg(1, *tmpFrameBuffer);

	kernelApplyBoxFilterYR1 = new cl::Kernel(program, "ApplyBoxFilterYR1");
	kernelApplyBoxFilterYR1->setArg(0, *tmpFrameBuffer);
	kernelApplyBoxFilterYR1->setArg(1, *passFrameBuffer);

	if (index == 0) {
		kernelBlendFrame = new cl::Kernel(program, "BlendFrame");
		kernelBlendFrame->setArg(0, *passFrameBuffer);
		kernelBlendFrame->setArg(1, *frameBuffer);

		kernelToneMapLinear = new cl::Kernel(program, "ToneMapLinear");
		kernelToneMapLinear->setArg(0, *frameBuffer);
		kernelToneMapLinear->setArg(1, *toneMapFrameBuffer);

		kernelUpdatePixelBuffer = new cl::Kernel(program, "UpdatePixelBuffer");
		kernelUpdatePixelBuffer->setArg(0, *toneMapFrameBuffer);
		kernelUpdatePixelBuffer->setArg(1, *pboBuff);
	} else {
		kernelBlendFrame = NULL;
		kernelToneMapLinear = NULL;
		kernelUpdatePixelBuffer = NULL;
	}
}

OCLRendererThread::~OCLRendererThread() {
	FreeOCLBuffer(&passFrameBuffer);
	FreeOCLBuffer(&tmpFrameBuffer);
	FreeOCLBuffer(&frameBuffer);
	FreeOCLBuffer(&toneMapFrameBuffer);
	FreeOCLBuffer(&bvhBuffer);
	FreeOCLBuffer(&gpuTaskBuffer);
	FreeOCLBuffer(&cameraBuffer);
	FreeOCLBuffer(&infiniteLightBuffer);
	FreeOCLBuffer(&matBuffer);
	FreeOCLBuffer(&matIndexBuffer);
	FreeOCLBuffer(&texMapBuffer);
	FreeOCLBuffer(&texMapRGBBuffer);
	FreeOCLBuffer(&texMapInstanceBuffer);
	FreeOCLBuffer(&bumpMapInstanceBuffer);

	if (index == 0) {
		delete pboBuff;
		glDeleteBuffersARB(1, &pbo);
	}

	delete kernelUpdatePixelBuffer;
	delete kernelToneMapLinear;
	delete kernelBlendFrame;
	delete kernelApplyBoxFilterXR1;
	delete kernelApplyBoxFilterYR1;
	delete kernelApplyBlurHeavyFilterXR1;
	delete kernelApplyBlurHeavyFilterYR1;
	delete kernelApplyBlurLightFilterXR1;
	delete kernelApplyBlurLightFilterYR1;
	delete kernelPathTracing;
	delete kernelInitFrameBuffer;
	delete kernelInit;
	delete cmdQueue;
	delete ctx;

	delete cpuFrameBuffer;
}

void OCLRendererThread::PrintMemUsage(const size_t size, const string &desc) const {
	if (size / 1024 < 10) {
		SFERA_LOG("[OCLRenderer::" << index << "] " << desc << " buffer size: " << size << "bytes");
	} else if (size / (1024 * 1024) < 10) {
		SFERA_LOG("[OCLRenderer::" << index << "] " << desc << " buffer size: " << (size / 1024) << "Kbytes");
	} else {
		SFERA_LOG("[OCLRenderer::" << index << "] " << desc << " buffer size: " << (size / (1024 * 1024)) << "Mbytes");
	}
}

void OCLRendererThread::AllocOCLBufferRO(cl::Buffer **buff, const size_t size, const string &desc) {
	if (*buff) {
		// Check the size of the already allocated buffer

		if (size == (*buff)->getInfo<CL_MEM_SIZE>()) {
			// I can reuse the buffer
			return;
		} else
			FreeOCLBuffer(buff);
	}

	PrintMemUsage(size, desc);

	*buff = new cl::Buffer(*ctx,
			CL_MEM_READ_ONLY,
			size);
	usedDeviceMemory += (*buff)->getInfo<CL_MEM_SIZE>();
}

void OCLRendererThread::AllocOCLBufferRO(cl::Buffer **buff, void *src, const size_t size, const string &desc) {
	if (*buff) {
		// Check the size of the already allocated buffer

		if (size == (*buff)->getInfo<CL_MEM_SIZE>()) {
			// I can reuse the buffer; just update the content
			cmdQueue->enqueueWriteBuffer(**buff, CL_FALSE, 0, size, src);
			return;
		} else
			FreeOCLBuffer(buff);
	}

	PrintMemUsage(size, desc);

	*buff = new cl::Buffer(*ctx,
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			size, src);
	usedDeviceMemory += (*buff)->getInfo<CL_MEM_SIZE>();
}

void OCLRendererThread::AllocOCLBufferRW(cl::Buffer **buff, const size_t size, const string &desc) {
	if (*buff) {
		// Check the size of the already allocated buffer

		if (size == (*buff)->getInfo<CL_MEM_SIZE>()) {
			// I can reuse the buffer
			return;
		} else
			FreeOCLBuffer(buff);
	}

	PrintMemUsage(size, desc);

	*buff = new cl::Buffer(*ctx, CL_MEM_READ_WRITE, size);
	usedDeviceMemory += (*buff)->getInfo<CL_MEM_SIZE>();
}

void OCLRendererThread::FreeOCLBuffer(cl::Buffer **buff) {
	if (*buff) {
		usedDeviceMemory -= (*buff)->getInfo<CL_MEM_SIZE>();
		delete *buff;
		*buff = NULL;
	}
}

void OCLRendererThread::Start() {
	renderThread = new boost::thread(boost::bind(OCLRendererThread::OCLRenderThreadStaticImpl, this));
}

void OCLRendererThread::Stop() {
	if (renderThread) {
		renderThread->interrupt();
		renderThread->join();
		delete renderThread;
		renderThread = NULL;
	}
}

void OCLRendererThread::OCLRenderThreadStaticImpl(OCLRendererThread *renderThread) {
	renderThread->OCLRenderThreadImpl();
}

void OCLRendererThread::UpdateBVHBuffer() {
	const CompiledScene &compiledScene(*(renderer->compiledScene));
	size_t bvhBufferSize = compiledScene.accel->nNodes * sizeof(BVHAccelArrayNode);

	if (!bvhBuffer || (bvhBuffer->getInfo<CL_MEM_SIZE>() < bvhBufferSize)) {
		AllocOCLBufferRO(&bvhBuffer, compiledScene.accel->bvhTree, bvhBufferSize, "BVH");
		kernelPathTracing->setArg(1, *bvhBuffer);
	} else if (compiledScene.editActionsUsed.Has(GEOMETRY_EDIT)) {
		// Upload the new BVH to the GPU
		cmdQueue->enqueueWriteBuffer(*bvhBuffer, CL_FALSE, 0, bvhBufferSize, compiledScene.accel->bvhTree);
	}
}

void OCLRendererThread::UpdateMaterialsBuffer() {
	const CompiledScene &compiledScene(*(renderer->compiledScene));
	if (compiledScene.editActionsUsed.Has(MATERIALS_EDIT)) {
		AllocOCLBufferRO(&matBuffer, (void *)(&compiledScene.mats[0]),
				sizeof(compiledscene::Material) * compiledScene.mats.size(), "Materials");
		AllocOCLBufferRO(&matIndexBuffer, (void *)(&compiledScene.sphereMats[0]),
				sizeof(unsigned int) * compiledScene.sphereMats.size(), "Material Indices");

		kernelPathTracing->setArg(5, *matBuffer);
		kernelPathTracing->setArg(6, *matIndexBuffer);
	}
}

void OCLRendererThread::OCLRenderThreadImpl() {
	try {
		const GameConfig &gameConfig(*(renderer->gameLevel->gameConfig));
		const unsigned int width = gameConfig.GetScreenWidth();
		const unsigned int height = gameConfig.GetScreenHeight();
		const unsigned int samplePerPass = gameConfig.GetOpenCLDeviceSamplePerPass(index);
		const CompiledScene &compiledScene(*(renderer->compiledScene));
		boost::barrier *barrier = renderer->barrier;

		while (!boost::this_thread::interruption_requested()) {
			barrier->wait();

			//------------------------------------------------------------------
			// Upload the new Camera to the GPU
			//------------------------------------------------------------------

			cmdQueue->enqueueWriteBuffer(*cameraBuffer,
					CL_FALSE, 0, sizeof(compiledscene::Camera), &compiledScene.camera);

			//------------------------------------------------------------------
			// Check if I have to update the BVH buffer
			//------------------------------------------------------------------

			UpdateBVHBuffer();

			//------------------------------------------------------------------
			// Check if I have to update the material related buffers
			//------------------------------------------------------------------

			UpdateMaterialsBuffer();

			//------------------------------------------------------------------
			// Render
			//------------------------------------------------------------------

			cmdQueue->enqueueNDRangeKernel(*kernelInitFrameBuffer, cl::NullRange,
					cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
					cl::NDRange(WORKGROUP_SIZE));

			for (unsigned int i = 0; i < samplePerPass; ++i) {
				cmdQueue->enqueueNDRangeKernel(*kernelPathTracing, cl::NullRange,
					cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
					cl::NDRange(WORKGROUP_SIZE));
			}

			//------------------------------------------------------------------
			// Apply a filter: approximated by applying a box filter
			// multiple times
			//------------------------------------------------------------------

			switch (gameConfig.GetRendererFilterType()) {
				case NO_FILTER:
					break;
				case BLUR_LIGHT: {
					const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
					for (unsigned int i = 0; i < filterPassCount; ++i) {
						cmdQueue->enqueueNDRangeKernel(*kernelApplyBlurLightFilterXR1, cl::NullRange,
								cl::NDRange(RoundUp<unsigned int>(height, WORKGROUP_SIZE)),
								cl::NDRange(WORKGROUP_SIZE));

						cmdQueue->enqueueNDRangeKernel(*kernelApplyBlurLightFilterYR1, cl::NullRange,
								cl::NDRange(RoundUp<unsigned int>(width, WORKGROUP_SIZE)),
								cl::NDRange(WORKGROUP_SIZE));
					}
					break;
				}
				case BLUR_HEAVY: {
					const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
					for (unsigned int i = 0; i < filterPassCount; ++i) {
						cmdQueue->enqueueNDRangeKernel(*kernelApplyBlurHeavyFilterXR1, cl::NullRange,
								cl::NDRange(RoundUp<unsigned int>(height, WORKGROUP_SIZE)),
								cl::NDRange(WORKGROUP_SIZE));

						cmdQueue->enqueueNDRangeKernel(*kernelApplyBlurHeavyFilterYR1, cl::NullRange,
								cl::NDRange(RoundUp<unsigned int>(width, WORKGROUP_SIZE)),
								cl::NDRange(WORKGROUP_SIZE));
					}
					break;
				}
				case BOX: {
					const unsigned int filterPassCount = gameConfig.GetRendererFilterIterations();
					for (unsigned int i = 0; i < filterPassCount; ++i) {
						cmdQueue->enqueueNDRangeKernel(*kernelApplyBoxFilterXR1, cl::NullRange,
								cl::NDRange(RoundUp<unsigned int>(height, WORKGROUP_SIZE)),
								cl::NDRange(WORKGROUP_SIZE));

						cmdQueue->enqueueNDRangeKernel(*kernelApplyBoxFilterYR1, cl::NullRange,
								cl::NDRange(RoundUp<unsigned int>(width, WORKGROUP_SIZE)),
								cl::NDRange(WORKGROUP_SIZE));
					}
					break;
				}
				default:
					assert (false);
			}

			//------------------------------------------------------------------
			// Read back the framebuffer if required
			//------------------------------------------------------------------

			if (renderer->renderThread.size() > 1) {
				// Multi-GPU case: read back the framebuffer, the merge is
				// done on the CPU

				cmdQueue->enqueueReadBuffer(*passFrameBuffer,
					CL_FALSE, 0, sizeof(Pixel) * width * height, cpuFrameBuffer->GetPixels());
			}

			//------------------------------------------------------------------

			cmdQueue->finish();
			barrier->wait();
		}
	} catch (boost::thread_interrupted) {
		SFERA_LOG("[OCLRendererThread::" << index << "] Render thread halted");
	}
}

void OCLRendererThread::DrawFrame() {
	const GameLevel &gameLevel(*(renderer->gameLevel));
	const GameConfig &gameConfig(*(gameLevel.gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();

	//--------------------------------------------------------------------------
	// Merge all the framebuffers if required
	//--------------------------------------------------------------------------

	const size_t threadCount = renderer->renderThread.size();
	if (threadCount > 1) {
		vector<Pixel *> cpuFrameBuffers(threadCount);
		for (size_t i = 0; i < threadCount; ++i)
				cpuFrameBuffers[i] = renderer->renderThread[i]->cpuFrameBuffer->GetPixels();

		Pixel *dst = cpuFrameBuffers[0];
		for (size_t i = 0; i < width * height; ++i) {
			float r = dst->r;
			float g = dst->g;
			float b = dst->b;

			for (size_t j = 1; j < threadCount; ++j) {
				r += cpuFrameBuffers[j]->r;
				g += cpuFrameBuffers[j]->g;
				b += cpuFrameBuffers[j]->b;

				cpuFrameBuffers[j] += 1;
			}

			dst->r = r;
			dst->g = g;
			dst->b = b;

			++dst;
		}

		cmdQueue->enqueueWriteBuffer(*passFrameBuffer,
					CL_FALSE, 0, sizeof(Pixel) * width * height, cpuFrameBuffers[0]);
	}

	//--------------------------------------------------------------------------
	// Blend the new frame with the old one
	//--------------------------------------------------------------------------

	kernelBlendFrame->setArg(2, renderer->blendFactor);

	cmdQueue->enqueueNDRangeKernel(*kernelBlendFrame, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));

	//--------------------------------------------------------------------------
	// Tone mapping
	//--------------------------------------------------------------------------

	switch (gameLevel.toneMap->GetType()) {
		case TONEMAP_REINHARD02:
		case TONEMAP_LINEAR:
			cmdQueue->enqueueNDRangeKernel(*kernelToneMapLinear, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));
			break;
		default:
			assert (false);

	}

	//--------------------------------------------------------------------------
	// Copy the OpenCL frame buffer to OpenGL one
	//--------------------------------------------------------------------------

	VECTOR_CLASS<cl::Memory> buffs;
	buffs.push_back(*pboBuff);
	cmdQueue->enqueueAcquireGLObjects(&buffs);

	cmdQueue->enqueueNDRangeKernel(*kernelUpdatePixelBuffer, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));

	cmdQueue->enqueueReleaseGLObjects(&buffs);
	cmdQueue->finish();

	// Draw the image on the screen
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
}

#endif
