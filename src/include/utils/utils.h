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

#ifndef _SFERA_UTILS_H
#define	_SFERA_UTILS_H

#include <cmath>

#if defined (__linux__)
#include <pthread.h>
#endif

#include <boost/thread.hpp>

#if defined(WIN32)
#include <float.h>
#define isnanf(a) _isnan(a)
#if !defined(isnan)
#define isnan(a) _isnan(a)
#endif
typedef unsigned int u_int;
#else
using std::isnan;
#endif

#if defined(__APPLE__) // OSX adaptions Jens Verwiebe
#  define memalign(a,b) valloc(b)
#include <string>
typedef unsigned int u_int;
#endif

#if defined(__APPLE__)
using std::isinf;
#endif

#if defined(__APPLE__)
#include <stdlib.h>
#else
#include <malloc.h>
#endif

#include <sstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef INFINITY
#define INFINITY (std::numeric_limits<float>::infinity())
#endif

#ifndef INV_PI
#define INV_PI  0.31830988618379067154f
#endif

#ifndef INV_TWOPI
#define INV_TWOPI  0.15915494309189533577f
#endif

template<class T> inline T Clamp(T val, T low, T high) {
	return val > low ? (val < high ? val : high) : low;
}

template<class T> inline T Max(T a, T b) {
	return a > b ? a : b;
}

template<class T> inline T Min(T a, T b) {
	return a < b ? a : b;
}

template<class T> inline void Swap(T &a, T &b) {
	const T tmp = a;
	a = b;
	b = tmp;
}

template<class T> inline T Mod(T a, T b) {
	if (b == 0)
		b = 1;

	a %= b;
	if (a < 0)
		a += b;

	return a;
}

inline float Radians(float deg) {
	return (M_PI / 180.f) * deg;
}

inline float Degrees(float rad) {
	return (180.f / M_PI) * rad;
}

inline float Sgn(float a) {
	return a < 0.f ? -1.f : 1.f;
}

inline int Sgn(int a) {
	return a < 0 ? -1 : 1;
}

template<class T> inline int Float2Int(T val) {
	return static_cast<int> (val);
}

template<class T> inline unsigned int Float2UInt(T val) {
	return val >= 0 ? static_cast<unsigned int> (val) : 0;
}

inline int Floor2Int(double val) {
	return static_cast<int> (floor(val));
}

inline int Floor2Int(float val) {
	return static_cast<int> (floorf(val));
}

inline unsigned int Floor2UInt(double val) {
	return val > 0. ? static_cast<unsigned int> (floor(val)) : 0;
}

inline unsigned int Floor2UInt(float val) {
	return val > 0.f ? static_cast<unsigned int> (floorf(val)) : 0;
}

inline int Ceil2Int(double val) {
	return static_cast<int> (ceil(val));
}

inline int Ceil2Int(float val) {
	return static_cast<int> (ceilf(val));
}

inline unsigned int Ceil2UInt(double val) {
	return val > 0. ? static_cast<unsigned int> (ceil(val)) : 0;
}

inline unsigned int Ceil2UInt(float val) {
	return val > 0.f ? static_cast<unsigned int> (ceilf(val)) : 0;
}

template <class T> inline std::string ToString(const T& t) {
	std::stringstream ss;
	ss << t;
	return ss.str();
}

template <class T> inline T RoundUp(const T a, const T b) {
	const unsigned int r = a % b;
	if (r == 0)
		return a;
	else
		return a + b - r;
}

template <class T> inline T RoundUpPow2(T v) {
	v--;

	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;

	return v+1;
}

inline unsigned int UIntLog2(unsigned int value) {
	unsigned int l = 0;
	while (value >>= 1) l++;
	return l;
}

#endif	/* _SFERA_UTILS_H */
