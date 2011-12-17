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
  "${CMAKE_CURRENT_SOURCE_DIR}/include/sferacfg.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/sferacfg.h"
  )


###########################################################################
#
# Generate kernel files
#
###########################################################################

SET(KERNELS kernel_core)
FOREACH(kernel ${KERNELS})
	STRING(TOLOWER ${kernel} kernel_lowercase)

	IF(WIN32)
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo "#if !defined(SFERA_DISABLE_OPENCL)" > ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo "#include \"renderer/ocl/kernels/kernels.h\"" >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo string KernelSource_${kernel} = >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND for /F \"tokens=*\" %%a in (${CMAKE_CURRENT_SOURCE_DIR}\\renderer\\ocl\\kernels\\${kernel_lowercase}.cl) do echo \"%%a\\n\" >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo ";" >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo "#endif" >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cl
		)
	ELSE(WIN32)
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo \"\#if !defined(SFERA_DISABLE_OPENCL)\" > ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo \"\#include \\"renderer/ocl/kernels/kernels.h\\"\" >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo "string KernelSource_${kernel} = " >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND cat renderer/ocl/kernels/${kernel_lowercase}.cl | awk '{ printf \(\"\\"%s\\\\n\\"\\n\", $$0\) }' >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo \"\;\" >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			COMMAND echo \"\#endif\" >> ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cpp
			MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/renderer/ocl/kernels/${kernel_lowercase}.cl
		)
	ENDIF(WIN32)
ENDFOREACH(kernel)


#############################################################################
#
# Sfera binary
#
#############################################################################

set(Sfera_SRCS
	acceleretor/bvhaccel.cpp
	displaysession.cpp
	epsilon.cpp
	gameconfig.cpp
	gamelevel.cpp
	gameplayer.cpp
	gamesession.cpp
	geometry/matrix4x4.cpp
	geometry/transform.cpp
	geometry/sphere.cpp
	pixel/framebuffer.cpp
	pixel/tonemap.cpp
	renderer/cpu/cpurenderer.cpp
	renderer/cpu/singlecpurenderer.cpp
	renderer/cpu/multicpurenderer.cpp
	renderer/ocl/compiledscene.cpp
	renderer/ocl/kernels/kernel_core.cpp
	renderer/ocl/oclrenderer.cpp
	physic/gamephysic.cpp
	sdl/camera.cpp
	sdl/light.cpp
	sdl/material.cpp
	sdl/scene.cpp
	sdl/texmap.cpp
	sfera.cpp
	utils/oclutils.cpp
	utils/packhighscore.cpp
	utils/packlist.cpp
	utils/packlevellist.cpp
	utils/rendertext.cpp
	utils/properties.cpp
	)

add_executable(Sfera ${Sfera_SRCS})

TARGET_LINK_LIBRARIES(Sfera ${GLEW_LIBRARY} ${OPENGL_LIBRARY} ${SDL_LIBRARY} ${SDLTTF_LIBRARY} ${Boost_LIBRARIES} ${BULLET_LIBRARIES})
TARGET_LINK_LIBRARIES(Sfera debug ${FreeImage_LIBRARY_DBG} optimized ${FreeImage_LIBRARY_REL} general ${FreeImage_LIBRARY_REL})
if (NOT SFERA_DISABLE_OPENCL)
	TARGET_LINK_LIBRARIES(Sfera ${OPENCL_LIBRARY})
endif()
