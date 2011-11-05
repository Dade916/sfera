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

#ifndef _SFERA_RAY_H
#define _SFERA_RAY_H

#include "epsilon.h"
#include "geometry/vector.h"
#include "geometry/point.h"

class  Ray {
public:
	// Ray Public Methods
	Ray() : maxt(std::numeric_limits<float>::infinity()) {
		mint = EPSILON;
	}

	Ray(const Point &origin, const Vector &direction)
		: o(origin), d(direction), maxt(std::numeric_limits<float>::infinity()) {
		mint = EPSILON;
	}

	Ray(const Point &origin, const Vector &direction,
		float start, float end = std::numeric_limits<float>::infinity())
		: o(origin), d(direction), mint(start), maxt(end) { }

	Point operator()(float t) const { return o + d * t; }
	void GetDirectionSigns(int signs[3]) const {
		signs[0] = d.x < 0.f;
		signs[1] = d.y < 0.f;
		signs[2] = d.z < 0.f;
	}

	// Ray Public Data
	Point o;
	Vector d;
	mutable float mint, maxt;
};

inline std::ostream &operator<<(std::ostream &os, const Ray &r) {
	os << "Ray[" << r.o << ", " << r.d << ", " << r.mint << "," << r.maxt << "]";
	return os;
}

#endif	/* _SFERA_RAY_H */
