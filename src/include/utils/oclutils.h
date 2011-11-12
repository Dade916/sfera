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

#ifndef _SFERA_OCL_UTILS_H
#define	_SFERA_OCL_UTILS_H

#if !defined(SFERA_DISABLE_OPENCL)

#include <string>
#include <map>

#include "sfera.h"

extern string OCLLocalMemoryTypeString(cl_device_local_mem_type type);
extern string OCLDeviceTypeString(cl_device_type type);
extern string OCLErrorString(cl_int error);

#endif

#endif	/* _SFERA_OCL_UTILS_H */
