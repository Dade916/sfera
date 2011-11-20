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

#include "sdl/editaction.h"
#include "renderer/ocl/oclrenderer.h"
#include "renderer/ocl/kernels/kernels.h"
#include "acceleretor/bvhaccel.h"
#include "utils/oclutils.h"

#if !defined(WIN32) && !defined(__APPLE__)
#include <GL/glx.h>
#endif

OCLRenderer::OCLRenderer(const GameLevel *level) : LevelRenderer(level),
		usedDeviceMemory(0),
		timeSinceLastCameraEdit(WallClockTime()),
		timeSinceLastNoCameraEdit(WallClockTime()) {
	const unsigned int width = gameLevel->gameConfig->GetScreenWidth();
	const unsigned int height = gameLevel->gameConfig->GetScreenHeight();

	compiledScene = new CompiledScene(level);

	//--------------------------------------------------------------------------
	// OpenCL setup
	//--------------------------------------------------------------------------

	bool deviceFound = false;
	cl::Device selectedDevice;

	// Scan all platforms and devices available
	VECTOR_CLASS<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	for (size_t i = 0; i < platforms.size(); ++i) {
		SFERA_LOG("OpenCL Platform " << i << ": " << platforms[i].getInfo<CL_PLATFORM_VENDOR>());

		// Get the list of devices available on the platform
		VECTOR_CLASS<cl::Device> devices;
		platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &devices);

		for (size_t j = 0; j < devices.size(); ++j) {
			SFERA_LOG("  OpenCL device " << j << ": " << devices[j].getInfo<CL_DEVICE_NAME>());
			SFERA_LOG("    Type: " << OCLDeviceTypeString(devices[j].getInfo<CL_DEVICE_TYPE>()));
			SFERA_LOG("    Units: " << devices[j].getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
			SFERA_LOG("    Global memory: " << devices[j].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / 1024 << "Kbytes");
			SFERA_LOG("    Local memory: " << devices[j].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 1024 << "Kbytes");
			SFERA_LOG("    Local memory type: " << OCLLocalMemoryTypeString(devices[j].getInfo<CL_DEVICE_LOCAL_MEM_TYPE>()));
			SFERA_LOG("    Constant memory: " << devices[j].getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>() / 1024 << "Kbytes");

			if (!deviceFound && (devices[j].getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU)) {
				selectedDevice = devices[j];
				deviceFound = true;
			}
		}
	}

	if (deviceFound) {
		SFERA_LOG("Selected OpenCL device: " << selectedDevice.getInfo<CL_DEVICE_NAME>());
		SFERA_LOG("  Type: " << OCLDeviceTypeString(selectedDevice.getInfo<CL_DEVICE_TYPE>()));
		SFERA_LOG("  Units: " << selectedDevice.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
	} else
		throw runtime_error("Unable to find a OpenCL GPU device");

	// Allocate a context with the selected device
	dev = selectedDevice;

	VECTOR_CLASS<cl::Device> devices;
	devices.push_back(dev);
	cl::Platform platform = dev.getInfo<CL_DEVICE_PLATFORM>();

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
	AllocOCLBufferRW(&frameBuffer, sizeof(Pixel) * width * height, "FrameBuffer");
	AllocOCLBufferRW(&toneMapFrameBuffer, sizeof(Pixel) * width * height, "ToneMap FrameBuffer");
	AllocOCLBufferRW(&gpuTaskBuffer, sizeof(ocl_kernels::GPUTask) * width * height, "GPUTask");
	AllocOCLBufferRW(&cameraBuffer, sizeof(ocl_kernels::GPUTask) * width * height, "Camera");
	AllocOCLBufferRO(&infiniteLightBuffer, (void *)(gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetPixels()),
			sizeof(Spectrum) * gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetWidth() *
			gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetHeight(), "Inifinite Light");

	AllocOCLBufferRO(&matBuffer, (void *)(&compiledScene->mats[0]),
			sizeof(compiledscene::Material) * compiledScene->mats.size(), "Materials");
	AllocOCLBufferRO(&matIndexBuffer, (void *)(&compiledScene->sphereMats[0]),
			sizeof(unsigned int) * compiledScene->sphereMats.size(), "Material Indices");

	if (compiledScene->texMaps.size() > 0) {
		AllocOCLBufferRO(&texMapBuffer, (void *)(&compiledScene->texMaps[0]),
				sizeof(compiledscene::TexMap) * compiledScene->texMaps.size(), "Texture Maps");

		AllocOCLBufferRO(&texMapRGBBuffer, (void *)(compiledScene->rgbTexMem),
				sizeof(Spectrum) * compiledScene->totRGBTexMem, "Texture Map Images");

		AllocOCLBufferRO(&texMapInstanceBuffer, (void *)(&compiledScene->sphereTexs[0]),
				sizeof(compiledscene::TexMapInstance) * compiledScene->sphereTexs.size(), "Texture Map Instances");

		if (compiledScene->sphereBumps.size() > 0)
			AllocOCLBufferRO(&bumpMapInstanceBuffer, (void *)(&compiledScene->sphereBumps[0]),
					sizeof(compiledscene::BumpMapInstance) * compiledScene->sphereBumps.size(), "Bump Map Instances");
	}

	//--------------------------------------------------------------------------
	// Create pixel buffer object for display
	//--------------------------------------------------------------------------

	glGenBuffersARB(1, &pbo);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
	glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, width * height *
			sizeof(GLubyte) * 4, 0, GL_STREAM_DRAW_ARB);
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	pboBuff = new cl::BufferGL(*ctx, CL_MEM_READ_WRITE, pbo);

	//--------------------------------------------------------------------------
	// Compile the kernel source
	//--------------------------------------------------------------------------

	// Set #define symbols
	stringstream ss;
	ss.precision(6);
	ss << scientific <<
			" -D PARAM_SCREEN_WIDTH=" << width <<
			" -D PARAM_SCREEN_HEIGHT=" << height <<
			" -D PARAM_SCREEN_SAMPLEPERPASS=" << gameLevel->gameConfig->GetRendererSamplePerPass() <<
			" -D PARAM_RAY_EPSILON=" << EPSILON << "f" <<
			" -D PARAM_MAX_DIFFUSE_BOUNCE=" << gameLevel->maxPathDiffuseBounces <<
			" -D PARAM_MAX_SPECULARGLOSSY_BOUNCE=" << gameLevel->maxPathSpecularGlossyBounces <<
			" -D PARAM_IL_SHIFT_U=" << gameLevel->scene->infiniteLight->GetShiftU() << "f" <<
			" -D PARAM_IL_SHIFT_V=" << gameLevel->scene->infiniteLight->GetShiftV() << "f" <<
			" -D PARAM_IL_GAIN_R=" << gameLevel->scene->infiniteLight->GetGain().r << "f" <<
			" -D PARAM_IL_GAIN_G=" << gameLevel->scene->infiniteLight->GetGain().g << "f" <<
			" -D PARAM_IL_GAIN_B=" << gameLevel->scene->infiniteLight->GetGain().b << "f" <<
			" -D PARAM_IL_MAP_WIDTH=" << gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetWidth() <<
			" -D PARAM_IL_MAP_HEIGHT=" << gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetHeight() <<
			" -D PARAM_GAMMA=" << gameLevel->toneMap->GetGamma() << "f";

	if (compiledScene->enable_MAT_MATTE)
		ss << " -D PARAM_ENABLE_MAT_MATTE";
	if (compiledScene->enable_MAT_MIRROR)
		ss << " -D PARAM_ENABLE_MAT_MIRROR";
	if (compiledScene->enable_MAT_GLASS)
		ss << " -D PARAM_ENABLE_MAT_GLASS";
	if (compiledScene->enable_MAT_METAL)
		ss << " -D PARAM_ENABLE_MAT_METAL";
	if (compiledScene->enable_MAT_ALLOY)
		ss << " -D PARAM_ENABLE_MAT_ALLOY";

	if (texMapBuffer) {
		ss << " -D PARAM_HAS_TEXTUREMAPS";
		
		if (compiledScene->sphereBumps.size() > 0)
			ss << " -D PARAM_HAS_BUMPMAPS";
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
	kernelInit->setArg(1, *passFrameBuffer);
	cmdQueue->enqueueNDRangeKernel(*kernelInit, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));

	kernelInitToneMapFB = new cl::Kernel(program, "InitFB");
	kernelInitToneMapFB->setArg(0, *passFrameBuffer);

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
		if (compiledScene->sphereBumps.size() > 0)
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

	kernelBlendFrame = new cl::Kernel(program, "BlendFrame");
	kernelBlendFrame->setArg(0, *passFrameBuffer);
	kernelBlendFrame->setArg(1, *frameBuffer);

	kernelUpdatePixelBuffer = new cl::Kernel(program, "UpdatePixelBuffer");
	kernelUpdatePixelBuffer->setArg(0, *frameBuffer);
	kernelUpdatePixelBuffer->setArg(1, *pboBuff);

}

