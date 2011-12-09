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

###########################################################################
#
# Configuration
#
# Use cmake "-DSfera_CUSTOM_CONFIG=YouFileCName" To define your personal settings
# where YouFileCName.cname must exist in one of the cmake include directories
# best to use cmake/SpecializedConfig/
# 
# To not load defaults before loading custom values define
# -DSfera_NO_DEFAULT_CONFIG=true
#
# WARNING: These variables will be cashed like any other
#
###########################################################################

IF (NOT Sfera_NO_DEFAULT_CONFIG)

  # Disable Boost automatic linking
  ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)

	IF (WIN32)

      MESSAGE(STATUS "Using default WIN32 Configuration settings")
			
	  set(ENV{QTDIR} "c:/qt/")
		
	  set(FREEIMAGE_SEARCH_PATH     "${Sfera_SOURCE_DIR}/../FreeImage")
      set(FreeImage_INC_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/source")
	  set(FreeImage_LIB_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/release"
		                                "${FREEIMAGE_SEARCH_PATH}/debug"
		                                "${FREEIMAGE_SEARCH_PATH}/dist")
			                              
	  set(BOOST_SEARCH_PATH         "${Sfera_SOURCE_DIR}/../boost")
	  set(OPENCL_SEARCH_PATH        "${Sfera_SOURCE_DIR}/../opencl")
	  set(GLUT_SEARCH_PATH          "${Sfera_SOURCE_DIR}/../freeglut")
			
	ELSE(WIN32)
			
	
	ENDIF(WIN32)
	
ELSE(NOT Sfera_NO_DEFAULT_CONFIG)
	
	MESSAGE(STATUS "Sfera_NO_DEFAULT_CONFIG defined - not using default configuration values.")

ENDIF(NOT Sfera_NO_DEFAULT_CONFIG)

# Setup libraries output directory
SET (LIBRARY_OUTPUT_PATH
   ${PROJECT_BINARY_DIR}/lib
   CACHE PATH
   "Single Directory for all Libraries"
   )

# Setup binaries output directory
SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY
	${PROJECT_BINARY_DIR}/bin
   CACHE PATH
   "Single Directory for all binaries"
	)

#
# Overwrite defaults with Custom Settings
#
IF (Sfera_CUSTOM_CONFIG)
	MESSAGE(STATUS "Using custom build config: ${Sfera_CUSTOM_CONFIG}")
	INCLUDE(${Sfera_CUSTOM_CONFIG})
ENDIF (Sfera_CUSTOM_CONFIG)

