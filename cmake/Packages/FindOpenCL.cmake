# Try to find OpenCL library and include path.
# Once done this will define
#
# OPENCL_FOUND
# OPENCL_INCLUDE_PATH
# OPENCL_LIBRARY
# 

IF (WIN32)
	FIND_PATH( OPENCL_INCLUDE_PATH CL/cl.h
		${OPENCL_ROOT}/include
		${Sfera_SOURCE_DIR}/../opencl
		DOC "The directory where CL/cl.h resides")
	FIND_LIBRARY( OPENCL_LIBRARY
		NAMES OpenCL.lib
		PATHS
		${OPENCL_LIBRARYDIR}
		${OPENCL_ROOT}/lib/
		${Sfera_SOURCE_DIR}/../opencl/
		DOC "The OpenCL library")
ELSE (WIN32)
	FIND_PATH( OPENCL_INCLUDE_PATH CL/cl.h
		/usr/include
		/usr/local/include
		/sw/include
		/opt/local/include
		${OPENCL_ROOT}/include
		/usr/src/opencl-sdk/include
		DOC "The directory where CL/cl.h resides")
	FIND_LIBRARY( OPENCL_LIBRARY
		NAMES opencl OpenCL
		PATHS
		/usr/lib64
		/usr/lib
		/usr/local/lib64
		/usr/local/lib
		/sw/lib
		/opt/local/lib
		/usr/src/opencl-sdk/lib/x86_64
		${OPENCL_LIBRARYDIR}
		DOC "The OpenCL library")
ENDIF (WIN32)

IF (OPENCL_INCLUDE_PATH)
	SET( OPENCL_FOUND 1 CACHE STRING "Set to 1 if OpenCL is found, 0 otherwise")
ELSE (OPENCL_INCLUDE_PATH)
	SET( OPENCL_FOUND 0 CACHE STRING "Set to 1 if OpenCL is found, 0 otherwise")
ENDIF (OPENCL_INCLUDE_PATH)

MARK_AS_ADVANCED( OPENCL_FOUND )