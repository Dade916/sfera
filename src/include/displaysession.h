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

#ifndef _SFERA_DISPLAYSESSION_H
#define	_SFERA_DISPLAYSESSION_H

#include "sfera.h"
#include "gameconfig.h"
#include "gamesession.h"
#include "utils/rendertext.h"

class DisplaySession {
public:
	DisplaySession(const GameConfig *cfg);
	~DisplaySession();

	void RunGame();

	const GameConfig *gameConfig;

private:
	void DrawLevelLabels(const string &bottomLabel, const string &topLabel) const;

	bool RunIntro();
	bool RunStartMenu();
	void RunShowHighScores();
	bool RunPackSelection(string *pack);
	void RunGameSession(const string &pack);
	bool RunLevel(GameSession &gameSession);

	SDL_Surface *screenSurface;

	TTF_Font *fontSmall, *fontMedium, *fontBig;
	RenderText *renderText;
};

#endif	/* _SFERA_DISPLAYSESSION_H */

