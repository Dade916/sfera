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

set(SFERA_VERSION_MAJOR 1)
set(SFERA_VERSION_MINOR 0devel1)

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/sferacfg.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/sferacfg.h"
  )


###########################################################################
#
# Generate kernel files
#
###########################################################################


#############################################################################
#
# Sfera binary
#
#############################################################################

include_directories(${Sfera_INCLUDE_DIR})
include_directories(".")
link_directories (${Sfera_LIB_DIR})

set(Sfera_SRCS
	sfera.cpp
	)

add_executable(Sfera ${Sfera_SRCS})

TARGET_LINK_LIBRARIES(Sfera ${GLEW_LIBRARY} ${OPENGL_LIBRARY})
TARGET_LINK_LIBRARIES(Sfera ${OpenCL_LIBRARY})
TARGET_LINK_LIBRARIES(Sfera ${GLUT_LIBRARY})

# This instructs FREEGLUT to emit a pragma for the static version
SET_TARGET_PROPERTIES(Sfera PROPERTIES COMPILE_DEFINITIONS FREEGLUT_STATIC)
