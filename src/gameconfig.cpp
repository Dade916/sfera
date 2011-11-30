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

#include "gameconfig.h"

const string GameConfig::SCREEN_WIDTH = "screen.width";
const string GameConfig::SCREEN_WIDTH_DEFAULT = "640";
const string GameConfig::SCREEN_HEIGHT = "screen.height";
const string GameConfig::SCREEN_HEIGHT_DEFAULT = "360";
const string GameConfig::SCREEN_REFRESH_CAP = "screen.refresh.cap";
const string GameConfig::SCREEN_REFRESH_CAP_DEFAULT = "60";
const string GameConfig::SCREEN_FONT_NAME = "screen.font.name";
const string GameConfig::SCREEN_FONT_NAME_DEFAULT = "gamedata/fonts/VeraBd.ttf";
const string GameConfig::SCREEN_FONT_SIZE = "screen.font.size";
const string GameConfig::SCREEN_FONT_SIZE_DEFAULT = "14";
const string GameConfig::PHYSIC_REFRESH_RATE = "physic.refresh.rate";
const string GameConfig::PHYSIC_REFRESH_RATE_DEFAULT = "60";
const string GameConfig::RENDERER_SAMPLEPERPASS = "renderer.sampleperpass";
const string GameConfig::RENDERER_SAMPLEPERPASS_DEFAULT = "3";
const string GameConfig::RENDERER_GHOSTFACTOR_CAMERAEDIT = "renderer.ghostfactor.cameraedit";
const string GameConfig::RENDERER_GHOSTFACTOR_CAMERAEDIT_DEFAULT = "0.75";
const string GameConfig::RENDERER_GHOSTFACTOR_NOCAMERAEDIT = "renderer.ghostfactor.nocameraedit";
const string GameConfig::RENDERER_GHOSTFACTOR_NOCAMERAEDIT_DEFAULT = "0.05";
const string GameConfig::RENDERER_GHOSTFACTOR_TIME = "renderer.ghostfactor.time";
const string GameConfig::RENDERER_GHOSTFACTOR_TIME_DEFAULT = "1.5";
const string GameConfig::RENDERER_FILTER_TYPE = "renderer.filter.type";
const string GameConfig::RENDERER_FILTER_TYPE_DEFAULT = "BLUR_LIGHT";
const string GameConfig::RENDERER_FILTER_RADIUS = "renderer.filter.radius";
const string GameConfig::RENDERER_FILTER_RADIUS_DEFAULT = "1";
const string GameConfig::RENDERER_FILTER_ITERATIONS = "renderer.filter.iterations";
const string GameConfig::RENDERER_FILTER_ITERATIONS_DEFAULT = "3";
const string GameConfig::RENDERER_TYPE = "renderer.type";
#if !defined(SFERA_DISABLE_OPENCL)
const string GameConfig::RENDERER_TYPE_DEFAULT = "OPENCL";
#else
const string GameConfig::RENDERER_TYPE_DEFAULT = "MULTICPU";
#endif
const string GameConfig::OPENCL_DEVICES_USEONLYGPUS = "opencl.devices.useonlygpus";
const string GameConfig::OPENCL_DEVICES_USEONLYGPUS_DEFAULT = "true";
const string GameConfig::OPENCL_DEVICES_SELECT = "opencl.devices.select";
const string GameConfig::OPENCL_DEVICES_SELECT_DEFAULT = "";
const string GameConfig::OPENCL_MEMTYPE = "opencl.memtype";
const string GameConfig::OPENCL_MEMTYPE_DEFAULT = "__constant";

GameConfig::GameConfig(const string &fileName) {
	InitValues();

	SFERA_LOG("Reading configuration file: " << fileName);
	cfg.LoadFile(fileName);
	InitCachedValues();
}

GameConfig::GameConfig() {
	InitValues();
	InitCachedValues();
}

GameConfig::~GameConfig() {
}

void GameConfig::LogParameters() {
	SFERA_LOG("Configuration: ");
	vector<string> keys = cfg.GetAllKeys();
	for (vector<string>::iterator i = keys.begin(); i != keys.end(); ++i)
		SFERA_LOG("  " << *i << " = " << cfg.GetString(*i, ""));
}

void GameConfig::LoadProperties(const Properties &prop) {
	cfg.Load(prop);

	InitCachedValues();
}

