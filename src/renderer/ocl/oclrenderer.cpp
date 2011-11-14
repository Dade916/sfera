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

	frameBuffer = new FrameBuffer(width, height);
	frameBuffer->Clear();

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

	if (!deviceFound)
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

	toneMapFrameBuffer = NULL;
	bvhBuffer = NULL;
	gpuTaskBuffer = NULL;
	cameraBuffer = NULL;

	AllocOCLBufferRW(&toneMapFrameBuffer, sizeof(Pixel) * width * height, "ToneMap FrameBuffer");
	AllocOCLBufferRW(&gpuTaskBuffer, sizeof(ocl_kernels::GPUTask) * width * height, "GPUTask");
	AllocOCLBufferRW(&cameraBuffer, sizeof(ocl_kernels::GPUTask) * width * height, "Camera");

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
			" -D PARAM_RAY_EPSILON=" << EPSILON << "f";

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
			cl::NDRange(RoundUp<unsigned int>(width * height, 64)),
			cl::NDRange(64));

	kernelInitToneMapFB = new cl::Kernel(program, "InitFB");
	kernelInitToneMapFB->setArg(0, *toneMapFrameBuffer);

	kernelUpdatePixelBuffer = new cl::Kernel(program, "UpdatePixelBuffer");
	kernelUpdatePixelBuffer->setArg(0, *toneMapFrameBuffer);
	kernelUpdatePixelBuffer->setArg(1, *pboBuff);

	kernelPathTracing = new cl::Kernel(program, "PathTracing");
	kernelPathTracing->setArg(0, *gpuTaskBuffer);
	kernelPathTracing->setArg(2, *cameraBuffer);
	kernelPathTracing->setArg(3, *toneMapFrameBuffer);
}

OCLRenderer::~OCLRenderer() {
	FreeOCLBuffer(&toneMapFrameBuffer);
	FreeOCLBuffer(&bvhBuffer);
	FreeOCLBuffer(&gpuTaskBuffer);
	FreeOCLBuffer(&cameraBuffer);

	delete pboBuff;
	glDeleteBuffersARB(1, &pbo);

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

	BVHAccel *accel = new BVHAccel(sphereList, treeType, isectCost, travCost, emptyBonus);

	size_t bvhBufferSize = accel->nNodes * sizeof(BVHAccelArrayNode);
	if (!bvhBuffer) {
		AllocOCLBufferRW(&bvhBuffer, bvhBufferSize, "BVH");

		kernelPathTracing->setArg(1, *bvhBuffer);
	} else {
		// Check if the buffer is of the right size
		if (bvhBuffer->getInfo<CL_MEM_SIZE>() < bvhBufferSize) {
			FreeOCLBuffer(&bvhBuffer);
			AllocOCLBufferRW(&bvhBuffer, bvhBufferSize, "BVH");

			kernelPathTracing->setArg(1, *bvhBuffer);
		}
	}

	// Upload the new BVH to the GPU
	cmdQueue->enqueueWriteBuffer(*bvhBuffer, CL_FALSE, 0, bvhBufferSize, accel->bvhTree);

	// Upload the new Camera to the GPU
	PerspectiveCamera &perpCamera(*(gameLevel->camera));
	camera.lensRadius = perpCamera.GetLensRadius();
	camera.focalDistance = perpCamera.GetFocalDistance();
	camera.yon = perpCamera.GetClipYon();
	camera.hither = perpCamera.GetClipHither();
	memcpy(camera.rasterToCameraMatrix, perpCamera.GetRasterToCameraMatrix().m, sizeof(float[4][4]));
	memcpy(camera.cameraToWorldMatrix, perpCamera.GetCameraToWorldMatrix().m, sizeof(float[4][4]));

	cmdQueue->enqueueWriteBuffer(*cameraBuffer, CL_FALSE, 0, sizeof(ocl_kernels::Camera), &camera);

	//--------------------------------------------------------------------------
	// Render
	//--------------------------------------------------------------------------

	const GameConfig &gameConfig(*(gameLevel->gameConfig));
	const unsigned int width = gameConfig.GetScreenWidth();
	const unsigned int height = gameConfig.GetScreenHeight();
	const unsigned int samplePerPass = gameConfig.GetRendererSamplePerPass();

	cmdQueue->enqueueNDRangeKernel(*kernelInitToneMapFB, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, 64)),
			cl::NDRange(64));

	for (unsigned int i = 0; i < samplePerPass; ++i) {
		cmdQueue->enqueueNDRangeKernel(*kernelPathTracing, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, 64)),
			cl::NDRange(64));
	}

	// Copy the OpenCL frame buffer to OpenGL one
	VECTOR_CLASS<cl::Memory> buffs;
	buffs.push_back(*pboBuff);
	cmdQueue->enqueueAcquireGLObjects(&buffs);

	cmdQueue->enqueueNDRangeKernel(*kernelUpdatePixelBuffer, cl::NullRange,
			cl::NDRange(RoundUp<unsigned int>(width * height, 64)),
			cl::NDRange(64));

	cmdQueue->enqueueReleaseGLObjects(&buffs);
	cmdQueue->finish();

	//--------------------------------------------------------------------------
	// Apply a filter: approximated by applying a box filter multiple times
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// Blend the new frame with the old one
	//--------------------------------------------------------------------------

	//--------------------------------------------------------------------------
	// Tone mapping
	//--------------------------------------------------------------------------

	// Draw the image on the screen
	glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

	delete accel;

	return width * height;
}

#endif
