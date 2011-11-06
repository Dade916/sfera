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

#ifndef _SFERA_GAMECONFIG_H
#define	_SFERA_GAMECONFIG_H

#include "sfera.h"
#include "utils/properties.h"

class GameConfig {
public:
	GameConfig(const string &fileName);
	GameConfig();
	~GameConfig();

	void LogParameters();

	void LoadProperties(const Properties &prop);

	unsigned int GetScreenWidth() const { return screenWidth; }
	unsigned int GetScreenHeight() const { return screenHeight; }
	unsigned int GetScreenRefreshCap() const { return screenRefreshCap; }
	unsigned int GetPhysicRefreshRate() const { return physicRefreshRate; }

	// SINGLECPU parameters
	unsigned int GetRendererSamplePerPass() const { return rendererSamplePerPass; }
	float GetRendererGhostFactorCameraEdit() const { return rendererGhostFactorCameraEdit; }
	float GetRendererGhostFactorNoCameraEdit() const { return rendererGhostFactorNoCameraEdit; }
	unsigned int GetRendererGhostFilterRaidus() const { return rendererFilterRadius; }
	unsigned int GetRendererGhostFilterIterations() const { return rendererFilterIterations; }

private:
	// List of possible properties
	const static string SCREEN_WIDTH;
	const static string SCREEN_WIDTH_DEFAULT;
	const static string SCREEN_HEIGHT;
	const static string SCREEN_HEIGHT_DEFAULT;
	const static string SCREEN_REFRESH_CAP;
	const static string SCREEN_REFRESH_CAP_DEFAULT;
	const static string PHYSIC_REFRESH_RATE;
	const static string PHYSIC_REFRESH_RATE_DEFAULT;
	const static string RENDERER_SAMPLEPERPASS;
	const static string RENDERER_SAMPLEPERPASS_DEFAULT;
	const static string RENDERER_GHOSTFACTOR_CAMERAEDIT;
	const static string RENDERER_GHOSTFACTOR_CAMERAEDIT_DEFAULT;
	const static string RENDERER_GHOSTFACTOR_NOCAMERAEDIT;
	const static string RENDERER_GHOSTFACTOR_NOCAMERAEDIT_DEFAULT;
	const static string RENDERER_FILTER_RADIUS;
	const static string RENDERER_FILTER_RADIUS_DEFAULT;
	const static string RENDERER_FILTER_ITERATIONS;
	const static string RENDERER_FILTER_ITERATIONS_DEFAULT;

	void InitValues();
	void InitCachedValues();

	Properties cfg;

	// Cached values
	unsigned int screenWidth;
	unsigned int screenHeight;
	unsigned int screenRefreshCap;
	unsigned int physicRefreshRate;

	unsigned int rendererSamplePerPass;
	float rendererGhostFactorCameraEdit;
	float rendererGhostFactorNoCameraEdit;
	unsigned int rendererFilterRadius;
	unsigned int rendererFilterIterations;

};

#endif	/* _SFERA_GAMECONFIG_H */
