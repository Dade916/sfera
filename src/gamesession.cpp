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

#include <iterator>
#include <vector>
#include <algorithm>
#include <boost/filesystem.hpp>

#include "gamesession.h"

using namespace boost::filesystem;

GameSession::GameSession(const GameConfig *cfg, const string &pack) :
	gameConfig(cfg), packName(pack), currentLevel(NULL), packLevelList(packName),
	currentLevelNumber(0), highScores(&packLevelList), totalLevelsTime(0.0) {
}

GameSession::~GameSession() {
	// Save the high scores
	highScores.Save();

	delete currentLevel;
}

void GameSession::Begin(const unsigned int startLevel) {
	currentLevelNumber = startLevel;
	LoadLevel(currentLevelNumber);
}

bool GameSession::NextLevel() {
	++currentLevelNumber;

	// Check if there is a next level
	if (currentLevelNumber > packLevelList.names.size())
		return false;

	LoadLevel(currentLevelNumber);

	return true;
}

void GameSession::LoadLevel(const unsigned int level) {
	stringstream ss;
	ss << "gamedata/packs/" + packName + "/lvl" << std::setw(2) << std::setfill('0') <<
			level << "-" + packLevelList.names[level - 1] << ".lvl";

	currentLevel = new GameLevel(gameConfig, ss.str());
}

void GameSession::SetLevelTime(const double t) {
	totalLevelsTime += t;

	if (IsNewHighScore(t)) {
		highScores.Set(currentLevelNumber, t);

		// Better safe than sorry...
		highScores.Save();
	}
}
