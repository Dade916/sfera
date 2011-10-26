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
	//--------------------------------------------------------------------------
	// Read the infinite light source
	//--------------------------------------------------------------------------

	const vector<string> ilParams = scnProp.GetStringVector("scene.infinitelight.file", "");
	if (ilParams.size() > 0) {
		TexMapInstance *tex = texMapCache->GetTexMapInstance(ilParams.at(0));
		infiniteLight = new InfiniteLight(tex);

		vector<float> vf = Properties::GetParameters(scnProp, "scene.infinitelight.gain", 3, "1.0 1.0 1.0");
		infiniteLight->SetGain(Spectrum(vf.at(0), vf.at(1), vf.at(2)));

		vf = Properties::GetParameters(scnProp, "scene.infinitelight.shift", 2, "0.0 0.0");
		infiniteLight->SetShift(vf.at(0), vf.at(1));
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

		sphereIndices[sphereName] = spheres.size();
		spheres.push_back(GameSphere(Point(vf[0], vf[1], vf[2]), vf[3], mass, staticObject));

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
		const Spectrum col(vf.at(0), vf.at(1), vf.at(2));

			mat = new MatteMaterial(col);
	} else if (matType == "MIRROR") {
		const vector<float> vf = Properties::GetParameters(prop, propName, 4, "1.0 1.0 1.0 1.0");
		const Spectrum col(vf.at(0), vf.at(1), vf.at(2));

		mat = new MirrorMaterial(col, vf.at(3) != 0.f);
	} else if (matType == "GLASS") {
		const vector<float> vf = Properties::GetParameters(prop, propName, 10, "1.0 1.0 1.0 1.0 1.0 1.0 1.0 1.5 1.0 1.0");
		const Spectrum Krfl(vf.at(0), vf.at(1), vf.at(2));
		const Spectrum Ktrn(vf.at(3), vf.at(4), vf.at(5));

		mat = new GlassMaterial(Krfl, Ktrn, vf.at(6), vf.at(7), vf.at(8) != 0.f, vf.at(9) != 0.f);
	} else if (matType == "METAL") {
		const vector<float> vf = Properties::GetParameters(prop, propName, 5, "1.0 1.0 1.0 10.0 1.0");
		const Spectrum col(vf.at(0), vf.at(1), vf.at(2));

		mat = new MetalMaterial(col, vf.at(3), vf.at(4) != 0.f);
	} else if (matType == "ALLOY") {
		const vector<float> vf = Properties::GetParameters(prop, propName, 9, "1.0 1.0 1.0 1.0 1.0 1.0 10.0 0.8 1.0");
		const Spectrum Kdiff(vf.at(0), vf.at(1), vf.at(2));
		const Spectrum Krfl(vf.at(3), vf.at(4), vf.at(5));

		mat = new AlloyMaterial(Kdiff, Krfl, vf.at(6), vf.at(7), vf.at(8) != 0.f);
	} else
		throw runtime_error("Unknown material type " + matType);

	if (prop.IsDefined(propName + ".emission")) {
		const vector<float> vf = Properties::GetParameters(prop, propName + ".emission", 3, "0.0 0.0 0.0");
		const Spectrum e(vf.at(0), vf.at(1), vf.at(2));

		mat->SetEmission(e);
	}

	materialIndices[matName] = materials.size();
	materials.push_back(mat);
}
