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
#include "sdl/sdl.h"
#include "sdl/light.h"

Scene::Scene(const Properties &scnProp, TextureMapCache *texMapCache) {
	gravityConstant = scnProp.GetFloat("scene.gravity.constant", 10.f);

	//--------------------------------------------------------------------------
	// Read the infinite light source
	//--------------------------------------------------------------------------

	const vector<string> ilParams = scnProp.GetStringVector("scene.infinitelight.file", "");
	if (ilParams.size() > 0) {
		TexMapInstance *tex = texMapCache->GetTexMapInstance(ilParams[0],
				0.f, 0.f, 1.f, 1.f);
		infiniteLight = new InfiniteLight(tex);

		vector<float> vf = Properties::GetParameters(scnProp, "scene.infinitelight.gain", 3, "1.0 1.0 1.0");
		infiniteLight->SetGain(Spectrum(vf[0], vf[1], vf[2]));

		vf = Properties::GetParameters(scnProp, "scene.infinitelight.shift", 2, "0.0 0.0");
		infiniteLight->SetShift(vf[0], vf[1]);
	} else
		throw runtime_error("Missing infinite light source definition");


	//--------------------------------------------------------------------------
	// Read all materials
	//--------------------------------------------------------------------------

	vector<string> matKeys = scnProp.GetAllKeys("scene.materials.");
	if (matKeys.size() == 0)
		throw runtime_error("No material definition found");

	for (vector<string>::const_iterator matKey = matKeys.begin(); matKey != matKeys.end(); ++matKey) {
		const string &key = *matKey;

		const string matName = Properties::ExtractField(key, 2);
		if (matName == "")
			throw runtime_error("Syntax error in " + key);

		// Check if the material has been already defined
		if (materialIndices.count(matName) > 0)
			continue;

		CreateMaterial("scene.materials." + matName, scnProp);
	}

	//--------------------------------------------------------------------------
	// Read all spheres
	//--------------------------------------------------------------------------

	vector<string> objKeys = scnProp.GetAllKeys("scene.spheres.");
	if (objKeys.size() == 0)
		throw runtime_error("No object definition found");

	double lastPrint = WallClockTime();
	for (vector<string>::const_iterator objKey = objKeys.begin(); objKey != objKeys.end(); ++objKey) {
		const string &key = *objKey;

		const string sphereName = Properties::ExtractField(key, 2);
		if (sphereName == "")
			throw runtime_error("Syntax error in " + key);

		// Check if the object has been already defined
		if (sphereIndices.count(sphereName) > 0)
			continue;

		const string propRoot = "scene.spheres." + sphereName;

		// Build the sphere
		const vector<float> vf = Properties::GetParameters(scnProp, propRoot + ".geometry", 4, "0.0 0.0 0.0 1.0");
		const float mass = scnProp.GetFloat(propRoot + ".mass", 1.f);
		const bool staticObject = ("yes" == scnProp.GetString(propRoot + ".static", "no"));
		const bool attractorObject = ("yes" == scnProp.GetString(propRoot + ".attractor", "no"));

		sphereIndices[sphereName] = spheres.size();
		spheres.push_back(GameSphere(Point(vf[0], vf[1], vf[2]), vf[3], mass,
				staticObject, attractorObject));

		const double now = WallClockTime();
		if (now - lastPrint > 2.0) {
			SFERA_LOG("Spheres count: " << spheres.size());
			lastPrint = now;
		}

		// Get the material
		const std::string matName = scnProp.GetString(propRoot + ".material", "");
		if (matName == "")
			throw std::runtime_error("Syntax error in material name of sphere: " + sphereName);
		if (materialIndices.count(matName) < 1)
			throw std::runtime_error("Unknown material: " + matName);
		Material *mat = materials[materialIndices[matName]];
		sphereMaterials.push_back(mat);

		// Get the texture map
		const std::string texName = scnProp.GetString(propRoot + ".texmap.file", "");
		if (texName != "") {
			const vector<float> vfshift = Properties::GetParameters(scnProp, propRoot + ".texmap.shift", 2, "0.0 0.0");
			const vector<float> vfscale = Properties::GetParameters(scnProp, propRoot + ".texmap.scale", 2, "1.0 1.0");

			TexMapInstance *tmi = texMapCache->GetTexMapInstance(texName,
					vfshift[0], vfshift[1], vfscale[0], vfscale[1]);
			sphereTexMaps.push_back(tmi);
		} else
			sphereTexMaps.push_back(NULL);

		// Get the bump map
		const std::string bumpName = scnProp.GetString(propRoot + ".bumpmap.file", "");
		if (bumpName != "") {
			const vector<float> vfshift = Properties::GetParameters(scnProp, propRoot + ".bumpmap.shift", 2, "0.0 0.0");
			const vector<float> vfscaleuv = Properties::GetParameters(scnProp, propRoot + ".bumpmap.scaleuv", 2, "1.0 1.0");
			const float scale = scnProp.GetFloat(propRoot + ".bumpmap.scale", 1.f);

			BumpMapInstance *bmi = texMapCache->GetBumpMapInstance(bumpName,
					vfshift[0], vfshift[1], vfscaleuv[0], vfscaleuv[1], scale);
			sphereBumpMaps.push_back(bmi);
		} else
			sphereBumpMaps.push_back(NULL);
	}
	SFERA_LOG("Spheres count: " << spheres.size());
}

