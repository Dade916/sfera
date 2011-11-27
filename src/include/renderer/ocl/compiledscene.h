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

#ifndef _SFERA_COMPILEDSCENE_H
#define	_SFERA_COMPILEDSCENE_H

#if !defined(SFERA_DISABLE_OPENCL)

#include "gamelevel.h"
#include "acceleretor/bvhaccel.h"
#include "sdl/editaction.h"

namespace compiledscene {

typedef struct {
	float lensRadius;
	float focalDistance;
	float yon, hither;

	float rasterToCameraMatrix[4][4];
	float cameraToWorldMatrix[4][4];
} Camera;

typedef struct {
	unsigned int rgbOffset;
	unsigned int width, height;
} TexMap;

typedef struct {
	unsigned int texMapIndex;
	float shiftU, shiftV;
	float scaleU, scaleV;
} TexMapInstance;

typedef struct {
	unsigned int texMapIndex;
	float shiftU, shiftV;
	float scaleU, scaleV;
	float scale;
} BumpMapInstance;

#define MAT_MATTE 0
#define MAT_MIRROR 1
#define MAT_GLASS 2
#define MAT_METAL 3
#define MAT_ALLOY 4

typedef struct {
    float r, g, b;
} MatteParam;

typedef struct {
    float r, g, b;
} MirrorParam;

typedef struct {
    float refl_r, refl_g, refl_b;
    float refrct_r, refrct_g, refrct_b;
    float ousideIor, ior;
    float R0;
} GlassParam;

typedef struct {
    float r, g, b;
    float exponent;
} MetalParam;

typedef struct {
    float diff_r, diff_g, diff_b;
    float refl_r, refl_g, refl_b;
    float exponent;
    float R0;
} AlloyParam;

typedef struct {
	unsigned int type;
	float emi_r, emi_g, emi_b;

	union {
		MatteParam matte;
		MirrorParam mirror;
        GlassParam glass;
        MetalParam metal;
        AlloyParam alloy;
	} param;
} Material;

}

class CompiledScene {
public:
	CompiledScene(const GameLevel *level);
	~CompiledScene();

	void Recompile(const EditActionList &editActions);

	compiledscene::Camera camera;

	BVHAccel *accel;

	// Compiled Materials
	bool enable_MAT_MATTE, enable_MAT_MIRROR, enable_MAT_GLASS,
		enable_MAT_METAL, enable_MAT_ALLOY;
	vector<compiledscene::Material> mats;
	vector<unsigned int> sphereMats;

	// Compiled TextureMaps
	vector<compiledscene::TexMap> texMaps;
	unsigned int totRGBTexMem;
	Spectrum *rgbTexMem;

	vector<compiledscene::TexMapInstance> sphereTexs;
	vector<compiledscene::BumpMapInstance> sphereBumps;

	EditActionList editActionsUsed;

private:
	void CompileCamera();
	void CompileGeometry();
	void CompileMaterial(Material *m, compiledscene::Material *gpum);
	void CompileMaterials();
	void CompileTextureMaps();

	const GameLevel *gameLevel;
};

#endif

#endif	/* _SFERA_COMPILEDSCENE_H */
