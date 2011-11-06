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

#ifndef _SFERA_H
#define	_SFERA_H

#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstddef>

#include <FreeImage.h>
#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#include <SDL/SDL_opengl.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__CYGWIN__)
#include <stddef.h>
#include <sys/time.h>
#elif defined (WIN32)
#include <windows.h>
#else
	Unsupported Platform !!!
#endif

#include "sferacfg.h"

#if !defined(SFERA_DISABLE_OPENCL)
#define __CL_ENABLE_EXCEPTIONS

#if defined(__APPLE__)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif
#endif

#include "geometry/vector.h"
#include "geometry/point.h"
#include "geometry/normal.h"
#include "geometry/ray.h"
#include "geometry/matrix4x4.h"
#include "geometry/transform.h"
#include "utils/utils.h"
#include "utils/oclutils.h"
#include "utils/properties.h"

using namespace std;

extern void SferaDebugHandler(const char *msg);

#define SFERA_LOG(a) { std::stringstream _SFERA_LOG_LOCAL_SS; _SFERA_LOG_LOCAL_SS << a; SferaDebugHandler(_SFERA_LOG_LOCAL_SS.str().c_str()); }

#endif	/* _SFERA_H */
