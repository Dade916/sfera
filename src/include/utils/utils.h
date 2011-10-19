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

#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__)
#include <stddef.h>
#include <sys/time.h>
#elif defined (WIN32)
#include <windows.h>
#else
#error "Unsupported Platform !!!"
#endif

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

inline double WallClockTime() {
#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__)
	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec + t.tv_usec / 1000000.0;
#elif defined (WIN32)
	return GetTickCount() / 1000.0;
#else
#error "Unsupported Platform !!!"
#endif
}

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

inline void StringTrim(std::string &str) {
	std::string::size_type pos = str.find_last_not_of(' ');
	if (pos != std::string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(' ');
		if (pos != std::string::npos) str.erase(0, pos);
	} else str.erase(str.begin(), str.end());
}

inline bool SetThreadRRPriority(boost::thread *thread, int pri = 0) {
#if defined (__linux__) || defined (__APPLE__) || defined(__CYGWIN__)
	{
		const pthread_t tid = (pthread_t)thread->native_handle();

		int policy = SCHED_FIFO;
		int sysMinPriority = sched_get_priority_min(policy);
		struct sched_param param;
		param.sched_priority = sysMinPriority + pri;

		return pthread_setschedparam(tid, policy, &param);
	}
#elif defined (WIN32)
	{
		const HANDLE tid = (HANDLE)thread->native_handle();
		if (!SetPriorityClass(tid, HIGH_PRIORITY_CLASS))
			return false;
		else
			return true;

		/*if (!SetThreadPriority(tid, THREAD_PRIORITY_HIGHEST))
			return false;
		else
			return true;*/
	}
#endif
	}

//------------------------------------------------------------------------------
// Memory
//------------------------------------------------------------------------------

#ifndef L1_CACHE_LINE_SIZE
#define L1_CACHE_LINE_SIZE 64
#endif

template<class T> inline T *AllocAligned(size_t size, std::size_t N = L1_CACHE_LINE_SIZE) {
#if defined(WIN32) && !defined(__CYGWIN__) // NOBOOK
	return static_cast<T *> (_aligned_malloc(size * sizeof (T), N));
#else // NOBOOK
	return static_cast<T *> (memalign(N, size * sizeof (T)));
#endif // NOBOOK
}

template<class T> inline void FreeAligned(T *ptr) {
#if defined(WIN32) && !defined(__CYGWIN__) // NOBOOK
	_aligned_free(ptr);
#else // NOBOOK
	free(ptr);
#endif // NOBOOK
}

template <typename T, std::size_t N = 16 > class AlignedAllocator {
public:
	typedef T value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

	typedef T *pointer;
	typedef const T *const_pointer;

	typedef T &reference;
	typedef const T &const_reference;

public:

	inline AlignedAllocator() throw () {
	}

	template <typename T2> inline AlignedAllocator(const AlignedAllocator<T2, N> &) throw () {
	}

	inline ~AlignedAllocator() throw () {
	}

	inline pointer adress(reference r) {
		return &r;
	}

	inline const_pointer adress(const_reference r) const {
		return &r;
	}

	inline pointer allocate(size_type n) {
		return AllocAligned<value_type > (n, N);
	}

	inline void deallocate(pointer p, size_type) {
		FreeAligned(p);
	}

	inline void construct(pointer p, const value_type &wert) {
		new (p) value_type(wert);
	}

	inline void destroy(pointer p) {
		p->~value_type();
	}

	inline size_type max_size() const throw () {
		return size_type(-1) / sizeof (value_type);
	}

	template <typename T2> struct rebind {
		typedef AlignedAllocator<T2, N> other;
	};
};

#define P_CLASS_ATTR __attribute__
#define P_CLASS_ATTR __attribute__

#if defined(WIN32) && !defined(__CYGWIN__) // NOBOOK

class __declspec(align(16)) Aligned16 {
#else // NOBOOK

class Aligned16 {
#endif // NOBOOK
public:

	/*
	Aligned16(){
		if(((int)this & 15) != 0){
			printf("bad alloc\n");
			assert(0);
		}
	}
	 */

	void *operator new(size_t s) {
		return AllocAligned<char>(s, 16);
	}

	void *operator new (size_t s, void *q) {
		return q;
	}

	void operator delete(void *p) {
		FreeAligned(p);
	}
#if defined(WIN32) && !defined(__CYGWIN__) // NOBOOK
};
#else // NOBOOK
} __attribute__((aligned(16)));
#endif // NOBOOK

#endif	/* _SFERA_UTILS_H */
