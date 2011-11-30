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

typedef enum {
	NO_FILTER, BLUR_LIGHT, BLUR_HEAVY, BOX
} FilterType;

typedef enum {
	SINGLE_CPU, MULTI_CPU, OPENCL
} RendererType;

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
	string GetScreenFontName() const { return screenFontName; }
	unsigned int GetScreenFontSize() const { return screenFontSize; }

	unsigned int GetPhysicRefreshRate() const { return physicRefreshRate; }

	// Renderer parameters
	unsigned int GetRendererSamplePerPass() const { return rendererSamplePerPass; }
	float GetRendererGhostFactorCameraEdit() const { return rendererGhostFactorCameraEdit; }
	float GetRendererGhostFactorNoCameraEdit() const { return rendererGhostFactorNoCameraEdit; }
	float GetRendererGhostFactorTime() const { return rendererGhostFactorTime; }
	FilterType GetRendererFilterType() const { return rendererFilterType; }
	unsigned int GetRendererFilterRaidus() const { return rendererFilterRadius; }
	unsigned int GetRendererFilterIterations() const { return rendererFilterIterations; }
	RendererType GetRendererType() const { return rendererType; }

	bool GetOpenCLUseOnlyGPUs() const { return openCLUseOnlyGPUs; }
	const string &GetOpenCLDeviceSelect() const { return openCLDeviceSelect; }
	unsigned int GetOpenCLDeviceSamplePerPass(const size_t index) const { return openCLSamplePerPass[index]; }
	const string &GetOpenCLMemType() const { return openCLMemType; }

private:
	// List of possible properties
	const static string SCREEN_WIDTH;
	const static string SCREEN_WIDTH_DEFAULT;
	const static string SCREEN_HEIGHT;
	const static string SCREEN_HEIGHT_DEFAULT;
	const static string SCREEN_REFRESH_CAP;
	const static string SCREEN_REFRESH_CAP_DEFAULT;
	const static string SCREEN_FONT_NAME;
	const static string SCREEN_FONT_NAME_DEFAULT;
	const static string SCREEN_FONT_SIZE;
	const static string SCREEN_FONT_SIZE_DEFAULT;
	const static string PHYSIC_REFRESH_RATE;
	const static string PHYSIC_REFRESH_RATE_DEFAULT;
	const static string RENDERER_SAMPLEPERPASS;
	const static string RENDERER_SAMPLEPERPASS_DEFAULT;
	const static string RENDERER_GHOSTFACTOR_CAMERAEDIT;
	const static string RENDERER_GHOSTFACTOR_CAMERAEDIT_DEFAULT;
	const static string RENDERER_GHOSTFACTOR_NOCAMERAEDIT;
	const static string RENDERER_GHOSTFACTOR_NOCAMERAEDIT_DEFAULT;
	const static string RENDERER_GHOSTFACTOR_TIME;
	const static string RENDERER_GHOSTFACTOR_TIME_DEFAULT;
	const static string RENDERER_FILTER_TYPE;
	const static string RENDERER_FILTER_TYPE_DEFAULT;
	const static string RENDERER_FILTER_RADIUS;
	const static string RENDERER_FILTER_RADIUS_DEFAULT;
	const static string RENDERER_FILTER_ITERATIONS;
	const static string RENDERER_FILTER_ITERATIONS_DEFAULT;
	const static string RENDERER_TYPE;
	const static string RENDERER_TYPE_DEFAULT;
	const static string OPENCL_DEVICES_USEONLYGPUS;
	const static string OPENCL_DEVICES_USEONLYGPUS_DEFAULT;
	const static string OPENCL_DEVICES_SELECT;
	const static string OPENCL_DEVICES_SELECT_DEFAULT;
	const static string OPENCL_MEMTYPE;
	const static string OPENCL_MEMTYPE_DEFAULT;

	void InitValues();
	void InitCachedValues();

	Properties cfg;

	// Cached values
	unsigned int screenWidth;
	unsigned int screenHeight;
	unsigned int screenRefreshCap;
	string screenFontName;
	unsigned int screenFontSize;
	unsigned int physicRefreshRate;

	unsigned int rendererSamplePerPass;
	float rendererGhostFactorCameraEdit;
	float rendererGhostFactorNoCameraEdit;
	float rendererGhostFactorTime;
	FilterType rendererFilterType;
	unsigned int rendererFilterRadius;
	unsigned int rendererFilterIterations;
	RendererType rendererType;

	bool openCLUseOnlyGPUs;
	string openCLDeviceSelect;
	string openCLMemType;

	vector<unsigned int> openCLSamplePerPass;
};

#endif	/* _SFERA_GAMECONFIG_H */