OCLRenderer::~OCLRenderer() {
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

	delete pboBuff;
	glDeleteBuffersARB(1, &pbo);

	delete kernelBlendFrame;
	delete kernelApplyBoxFilterXR1;
	delete kernelApplyBoxFilterYR1;
	delete kernelApplyBlurHeavyFilterXR1;
	delete kernelApplyBlurHeavyFilterYR1;
	delete kernelApplyBlurLightFilterXR1;
	delete kernelApplyBlurLightFilterYR1;
	delete kernelPathTracing;
	delete kernelUpdatePixelBuffer;
	delete kernelInitToneMapFB;
	delete kernelInit;
	delete cmdQueue;
	delete ctx;
}

void OCLRenderer::AllocOCLBufferRO(cl::Buffer **buff, void *src, const size_t size, const string &desc) {
	if (*buff) {
		// Check the size of the already allocated buffer

		if (size == (*buff)->getInfo<CL_MEM_SIZE>()) {
			// I can reuse the buffer; just update the content
			cmdQueue->enqueueWriteBuffer(**buff, CL_FALSE, 0, size, src);
			return;
		}
	}

	SFERA_LOG("[OCLRenderer] " << desc << " buffer size: " << (size / 1024) << "Kbytes");
	*buff = new cl::Buffer(*ctx,
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			size, src);
	usedDeviceMemory += (*buff)->getInfo<CL_MEM_SIZE>();
}

