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

GameLevel::GameLevel(const GameConfig *cfg, const string &levelFileName) : gameConfig(cfg) {
	SFERA_LOG("Reading level: " << levelFileName);

	Properties lvlProp(levelFileName);

	scene = new Scene(lvlProp);

	// Read player information
	const std::vector<float> vf = Properties::GetParameters(lvlProp, "player.position", 3, "0.0 0.0 1.0");
	player = new GamePlayer(Point(vf[0], vf[1], vf[2]));
}

GameLevel::~GameLevel() {
	delete scene;
	delete player;
}
