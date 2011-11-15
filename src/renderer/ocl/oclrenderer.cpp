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
	infiniteLightBuffer = NULL;
	matBuffer = NULL;
	matIndexBuffer = NULL;

	AllocOCLBufferRW(&toneMapFrameBuffer, sizeof(Pixel) * width * height, "ToneMap FrameBuffer");
	AllocOCLBufferRW(&gpuTaskBuffer, sizeof(ocl_kernels::GPUTask) * width * height, "GPUTask");
	AllocOCLBufferRW(&cameraBuffer, sizeof(ocl_kernels::GPUTask) * width * height, "Camera");
	AllocOCLBufferRO(&infiniteLightBuffer, (void *)(gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetPixels()),
			sizeof(Spectrum) * gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetWidth() *
			gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetHeight(), "Inifinite Light");

	CompileMaterials();
	AllocOCLBufferRO(&matBuffer, (void *)(&mats[0]),
			sizeof(ocl_kernels::Material) * mats.size(), "Materials");
	AllocOCLBufferRO(&matIndexBuffer, (void *)(&sphereMats[0]),
			sizeof(unsigned int) * sphereMats.size(), "Material Indices");

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
			" -D PARAM_IL_MAP_HEIGHT=" << gameLevel->scene->infiniteLight->GetTexture()->GetTexMap()->GetHeight();

	if (enable_MAT_MATTE)
		ss << " -D PARAM_ENABLE_MAT_MATTE";
	if (enable_MAT_MIRROR)
		ss << " -D PARAM_ENABLE_MAT_MIRROR";
	if (enable_MAT_GLASS)
		ss << " -D PARAM_ENABLE_MAT_GLASS";
	if (enable_MAT_METAL)
		ss << " -D PARAM_ENABLE_MAT_METAL";
	if (enable_MAT_ALLOY)
		ss << " -D PARAM_ENABLE_MAT_ALLOY";

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
	kernelPathTracing->setArg(3, *infiniteLightBuffer);
	kernelPathTracing->setArg(4, *toneMapFrameBuffer);
	kernelPathTracing->setArg(5, *matBuffer);
	kernelPathTracing->setArg(6, *matIndexBuffer);
}

OCLRenderer::~OCLRenderer() {
	FreeOCLBuffer(&toneMapFrameBuffer);
	FreeOCLBuffer(&bvhBuffer);
	FreeOCLBuffer(&gpuTaskBuffer);
	FreeOCLBuffer(&cameraBuffer);
	FreeOCLBuffer(&infiniteLightBuffer);
	FreeOCLBuffer(&matBuffer);
	FreeOCLBuffer(&matIndexBuffer);

	delete pboBuff;
	glDeleteBuffersARB(1, &pbo);

	delete kernelPathTracing;
	delete kernelUpdatePixelBuffer;
	delete kernelInitToneMapFB;
	delete kernelInit;
	delete cmdQueue;
	delete ctx;
}

