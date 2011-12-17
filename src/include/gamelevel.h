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

#ifndef _SFERA_GAMELEVEL_H
#define	_SFERA_GAMELEVEL_H

#include "sfera.h"
#include "gameconfig.h"
#include "gameplayer.h"
#include "sdl/scene.h"
#include "sdl/camera.h"
#include "sdl/texmap.h"
#include "sdl/editaction.h"
#include "pixel/tonemap.h"

class GameLevel {
public:
	GameLevel(const GameConfig *cfg, const string &levelFileName);
	~GameLevel();

	void Refresh(const float refreshRate);

	mutable boost::mutex levelMutex;

	const GameConfig *gameConfig;

	unsigned int maxPathDiffuseBounces;
	unsigned int maxPathSpecularGlossyBounces;
	Scene *scene;
	TextureMapCache *texMapCache;
	ToneMap *toneMap;

	GamePlayer *player;
	PerspectiveCamera *camera;

	double startTime;
	unsigned int offPillCount;

	EditActionList editActionList;
};

#endif	/* _SFERA_GAMELEVEL_H */
