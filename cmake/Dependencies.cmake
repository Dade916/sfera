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

include(FindPkgMacros)
getenv_path(Sfera_DEPENDENCIES_DIR)

#######################################################################
# Core dependencies
#######################################################################

# Find FreeImage
find_package(FreeImage)

if (FreeImage_FOUND)
  include_directories(${FreeImage_INCLUDE_DIRS})
endif ()

#######################################################################

# Find Boost
set(Boost_USE_STATIC_LIBS       ON)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
set(BOOST_ROOT                  "${BOOST_SEARCH_PATH}")
#set(Boost_DEBUG                 ON)
set(Boost_MINIMUM_VERSION       "1.43.0")

set(Boost_ADDITIONAL_VERSIONS "1.46.2" "1.46.1" "1.46" "1.46.0" "1.45" "1.45.0" "1.44" "1.44.0" "1.43" "1.43.0" )

set(Sfera_BOOST_COMPONENTS thread filesystem system)
find_package(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${Sfera_BOOST_COMPONENTS})
if (NOT Boost_FOUND)
        # Try again with the other type of libs
        if(Boost_USE_STATIC_LIBS)
                set(Boost_USE_STATIC_LIBS)
        else()
                set(Boost_USE_STATIC_LIBS OFF)
        endif()
        find_package(Boost ${Boost_MINIMUM_VERSION} COMPONENTS ${Sfera_BOOST_COMPONENTS})
endif()

if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
endif ()

#######################################################################

find_package(OpenGL)

if (OPENGL_FOUND)
  include_directories(${OPENGL_INCLUDE_PATH})
endif()

#######################################################################

set(GLEW_ROOT                  "${GLEW_SEARCH_PATH}")
if(NOT APPLE)
	find_package(GLEW)
endif()

# GLEW
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_PATH})
endif ()

#######################################################################

set(SDL_ROOT                  "${SDL_SEARCH_PATH}")
find_package(SDL)

# SDL
if (SDL_FOUND)
  include_directories(${SDL_INCLUDE_PATH})
endif ()

set(SDLTTF_ROOT                  "${SDLTTF_SEARCH_PATH}")

find_package(SDL_ttf)

# SDL TTF
if (SDLTTF_FOUND)
  include_directories(${SDLTTF_INCLUDE_PATH})
endif ()

################################################################################

if (NOT SFERA_DISABLE_OPENCL)
  set(OPENCL_ROOT                  "${OPENCL_SEARCH_PATH}")
  find_package(OpenCL)
  # OpenCL
  if (OPENCL_FOUND)
    include_directories(${OPENCL_INCLUDE_PATH})
  endif ()
endif ()

################################################################################

set(BULLET_ROOT                  "${BULLET_SEARCH_PATH}")
find_package(Bullet)
# Bullet Physics
if (BULLET_FOUND)
  include_directories(${BULLET_INCLUDE_DIRS})
endif ()
