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

#ifndef _SFERA_PACKHIGHSCORE_H
#define	_SFERA_PACKHIGHSCORE_H

#include "sfera.h"
#include "packlevellist.h"

class PackHighScore {
public:
	PackHighScore(const PackLevelList *packLevelList);
	~PackHighScore() { }

	void Save() const;

	double Get(size_t i) const { return scores[i - 1]; }
	void Set(size_t i, double t) { scores[i - 1] = t; }

	double GetTotal() const { return totalScore; }
	void SetTotal(const double t) { totalScore = t; }

private:
	const PackLevelList *packLevelList;

	vector<double> scores;
	double totalScore;
};

#endif	/* _SFERA_PACKHIGHSCORE_H */