Scene::~Scene() {
	delete infiniteLight;
}

void Scene::CreateMaterial(const string &propName, const Properties &prop) {
	const string matName = Properties::ExtractField(propName, 2);
	if (matName == "")
		throw runtime_error("Syntax error in " + propName);
	const string matType = prop.GetString(propName + ".type", "");
	if (matType == "")
		throw runtime_error("Syntax error in " + propName);
	SFERA_LOG("Material definition: " << matName << " [" << matType << "]");

	Material *mat;
	if (matType == "MATTE") {
		const vector<float> vf = Properties::GetParameters(prop, propName + ".params", 3, "1.0 1.0 1.0");
		const Spectrum col(vf[0], vf[1], vf[2]);

			mat = new MatteMaterial(col);
	} else if (matType == "MIRROR") {
		const vector<float> vf = Properties::GetParameters(prop, propName, 4, "1.0 1.0 1.0 1.0");
		const Spectrum col(vf[0], vf[1], vf[2]);

		mat = new MirrorMaterial(col, vf[3] != 0.f);
	} else if (matType == "GLASS") {
		const vector<float> vf = Properties::GetParameters(prop, propName, 10, "1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.5 1.0 1.0");
		const Spectrum Krfl(vf[0], vf[1], vf[2]);
		const Spectrum Ktrn(vf[3], vf[4], vf[5]);

		mat = new GlassMaterial(Krfl, Ktrn, vf[6], vf[7], vf[8] != 0.f, vf[9] != 0.f);
	} else if (matType == "METAL") {
		const vector<float> vf = Properties::GetParameters(prop, propName, 5, "1.0 1.0 1.0 10.0 1.0");
		const Spectrum col(vf[0], vf[1], vf[2]);

		mat = new MetalMaterial(col, vf[3], vf[4] != 0.f);
	} else if (matType == "ALLOY") {
		const vector<float> vf = Properties::GetParameters(prop, propName, 9, "1.0 1.0 1.0 1.0 1.0 1.0 10.0 0.8 1.0");
		const Spectrum Kdiff(vf[0], vf[1], vf[2]);
		const Spectrum Krfl(vf[3], vf[4], vf[5]);

		mat = new AlloyMaterial(Kdiff, Krfl, vf[6], vf[7], vf[8] != 0.f);
	} else
		throw runtime_error("Unknown material type " + matType);

	if (prop.IsDefined(propName + ".emission")) {
		const vector<float> vf = Properties::GetParameters(prop, propName + ".emission", 3, "0.0 0.0 0.0");
		const Spectrum e(vf[0], vf[1], vf[2]);

		mat->SetEmission(e);
	}

	materialIndices[matName] = materials.size();
	materials.push_back(mat);
}
