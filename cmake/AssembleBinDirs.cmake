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
# Binary samples directory
#
###########################################################################

IF (WIN32)
	
  # For MSVC moving exe files gets done automatically
  # If there is someone compiling on windows and
  # not using msvc (express is free) - feel free to implement

ELSE (WIN32)
	set(CMAKE_INSTALL_PREFIX .)

	# Windows 32bit
	set(SFERA_BIN_WIN32_DIR "sfera-win32-v1.0devel1")
	add_custom_command(
		OUTPUT "${SFERA_BIN_WIN32_DIR}"
		COMMAND rm -rf ${SFERA_BIN_WIN32_DIR}
		COMMAND mkdir ${SFERA_BIN_WIN32_DIR}
		COMMAND cp -r gamedata ${SFERA_BIN_WIN32_DIR}
		COMMAND rm -rf ${SFERA_BIN_WIN32_NOOPENCL_DIR}/gamedata/scores/*
		COMMAND cp bin/win32/Sfera.exe bin/win32/*.dll bin/win32/*.bat ${SFERA_BIN_WIN32_DIR}
		COMMAND cp AUTHORS.txt COPYING.txt README.txt ${SFERA_BIN_WIN32_DIR}
		COMMENT "Building ${SFERA_BIN_WIN32_DIR}")

	add_custom_command(
		OUTPUT "${SFERA_BIN_WIN32_DIR}.zip"
		COMMAND zip -r ${SFERA_BIN_WIN32_DIR}.zip ${SFERA_BIN_WIN32_DIR}
		COMMAND rm -rf ${SFERA_BIN_WIN32_DIR}
		DEPENDS ${SFERA_BIN_WIN32_DIR}
		COMMENT "Building ${SFERA_BIN_WIN32_DIR}.zip")

	add_custom_target(sfera_win32_zip DEPENDS "${SFERA_BIN_WIN32_DIR}.zip")

	# Windows 32bit (No OpenCL)
	set(SFERA_BIN_WIN32_NOOPENCL_DIR "sfera-win32-noopencl-v1.0devel1")
	add_custom_command(
		OUTPUT "${SFERA_BIN_WIN32_NOOPENCL_DIR}"
		COMMAND rm -rf ${SFERA_BIN_WIN32_NOOPENCL_DIR}
		COMMAND mkdir ${SFERA_BIN_WIN32_NOOPENCL_DIR}
		COMMAND cp -r gamedata ${SFERA_BIN_WIN32_NOOPENCL_DIR}
		COMMAND rm -rf ${SFERA_BIN_WIN32_NOOPENCL_DIR}/gamedata/scores/*
		COMMAND cp bin/win32/Sfera-noopencl.exe ${SFERA_BIN_WIN32_NOOPENCL_DIR}/Sfera.exe
		COMMAND cp bin/win32/*.dll bin/win32/*.bat ${SFERA_BIN_WIN32_NOOPENCL_DIR}
		COMMAND cp gamedata/cfgs/cpu.cfg ${SFERA_BIN_WIN32_NOOPENCL_DIR}/gamedata/cfgs/default.cfg
		COMMAND cp AUTHORS.txt COPYING.txt README.txt ${SFERA_BIN_WIN32_NOOPENCL_DIR}
		COMMENT "Building ${SFERA_BIN_WIN32_NOOPENCL_DIR}")

	add_custom_command(
		OUTPUT "${SFERA_BIN_WIN32_NOOPENCL_DIR}.zip"
		COMMAND zip -r ${SFERA_BIN_WIN32_NOOPENCL_DIR}.zip ${SFERA_BIN_WIN32_NOOPENCL_DIR}
		COMMAND rm -rf ${SFERA_BIN_WIN32_NOOPENCL_DIR}
		DEPENDS ${SFERA_BIN_WIN32_NOOPENCL_DIR}
		COMMENT "Building ${SFERA_BIN_WIN32_NOOPENCL_DIR}.zip")

	add_custom_target(sfera_win32_noopencl_zip DEPENDS "${SFERA_BIN_WIN32_NOOPENCL_DIR}.zip")

	# Windows 64bit

	# Linux 64bit
	set(SFERA_BIN_LINUX64_DIR "sfera-linux64-v1.0devel1")
	add_custom_command(
		OUTPUT "${SFERA_BIN_LINUX64_DIR}"
		COMMAND rm -rf ${SFERA_BIN_LINUX64_DIR}
		COMMAND mkdir ${SFERA_BIN_LINUX64_DIR}
		COMMAND cp -r gamedata ${SFERA_BIN_LINUX64_DIR}
		COMMAND rm -rf ${SFERA_BIN_WIN32_NOOPENCL_DIR}/gamedata/scores/*
		COMMAND cp bin/Sfera ${SFERA_BIN_LINUX64_DIR}
		COMMAND cp AUTHORS.txt COPYING.txt README.txt ${SFERA_BIN_LINUX64_DIR}
		COMMENT "Building ${SFERA_BIN_LINUX64_DIR}")

	add_custom_command(
		OUTPUT "${SFERA_BIN_LINUX64_DIR}.zip"
		COMMAND zip -r ${SFERA_BIN_LINUX64_DIR}.zip ${SFERA_BIN_LINUX64_DIR}
		COMMAND rm -rf ${SFERA_BIN_LINUX64_DIR}
		DEPENDS ${SFERA_BIN_LINUX64_DIR}
		COMMENT "Building ${SFERA_BIN_LINUX64_DIR}.zip")

	add_custom_target(sfera_linux64_zip DEPENDS "${SFERA_BIN_LINUX64_DIR}.zip")
ENDIF(WIN32)
