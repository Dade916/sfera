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

#include "gamelevel.h"
#include "epsilon.h"
#include "sdl/editaction.h"

GameLevel::GameLevel(const GameConfig *cfg, const string &levelFileName) : gameConfig(cfg) {
	SFERA_LOG("Reading level: " << levelFileName);

	Properties lvlProp(levelFileName);
	texMapCache = new TextureMapCache();

	//--------------------------------------------------------------------------
	// Read the path tracing configuration
	//--------------------------------------------------------------------------

	EPSILON = lvlProp.GetFloat("path.epsilon", 0.001f);
	maxPathDiffuseBounces = lvlProp.GetInt("path.maxdiffusebounces", 2);
	maxPathSpecularGlossyBounces = lvlProp.GetInt("path.maxspecularglossybounces", 4);

	//--------------------------------------------------------------------------
	// Read the scene
	//--------------------------------------------------------------------------

	scene = new Scene(lvlProp, texMapCache);
	offPillCount = 0;

	//--------------------------------------------------------------------------
	// Read tone mapping information
	//--------------------------------------------------------------------------

	const float gamma = lvlProp.GetFloat("film.gamma", 2.2f);

	const string toneMapType = lvlProp.GetString("film.tonemap.type", "LINEAR");
	if (toneMapType == "LINEAR") {
		const float scale = lvlProp.GetFloat("film.tonemap.linear.scale", 1.f);
		toneMap = new LinearToneMap(gamma, scale);
	} else if (toneMapType == "REINHARD02") {
		const float preScale = lvlProp.GetFloat("film.tonemap.reinhard02.prescale", 1.f);
		const float postScale = lvlProp.GetFloat("film.tonemap.reinhard02.postscale", 1.2f);
		const float burn = lvlProp.GetFloat("film.tonemap.reinhard02.burn", 3.75f);
		toneMap = new Reinhard02ToneMap(gamma, preScale, postScale, burn);
	} else
		throw std::runtime_error("Missing tone mapping definition");

	//--------------------------------------------------------------------------
	// Read player information
	//--------------------------------------------------------------------------

	player = new GamePlayer(lvlProp);
	// The body is always added as the last sfere to the scene
	player->body.index = scene->spheres.size();

	camera = new PerspectiveCamera(
		Point(0.f, 0.f, 1.f),
		Point(0.f, 0.f, 0.f),
		Vector(0.f, 0.f, 1.f));
	player->UpdateCamera(*camera, gameConfig->GetScreenWidth(), gameConfig->GetScreenHeight());

	editActionList.AddAllAction();

	startTime = WallClockTime();
}

GameLevel::~GameLevel() {
	delete scene;
	delete player;
	delete camera;
	delete texMapCache;
	delete toneMap;
}

void GameLevel::Refresh(const float refreshRate) {
	player->UpdateLocalCoordSystem();
	player->ApplyInputs(refreshRate);
	player->UpdatePuppet();
	player->UpdateCamera(*camera, gameConfig->GetScreenWidth(), gameConfig->GetScreenHeight());
}