void OCLRenderer::AllocOCLBufferRW(cl::Buffer **buff, const size_t size, const string &desc) {
	if (*buff) {
		// Check the size of the already allocated buffer

		if (size == (*buff)->getInfo<CL_MEM_SIZE>()) {
			// I can reuse the buffer
			return;
		}
	}

	SFERA_LOG("[OCLRenderer] " << desc << " buffer size: " << (size / 1024) << "Kbytes");
	*buff = new cl::Buffer(*ctx, CL_MEM_READ_WRITE, size);
	usedDeviceMemory += (*buff)->getInfo<CL_MEM_SIZE>();
}

void OCLRenderer::FreeOCLBuffer(cl::Buffer **buff) {
	if (*buff) {
		usedDeviceMemory -= (*buff)->getInfo<CL_MEM_SIZE>();
		delete *buff;
		*buff = NULL;
	}
}

size_t OCLRenderer::DrawFrame(const EditActionList &editActionList) {
	//--------------------------------------------------------------------------
	// Recompile the scene
	//--------------------------------------------------------------------------

	compiledScene->Recompile(editActionList);

	//--------------------------------------------------------------------------
	// Upload the new Camera to the GPU
	//--------------------------------------------------------------------------

	if (editActionList.Has(CAMERA_EDIT))
		cmdQueue->enqueueWriteBuffer(*cameraBuffer, CL_FALSE, 0, sizeof(compiledscene::Camera), &compiledScene->camera);

	//--------------------------------------------------------------------------
	// Check if I have to update the BVH buffer
	//--------------------------------------------------------------------------

	size_t bvhBufferSize = compiledScene->accel->nNodes * sizeof(BVHAccelArrayNode);
	if (!bvhBuffer) {
		AllocOCLBufferRW(&bvhBuffer, bvhBufferSize, "BVH");
		// Upload the new BVH to the GPU
		cmdQueue->enqueueWriteBuffer(*bvhBuffer, CL_FALSE, 0, bvhBufferSize, compiledScene->accel->bvhTree);

		kernelPathTracing->setArg(1, *bvhBuffer);
	} else if (bvhBuffer->getInfo<CL_MEM_SIZE>() < bvhBufferSize) {
		// Check if the buffer is of the right size
		FreeOCLBuffer(&bvhBuffer);
		AllocOCLBufferRW(&bvhBuffer, bvhBufferSize, "BVH");
		// Upload the new BVH to the GPU
		cmdQueue->enqueueWriteBuffer(*bvhBuffer, CL_FALSE, 0, bvhBufferSize, compiledScene->accel->bvhTree);

		kernelPathTracing->setArg(1, *bvhBuffer);
	} else if (editActionList.Has(GEOMETRY_EDIT)) {
		// Upload the new BVH to the GPU
		cmdQueue->enqueueWriteBuffer(*bvhBuffer, CL_FALSE, 0, bvhBufferSize, compiledScene->accel->bvhTree);
	}

	//--------------------------------------------------------------------------
	// Render
	//--------------------------------------------------------------------------

	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();
	const unsigned int samplePerPass = gameConfig.GetRendererSamplePerPass();

	cmdQueue->enqueueNDRangeKernel(*kernelInitToneMapFB, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));

	for (unsigned int i = 0; i < samplePerPass; ++i) {
		cmdQueue->enqueueNDRangeKernel(*kernelPathTracing, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));
	}

	//--------------------------------------------------------------------------
	// Apply a filter: approximated by applying a box filter multiple times
	//--------------------------------------------------------------------------

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
		k = dt / 5.0f;
	}

	const float blendFactor = (1.f - k) * gameConfig.GetRendererGhostFactorCameraEdit() +
		k * gameConfig.GetRendererGhostFactorNoCameraEdit();
	kernelBlendFrame->setArg(2, blendFactor);

	cmdQueue->enqueueNDRangeKernel(*kernelBlendFrame, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, WORKGROUP_SIZE)),
			cl::NDRange(WORKGROUP_SIZE));

	//--------------------------------------------------------------------------
	// Tone mapping
	//--------------------------------------------------------------------------

	// Copy the OpenCL frame buffer to OpenGL one
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

	return samplePerPass * width * height;
}

#endif
