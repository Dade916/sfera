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

#ifndef _SFERA_SDL_GAMESPHERE_H
#define	_SFERA_SDL_GAMESPHERE_H

#include "geometry/sphere.h"

class GameSphere {
public:
	GameSphere() { }
	GameSphere(const size_t i,
		const Point &c, const float r,
		const float m, const float linearDamp, const float angularDamp,
		const bool st, const bool at, const bool pl, const bool pu) :
		index(i), sphere(c, r),
		mass(m), linearDamping(linearDamp), angularDamping(angularDamp),
		staticObject(st), attractorObject(at), pillObject(pl), puppetObject(pu),
		isPillOff(false) { }
	~GameSphere() { }

	size_t index;
	Sphere sphere;
	float mass, linearDamping, angularDamping;

	bool staticObject, attractorObject, pillObject, puppetObject;
	bool isPillOff;
};

#endif	/* _SFERA_SDL_GAMESPHERE_H */
