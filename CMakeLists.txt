###########################################################################
#   Copyright (C) 1998-2011 by authors (see AUTHORS.txt )                 #
#                                                                         #
#   This file is part of Sfera.                                           #
#                                                                         #
#   Sfera is free software; you can redistribute it and/or modify         #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 3 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   Sfera is distributed in the hope that it will be useful,              #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  If not, see <http://www.gnu.org/licenses/>. #
#                                                                         #
###########################################################################

MESSAGE(STATUS "CMAKE VERSION DETECTED " ${CMAKE_VERSION})


###########################################################################
#
# check and configure cmake
#
###########################################################################
# Fresh start
cmake_minimum_required(VERSION 2.8)
cmake_policy(VERSION 2.8)

project(Sfera)

# Dont overconfigure
SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limit configs" FORCE)


###########################################################################
#
# Include necessary submodules
#
###########################################################################

set(CMAKE_MODULE_PATH
	"${Sfera_SOURCE_DIR}"
	"${Sfera_SOURCE_DIR}/cmake"
	"${Sfera_SOURCE_DIR}/cmake/Utils"
	"${Sfera_SOURCE_DIR}/cmake/Packages"
	"${Sfera_SOURCE_DIR}/cmake/SpecializedConfig"
)

INCLUDE(PlatformSpecific)
INCLUDE(Configuration)

# Install CMake modules
#add_subdirectory(CMake)

SET(Sfera_INCLUDE_DIR "${Sfera_SOURCE_DIR}/src/include")
include_directories("${Sfera_INCLUDE_DIR}")

# When using single configuration generators like make
# cmake does need to know which of the possible configurations
# to generate
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

message(STATUS "Building mode: " ${CMAKE_BUILD_TYPE})

# Find dependencies
include(Dependencies)

if (NOT OPENGL_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required OpenGL files")
endif()

if (NOT Boost_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required Boost files - Please check ${BOOST_SEARCH_PATH}")
endif()

if (NOT FreeImage_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required FreeImage files - Please check ${FREEIMAGE_SEARCH_PATH}")
endif()

if (NOT GLEW_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required GLEW files - Please check ${GLEW_SEARCH_PATH}")
endif()

if (NOT OPENCL_FOUND AND NOT SFERA_DISABLE_OPENCL)
	MESSAGE(FATAL_ERROR "--> Could not locate required OpenCL files - Please check ${OPENCL_SEARCH_PATH}")
endif()

if (NOT SDL_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required SDL files - Please check ${SDL_SEARCH_PATH}")
endif()

if (NOT SDLTTF_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required SDL TTF files - Please check ${SDLTTF_SEARCH_PATH}")
endif()

if (NOT BULLET_FOUND)
	MESSAGE(FATAL_ERROR "--> Could not locate required Bullet Physics files - Please check ${BULLET_SEARCH_PATH}")
endif()

if (SFERA_DISABLE_OPENCL)
	ADD_DEFINITIONS("-DSFERA_DISABLE_OPENCL")
endif()

###########################################################################
#
# Samples
#
###########################################################################

add_subdirectory(src)


###########################################################################
#
# For non win32 we'll have to copy everything to a single dir
#
###########################################################################

INCLUDE(AssembleBinDirs)
