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
const string GameConfig::SCREEN_REFRESH_CAP_DEFAULT = "30";

GameConfig::GameConfig(const string &fileName) {
	InitValues();

	SFERA_LOG("Reading configuration file: " << fileName);
	cfg.LoadFile(fileName);
	InitCachedValues();

	LogParameters();
}

GameConfig::GameConfig() {
	InitValues();
	InitCachedValues();

	LogParameters();
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

unsigned int GameConfig::GetScreenWidth() const {
	return screenWidth;
}

unsigned int GameConfig::GetScreenHeight() const {
	return screenHeight;
}

unsigned int GameConfig::GetScreenRefreshCap() const {
	return screenRefreshCap;
}

void GameConfig::InitValues() {
	// Default configuration parameters
	cfg.SetString(SCREEN_WIDTH, SCREEN_WIDTH_DEFAULT);
	cfg.SetString(SCREEN_HEIGHT, SCREEN_HEIGHT_DEFAULT);
	cfg.SetString(SCREEN_REFRESH_CAP, SCREEN_REFRESH_CAP_DEFAULT);
}

void GameConfig::InitCachedValues() {
	screenWidth = (unsigned int)cfg.GetInt(SCREEN_WIDTH, atoi(SCREEN_WIDTH_DEFAULT.c_str()));
	screenHeight = (unsigned int)cfg.GetInt(SCREEN_HEIGHT, atoi(SCREEN_HEIGHT_DEFAULT.c_str()));
	screenRefreshCap = (unsigned int)cfg.GetInt(SCREEN_REFRESH_CAP, atoi(SCREEN_REFRESH_CAP_DEFAULT.c_str()));
}