void OCLRenderer::CompileMaterial(Material *m, ocl_kernels::Material *gpum) {
	gpum->emi_r = m->GetEmission().r;
	gpum->emi_g = m->GetEmission().g;
	gpum->emi_b = m->GetEmission().b;

	switch (m->GetType()) {
		case MATTE: {
			enable_MAT_MATTE = true;
			MatteMaterial *mm = (MatteMaterial *)m;

			gpum->type = MAT_MATTE;
			gpum->param.matte.r = mm->GetKd().r;
			gpum->param.matte.g = mm->GetKd().g;
			gpum->param.matte.b = mm->GetKd().b;
			break;
		}
		case MIRROR: {
			enable_MAT_MIRROR = true;
			MirrorMaterial *mm = (MirrorMaterial *)m;

			gpum->type = MAT_MIRROR;
			gpum->param.mirror.r = mm->GetKr().r;
			gpum->param.mirror.g = mm->GetKr().g;
			gpum->param.mirror.b = mm->GetKr().b;
			break;
		}
		case GLASS: {
			enable_MAT_GLASS = true;
			GlassMaterial *gm = (GlassMaterial *)m;

			gpum->type = MAT_GLASS;
			gpum->param.glass.refl_r = gm->GetKrefl().r;
			gpum->param.glass.refl_g = gm->GetKrefl().g;
			gpum->param.glass.refl_b = gm->GetKrefl().b;

			gpum->param.glass.refrct_r = gm->GetKrefrct().r;
			gpum->param.glass.refrct_g = gm->GetKrefrct().g;
			gpum->param.glass.refrct_b = gm->GetKrefrct().b;

			gpum->param.glass.ousideIor = gm->GetOutsideIOR();
			gpum->param.glass.ior = gm->GetIOR();
			gpum->param.glass.R0 = gm->GetR0();
			break;
		}
		case METAL: {
			enable_MAT_METAL = true;
			MetalMaterial *mm = (MetalMaterial *)m;

			gpum->type = MAT_METAL;
			gpum->param.metal.r = mm->GetKr().r;
			gpum->param.metal.g = mm->GetKr().g;
			gpum->param.metal.b = mm->GetKr().b;
			gpum->param.metal.exponent = mm->GetExp();
			break;
		}
		case ALLOY: {
			enable_MAT_ALLOY = true;
			AlloyMaterial *am = (AlloyMaterial *)m;

			gpum->type = MAT_ALLOY;
			gpum->param.alloy.refl_r= am->GetKrefl().r;
			gpum->param.alloy.refl_g = am->GetKrefl().g;
			gpum->param.alloy.refl_b = am->GetKrefl().b;

			gpum->param.alloy.diff_r = am->GetKd().r;
			gpum->param.alloy.diff_g = am->GetKd().g;
			gpum->param.alloy.diff_b = am->GetKd().b;

			gpum->param.alloy.exponent = am->GetExp();
			gpum->param.alloy.R0 = am->GetR0();
			break;
		}
		default: {
			enable_MAT_MATTE = true;
			gpum->type = MAT_MATTE;
			gpum->param.matte.r = 0.75f;
			gpum->param.matte.g = 0.75f;
			gpum->param.matte.b = 0.75f;
			break;
		}
	}
}

void OCLRenderer::CompileMaterials() {
	SFERA_LOG("[OCLRenderer] Compile Materials");

	Scene &scene(*(gameLevel->scene));

	//--------------------------------------------------------------------------
	// Translate material definitions
	//--------------------------------------------------------------------------

	const double tStart = WallClockTime();

	enable_MAT_MATTE = false;
	enable_MAT_MIRROR = false;
	enable_MAT_GLASS = false;
	enable_MAT_METAL = false;
	enable_MAT_ALLOY = false;

	const unsigned int materialsCount = scene.materials.size() + GAMEPLAYER_PUPPET_SIZE;
	mats.resize(materialsCount);

	// Add scene materials
	for (unsigned int i = 0; i < scene.materials.size(); ++i) {
		Material *m = scene.materials[i];
		ocl_kernels::Material *gpum = &mats[i];

		CompileMaterial(m, gpum);
	}

	// Add player materials
	for (unsigned int i = 0; i < GAMEPLAYER_PUPPET_SIZE; ++i) {
		Material *m = gameLevel->player->puppetMaterial[i];
		ocl_kernels::Material *gpum = &mats[i + scene.materials.size()];

		CompileMaterial(m, gpum);
	}

	//--------------------------------------------------------------------------
	// Translate sphere material indices
	//--------------------------------------------------------------------------

	const unsigned int sphereCount = scene.sphereMaterials.size() + GAMEPLAYER_PUPPET_SIZE;
	sphereMats.resize(sphereCount);

	// Translate scene material indices
	for (unsigned int i = 0; i < scene.sphereMaterials.size(); ++i) {
		Material *m = scene.sphereMaterials[i];

		// Look for the index
		unsigned int index = 0;
		for (unsigned int j = 0; j < scene.materials.size(); ++j) {
			if (m == scene.materials[j]) {
				index = j;
				break;
			}
		}

		sphereMats[i] = index;
	}

	// Translate player material indices
	for (unsigned int i = 0; i < GAMEPLAYER_PUPPET_SIZE; ++i)
		sphereMats[i + scene.sphereMaterials.size()] = i + scene.materials.size();

	const double tEnd = WallClockTime();
	SFERA_LOG("[OCLRenderer] Material compilation time: " << int((tEnd - tStart) * 1000.0) << "ms");
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

	return samplePerPass * width * height;
}

#endif
