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

#ifndef _SFERA_GAMESESSION_H
#define	_SFERA_GAMESESSION_H

#include "sfera.h"
#include "gameconfig.h"
#include "gamelevel.h"
#include "utils/packlevellist.h"
#include "utils/packhighscore.h"

class GameSession {
public:
	GameSession(const GameConfig *cfg, const string &pack);
	~GameSession();

	void Begin(const unsigned int startLevel = 1);
	bool NextLevel();

	unsigned int GetCurrentLevel() const { return currentLevelNumber; }
	const string &GetCurrentLevelName() const { return packLevelList.names[currentLevelNumber - 1]; }
	double GetTotalLevelsTime() const { return totalLevelsTime; }
	bool IsNewTotalTimeHighScore() {
		return ((totalLevelsTime > 0.0) && (
				(highScores.GetTotal() == 0.0) ||
				(totalLevelsTime < highScores.GetTotal())));
	}
	void SetTotalLevelsTime() {
		if (IsNewTotalTimeHighScore()) {
			highScores.SetTotal(totalLevelsTime);
			highScores.Save();
		}
	}
	bool IsNewHighScore(const double t) const {
		const double st = highScores.Get(currentLevelNumber);
		return (st == 0.f) || (st > t);
	}
	void SetLevelTime(const double t);
	double GetHighScore() const { return highScores.Get(currentLevelNumber); };
	double GetTotalTimeHighScore() const { return highScores.GetTotal(); };

	bool IsAllPackDone() const { return (currentLevelNumber > packLevelList.names.size()); }

	const GameConfig *gameConfig;
	const string packName;

	GameLevel *currentLevel;

private:
	void LoadLevel(const unsigned int level);

	PackLevelList packLevelList;
	unsigned int currentLevelNumber;

	PackHighScore highScores;

	double totalLevelsTime;
};

#endif	/* _SFERA_GAMESESSION_H */
