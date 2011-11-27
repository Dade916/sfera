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

#ifndef _SFERA_SDL_SCENE_H
#define	_SFERA_SDL_SCENE_H

#include <vector>
#include <map>

#include "sfera.h"
#include "utils/properties.h"
#include "gamesphere.h"
#include "sdl/material.h"
#include "sdl/texmap.h"
#include "sdl/light.h"

class Scene {
public:
	Scene(const Properties &scnProp, TextureMapCache *texMapCache);
	~Scene();

	float gravityConstant;

	InfiniteLight *infiniteLight;

	vector<Material *> materials; // All materials
	map<string, size_t> materialIndices; // All materials indices

	map<string, size_t> sphereIndices; // All object indices
	vector<GameSphere> spheres; // All sferes
	vector<Material *> sphereMaterials; // One for each object
	vector<TexMapInstance *> sphereTexMaps; // One for each object
	vector<BumpMapInstance *> sphereBumpMaps; // One for each object

	unsigned int pillCount;
	Material *pillOffMaterial;

private:
	void CreateMaterial(const string &propName, const Properties &prop);
};

#endif	/* _SFERA_SDL_SCENE_H */
