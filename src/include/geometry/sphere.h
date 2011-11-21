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

#ifndef _SFERA_SPHERE_H
#define	_SFERA_SPHERE_H

#include "geometry/ray.h"

class Sphere {
public:
	Sphere() { };
	Sphere(const Point &c, const float r) : center(c), rad(r) { };
	~Sphere() { };

	bool Intersect(Ray *ray) const;
	bool IntersectP(const Ray *ray, float *hitT) const;

	float Area() const { return 4.f * M_PI * rad * rad; };

	bool Contains(const Sphere &s) const {
		if (s.rad + Distance(center, s.center) <= rad)
			return true;
		else
			return false;
	}

	float Mass() const { return CalcMass(rad); }
	float Volume() const { return CalcVolume(rad); }

	static float CalcMass(const float r) { return 0.75f * CalcVolume(r); }
	static float CalcVolume(const float r) { return (4.f / 3.f * M_PI) * r * r * r; }

	Point center;
	float rad;
};

inline Sphere Union(const Sphere &s0, const Sphere &s1) {
	const Point pMin(
		Min(s0.center.x - s0.rad, s1.center.x - s1.rad),
		Min(s0.center.y - s0.rad, s1.center.y - s1.rad),
		Min(s0.center.z - s0.rad, s1.center.z - s1.rad));
	const Point pMax(
		Max(s0.center.x + s0.rad, s1.center.x + s1.rad),
		Max(s0.center.y + s0.rad, s1.center.y + s1.rad),
		Max(s0.center.z + s0.rad, s1.center.z + s1.rad));

	const Point center = .5f * (pMin + pMax);
	return Sphere(center, Distance(center, pMax));
}

/*inline Sphere Union(const Sphere &s0, const Sphere &s1) {
	const Vector vdist = s1.center - s0.center;
	const float dist = vdist.Length();

	if (s1.rad + dist <= s0.rad)
		return s0;
	else if (s0.rad + dist <= s1.rad)
		return s1;
	else {
		const float r = (s0.rad + dist + s1.rad) * .5f;
		const float ratio = (r - s0.rad) / dist;

		return Sphere(Point(vdist.x * ratio ,vdist.y * ratio ,vdist.z * ratio), r);
	}
}*/

inline std::ostream & operator<<(std::ostream &os, const Sphere &s) {
	os << "Sphere[" << s.center << ", " << s.rad << "]";
	return os;
}

#endif	/* _SFERA_SPHERE_H */
