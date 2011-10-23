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

#ifndef _SFERA_UV_H
#define _SFERA_UV_H

class UV {
public:
	// UV Methods

	UV(float _u = 0.f, float _v = 0.f)
	: u(_u), v(_v) {
	}

	UV(float v[2]) : u(v[0]), v(v[1]) {
	}


	UV & operator+=(const UV &p) {
		u += p.u;
		v += p.v;
		return *this;
	}

	UV & operator-=(const UV &p) {
		u -= p.u;
		v -= p.v;
		return *this;
	}

	UV operator+(const UV &p) const {
		return UV(u + p.u, v + p.v);
	}

	UV operator*(float f) const {
		return UV(f*u, f*v);
	}

	UV & operator*=(float f) {
		u *= f;
		v *= f;
		return *this;
	}

	UV operator/(float f) const {
		float inv = 1.f / f;
		return UV(inv*u, inv*v);
	}

	UV & operator/=(float f) {
		float inv = 1.f / f;
		u *= inv;
		v *= inv;
		return *this;
	}

	float operator[](int i) const {
		return (&u)[i];
	}

	float &operator[](int i) {
		return (&u)[i];
	}

	// UV Public Data
	float u, v;
};

inline std::ostream & operator<<(std::ostream &os, const UV &v) {
	os << "UV[" << v.u << ", " << v.v << "]";
	return os;
}

inline UV operator*(float f, const UV &p) {
	return p*f;
}

#endif	/* _SFERA_UV_H */
