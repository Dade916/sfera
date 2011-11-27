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

#include "renderer/ocl/compiledscene.h"
#include "sdl/editaction.h"

CompiledScene::CompiledScene(const GameLevel *level) : gameLevel(level) {
	accel = NULL;
	totRGBTexMem = 0;
	rgbTexMem = NULL;

	CompileTextureMaps();

	EditActionList ea;
	ea.AddAllAction();

	Recompile(ea);
}

CompiledScene::~CompiledScene() {
	delete accel;
	delete[] rgbTexMem;
}

void CompiledScene::CompileCamera() {
	PerspectiveCamera &perpCamera(*(gameLevel->camera));
	camera.lensRadius = perpCamera.GetLensRadius();
	camera.focalDistance = perpCamera.GetFocalDistance();
	camera.yon = perpCamera.GetClipYon();
	camera.hither = perpCamera.GetClipHither();
	memcpy(camera.rasterToCameraMatrix, perpCamera.GetRasterToCameraMatrix().m, sizeof(float[4][4]));
	memcpy(camera.cameraToWorldMatrix, perpCamera.GetCameraToWorldMatrix().m, sizeof(float[4][4]));
}

void CompiledScene::CompileGeometry() {
	delete accel;

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
}

void CompiledScene::CompileMaterial(Material *m, compiledscene::Material *gpum) {
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

void CompiledScene::CompileMaterials() {
	SFERA_LOG("[OCLRenderer::CompiledScene] Compile Materials");

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
		compiledscene::Material *gpum = &mats[i];

		CompileMaterial(m, gpum);
	}

	// Add player materials
	for (unsigned int i = 0; i < GAMEPLAYER_PUPPET_SIZE; ++i) {
		Material *m = gameLevel->player->puppetMaterial[i];
		compiledscene::Material *gpum = &mats[i + scene.materials.size()];

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
	SFERA_LOG("[OCLRenderer::CompiledScene] Material compilation time: " << int((tEnd - tStart) * 1000.0) << "ms");
}

void CompiledScene::CompileTextureMaps() {
	SFERA_LOG("[OCLRenderer::CompiledScene] Compile TextureMaps");

	Scene &scene(*(gameLevel->scene));

	texMaps.resize(0);
	sphereTexs.resize(0);
	sphereBumps.resize(0);
	delete[] rgbTexMem;

	//--------------------------------------------------------------------------
	// Translate sphere texture maps
	//--------------------------------------------------------------------------

	const double tStart = WallClockTime();

	std::vector<TextureMap *> tms;
	gameLevel->texMapCache->GetTexMaps(tms);
	// Calculate the amount of ram to allocate
	totRGBTexMem = 0;

	for (unsigned int i = 0; i < tms.size(); ++i) {
		TextureMap *tm = tms[i];
		const unsigned int pixelCount = tm->GetWidth() * tm->GetHeight();

		totRGBTexMem += pixelCount;
	}

	// Allocate texture map memory
	if (totRGBTexMem > 0) {
		texMaps.resize(tms.size());

		if (totRGBTexMem > 0) {
			unsigned int rgbOffset = 0;
			rgbTexMem = new Spectrum[totRGBTexMem];

			for (unsigned int i = 0; i < tms.size(); ++i) {
				TextureMap *tm = tms[i];
				const unsigned int pixelCount = tm->GetWidth() * tm->GetHeight();

				memcpy(&rgbTexMem[rgbOffset], tm->GetPixels(), pixelCount * sizeof(Spectrum));
				texMaps[i].rgbOffset = rgbOffset;
				texMaps[i].width = tm->GetWidth();
				texMaps[i].height = tm->GetHeight();

				rgbOffset += pixelCount;
			}
		} else
			rgbTexMem = NULL;

		//----------------------------------------------------------------------

		// Translate sphere texture instances
		const unsigned int sphereCount = scene.spheres.size();
		sphereTexs.resize(sphereCount + GAMEPLAYER_PUPPET_SIZE);
		for (unsigned int i = 0; i < sphereCount; ++i) {
			TexMapInstance *t = scene.sphereTexMaps[i];

			if (t) {
				// Look for the index
				unsigned int index = 0;
				for (unsigned int j = 0; j < tms.size(); ++j) {
					if (t->GetTexMap() == tms[j]) {
						index = j;
						break;
					}
				}

				sphereTexs[i].texMapIndex = index;
				sphereTexs[i].shiftU = t->GetShiftU();
				sphereTexs[i].shiftV = t->GetShiftV();
				sphereTexs[i].scaleU = t->GetScaleU();
				sphereTexs[i].scaleV = t->GetScaleV();
			} else
				sphereTexs[i].texMapIndex = 0xffffffffu;
		}

		// Player puppet has no texture maps
		for (unsigned int i = 0; i < GAMEPLAYER_PUPPET_SIZE; ++i)
			sphereTexs[i + sphereCount].texMapIndex = 0xffffffffu;

		//----------------------------------------------------------------------

		// Translate sphere bump map instances
		bool hasBumpMapping = false;
		sphereBumps.resize(sphereCount + GAMEPLAYER_PUPPET_SIZE);
		for (unsigned int i = 0; i < sphereCount; ++i) {
			BumpMapInstance *b = scene.sphereBumpMaps[i];

			if (b) {
				// Look for the index
				unsigned int index = 0;
				for (unsigned int j = 0; j < tms.size(); ++j) {
					if (b->GetTexMap() == tms[j]) {
						index = j;
						break;
					}
				}

				sphereBumps[i].texMapIndex = index;
				sphereBumps[i].shiftU = b->GetShiftU();
				sphereBumps[i].shiftV = b->GetShiftV();
				sphereBumps[i].scaleU = b->GetScaleU();
				sphereBumps[i].scaleV = b->GetScaleV();
				sphereBumps[i].scale = b->GetScale();
				hasBumpMapping = true;
			} else
				sphereBumps[i].texMapIndex = 0xffffffffu;
		}

		// Player puppet has no bump maps
		for (unsigned int i = 0; i < GAMEPLAYER_PUPPET_SIZE; ++i)
			sphereBumps[i + sphereCount].texMapIndex = 0xffffffffu;

		if (!hasBumpMapping)
			sphereBumps.resize(0);
	} else {
		texMaps.resize(0);
		sphereTexs.resize(0);
		sphereBumps.resize(0);
		rgbTexMem = NULL;
	}

	const double tEnd = WallClockTime();
	SFERA_LOG("[OCLRenderer::CompiledScene] Texture maps compilation time: " << int((tEnd - tStart) * 1000.0) << "ms");
}

void CompiledScene::Recompile(const EditActionList &editActions) {
	editActionsUsed.Reset();

	if (editActions.Has(CAMERA_EDIT)) {
		editActionsUsed.AddAction(CAMERA_EDIT);
		CompileCamera();
	}
	if (editActions.Has(GEOMETRY_EDIT)) {
		editActionsUsed.AddAction(GEOMETRY_EDIT);
		CompileGeometry();
	}
	if (editActions.Has(MATERIALS_EDIT)) {
		editActionsUsed.AddAction(MATERIALS_EDIT);
		CompileMaterials();
	}
}

#endif