void GameConfig::InitValues() {
	// Default configuration parameters
	cfg.SetString(SCREEN_WIDTH, SCREEN_WIDTH_DEFAULT);
	cfg.SetString(SCREEN_HEIGHT, SCREEN_HEIGHT_DEFAULT);
	cfg.SetString(SCREEN_REFRESH_CAP, SCREEN_REFRESH_CAP_DEFAULT);
	cfg.SetString(SCREEN_FONT_NAME, SCREEN_FONT_NAME_DEFAULT);
	cfg.SetString(SCREEN_FONT_SIZE, SCREEN_FONT_SIZE_DEFAULT);
	cfg.SetString(PHYSIC_REFRESH_RATE, PHYSIC_REFRESH_RATE_DEFAULT);
	cfg.SetString(RENDERER_SAMPLEPERPASS, RENDERER_SAMPLEPERPASS_DEFAULT);
	cfg.SetString(RENDERER_GHOSTFACTOR_CAMERAEDIT, RENDERER_GHOSTFACTOR_CAMERAEDIT_DEFAULT);
	cfg.SetString(RENDERER_GHOSTFACTOR_NOCAMERAEDIT, RENDERER_GHOSTFACTOR_NOCAMERAEDIT_DEFAULT);
	cfg.SetString(RENDERER_GHOSTFACTOR_TIME, RENDERER_GHOSTFACTOR_TIME_DEFAULT);
	cfg.SetString(RENDERER_FILTER_TYPE, RENDERER_FILTER_TYPE_DEFAULT);
	cfg.SetString(RENDERER_FILTER_RADIUS, RENDERER_FILTER_RADIUS_DEFAULT);
	cfg.SetString(RENDERER_FILTER_ITERATIONS, RENDERER_FILTER_ITERATIONS_DEFAULT);
	cfg.SetString(RENDERER_TYPE, RENDERER_TYPE_DEFAULT);
	cfg.SetString(OPENCL_DEVICES_USEONLYGPUS, OPENCL_DEVICES_USEONLYGPUS_DEFAULT);
	cfg.SetString(OPENCL_DEVICES_SELECT, OPENCL_DEVICES_SELECT_DEFAULT);
	cfg.SetString(OPENCL_MEMTYPE, OPENCL_MEMTYPE_DEFAULT);
}

void GameConfig::InitCachedValues() {
	screenWidth = (unsigned int)cfg.GetInt(SCREEN_WIDTH, atoi(SCREEN_WIDTH_DEFAULT.c_str()));
	screenHeight = (unsigned int)cfg.GetInt(SCREEN_HEIGHT, atoi(SCREEN_HEIGHT_DEFAULT.c_str()));
	screenRefreshCap = (unsigned int)cfg.GetInt(SCREEN_REFRESH_CAP, atoi(SCREEN_REFRESH_CAP_DEFAULT.c_str()));
	screenFontName = cfg.GetString(SCREEN_FONT_NAME, SCREEN_FONT_NAME_DEFAULT);
	screenFontSize = (unsigned int)cfg.GetInt(SCREEN_FONT_SIZE, atoi(SCREEN_FONT_SIZE_DEFAULT.c_str()));

	physicRefreshRate = (unsigned int)cfg.GetInt(PHYSIC_REFRESH_RATE, atoi(PHYSIC_REFRESH_RATE_DEFAULT.c_str()));

	rendererSamplePerPass = (unsigned int)cfg.GetInt(RENDERER_SAMPLEPERPASS, atoi(RENDERER_SAMPLEPERPASS_DEFAULT.c_str()));
	rendererGhostFactorCameraEdit = cfg.GetFloat(RENDERER_GHOSTFACTOR_CAMERAEDIT, atof(RENDERER_GHOSTFACTOR_CAMERAEDIT_DEFAULT.c_str()));
	rendererGhostFactorNoCameraEdit = cfg.GetFloat(RENDERER_GHOSTFACTOR_NOCAMERAEDIT, atof(RENDERER_GHOSTFACTOR_NOCAMERAEDIT_DEFAULT.c_str()));
	rendererGhostFactorTime = cfg.GetFloat(RENDERER_GHOSTFACTOR_TIME, atof(RENDERER_GHOSTFACTOR_TIME_DEFAULT.c_str()));

	string filterType = cfg.GetString(RENDERER_FILTER_TYPE, RENDERER_FILTER_TYPE_DEFAULT);
	if (filterType == "NO_FILTER")
		rendererFilterType = NO_FILTER;
	else if (filterType == "BLUR_LIGHT")
		rendererFilterType = BLUR_LIGHT;
	else if (filterType == "BLUR_HEAVY")
		rendererFilterType = BLUR_HEAVY;
	else if (filterType == "BOX")
		rendererFilterType = BOX;
	else
		throw runtime_error("Unknown filter type: " + filterType);

	rendererFilterRadius = (unsigned int)cfg.GetInt(RENDERER_FILTER_RADIUS, atoi(RENDERER_FILTER_RADIUS_DEFAULT.c_str()));
	rendererFilterIterations = (unsigned int)cfg.GetInt(RENDERER_FILTER_ITERATIONS, atoi(RENDERER_FILTER_ITERATIONS_DEFAULT.c_str()));

	string rendType = cfg.GetString(RENDERER_TYPE, RENDERER_TYPE_DEFAULT);
	if (rendType == "SINGLE_CPU")
		rendererType = SINGLE_CPU;
	else if (rendType == "MULTI_CPU")
		rendererType = MULTI_CPU;
#if !defined(SFERA_DISABLE_OPENCL)
	else if (rendType == "OPENCL")
		rendererType = OPENCL;
#endif
	else
		throw runtime_error("Unknown rendrer type: " + rendType);

	openCLUseOnlyGPUs = (cfg.GetString(OPENCL_DEVICES_USEONLYGPUS, OPENCL_DEVICES_USEONLYGPUS_DEFAULT) == "true");
	openCLDeviceSelect = cfg.GetString(OPENCL_DEVICES_SELECT, OPENCL_DEVICES_SELECT_DEFAULT);
	openCLMemType = cfg.GetString(OPENCL_MEMTYPE, OPENCL_MEMTYPE_DEFAULT);

	// Up to 64 devices
	openCLSamplePerPass.resize(64, rendererSamplePerPass);
	for (size_t i = 0; i < 64; ++i) {
		stringstream ss;
		ss << "opencl.devices." << i << ".sampleperpass";
		openCLSamplePerPass[i] = (unsigned int)cfg.GetInt(ss.str(), rendererSamplePerPass);
	}
}
