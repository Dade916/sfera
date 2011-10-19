/***************************************************************************
 *   Copyright (C) 1998-2010 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#include "sfera.h"
#include "sdl/sdl.h"

Scene::Scene(const std::string &fileName) {
	SFERA_LOG("Reading scene: " << fileName);

	Properties scnProp(fileName);

	//--------------------------------------------------------------------------
	// Read all spheres
	//--------------------------------------------------------------------------

	std::vector<std::string> objKeys = scnProp.GetAllKeys("scene.spheres.");
	if (objKeys.size() == 0)
		throw std::runtime_error("Unable to find object definitions");

	double lastPrint = WallClockTime();
	for (std::vector<std::string>::const_iterator objKey = objKeys.begin(); objKey != objKeys.end(); ++objKey) {
		const std::string &key = *objKey;

		// Check if it is the root of the definition of an object otherwise skip
		const size_t dot1 = key.find(".", std::string("scene.spheres.").length());
		if (dot1 == std::string::npos)
			continue;
		const size_t dot2 = key.find(".", dot1 + 1);
		if (dot2 != std::string::npos)
			continue;

		const std::string sphereName = Properties::ExtractField(key, 3);
		if (sphereName == "")
			throw std::runtime_error("Syntax error in " + key);

		// Build the sphere
		const std::vector<float> vf = GetParameters(scnProp, key + ".geometry", 4, "0.0 0.0 0.0 1.0");

		spheres.push_back(Sphere(Point(vf[0], vf[1], vf[2]), vf[3]));

		const double now = WallClockTime();
		if (now - lastPrint > 2.0) {
			SFERA_LOG("Spheres count: " << spheres.size());
			lastPrint = now;
		}
	}
	SFERA_LOG("Spheres count: " << spheres.size());
}

Scene::~Scene() {
}

std::vector<float> Scene::GetParameters(const Properties &prop, const std::string &paramName,
		const unsigned int paramCount, const std::string &defaultValue) {
	const std::vector<float> vf = prop.GetFloatVector(paramName, defaultValue);
	if (vf.size() != paramCount) {
		std::stringstream ss;
		ss << "Syntax error in " << paramName << " (required " << paramCount << " parameters)";
		throw std::runtime_error(ss.str());
	}

	return vf;
}
