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

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

#include "utils/packhighscore.h"
#include "utils/packlevellist.h"

PackHighScore::PackHighScore(const PackLevelList *levelList) {
	packLevelList = levelList;

	// Read the high scores
	Properties *propScores;

	const string scoreFileName = "gamedata/scores/" + packLevelList->packName + ".scr";
	path scorePath(scoreFileName);
	if (!exists(scorePath))
		propScores = new Properties();
	else
		propScores = new Properties(scoreFileName);

	// Set the current high scores
	for (size_t i = 0; i < packLevelList->names.size(); ++i)
		scores.push_back(propScores->GetFloat("scores." + packLevelList->packName + ".level." + ToString(i + 1), 0.f));

	totalScore = propScores->GetFloat("scores." + packLevelList->packName + ".total", 0.f);
}

void PackHighScore::Save() const {
	Properties propScores;
	for (size_t i = 0; i < packLevelList->names.size(); ++i)
		propScores.SetString("scores." + packLevelList->packName + ".level." + ToString(i + 1),
				ToString(scores[i]));

	propScores.SetString("scores." + packLevelList->packName + ".total",
				ToString(totalScore));

	propScores.SaveFile("gamedata/scores/" + packLevelList->packName + ".scr");
}
