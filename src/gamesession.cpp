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
	gameConfig(cfg), packName(pack), currentLevel(NULL), currentLevelNumber(0),
	totalLevelsTime(0.0) {
	// Build the list of levels
	string packDir("gamedata/packs/" + pack);
	SFERA_LOG("Looking for levels inside pack: " << packDir);
	try {
		path packPath(packDir);

		if (!exists(packPath))
			throw runtime_error(packDir + " directory doesn't exist");
		if (!is_directory(packPath))
			throw runtime_error(packDir + " is not a directory");

        vector<path> levelFiles;
        copy(directory_iterator(packPath), directory_iterator(), back_inserter(levelFiles));
        sort(levelFiles.begin(), levelFiles.end());

		unsigned int level = 1;
		stringstream ss;
		ss << "lvl" << std::setw(2) << std::setfill('0') << level << "-";
		string levelPrefix = ss.str();
		for (vector<path>::const_iterator it(levelFiles.begin()); it != levelFiles.end(); ++it) {
			string levelName = it->filename();
			SFERA_LOG("  " << levelName);

			// Check if it is the definition of the level
			if (levelName.substr(0, 6) == ss.str()) {
				SFERA_LOG("    Used for level: " << level);
				levelNames.push_back(levelName.substr(6));

				// Look for the next level
				++level;
				ss.str("");
				ss << "lvl" << std::setw(2) << std::setfill('0') << level << "-";
				levelPrefix = ss.str();
			}
		}
	} catch (const filesystem_error &ex) {
		SFERA_LOG("Error reading the level pack: " << packDir << endl << "Error: "<< ex.what());
	}
}

GameSession::~GameSession() {
	delete currentLevel;
}

void GameSession::Begin(const unsigned int startLevel) {
	currentLevelNumber = startLevel;
	LoadLevel(currentLevelNumber);
}

bool GameSession::NextLevel() {
	++currentLevelNumber;

	// Check if there is a next level
	if (currentLevelNumber > levelNames.size())
		return false;

	LoadLevel(currentLevelNumber);

	return true;
}

void GameSession::LoadLevel(const unsigned int level) {
	stringstream ss;
	ss << "gamedata/packs/" + packName + "/lvl" << std::setw(2) << std::setfill('0') << level << "-" + levelNames[level - 1];

	currentLevel = new GameLevel(gameConfig, ss.str());
}
