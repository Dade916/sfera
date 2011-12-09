
###########################################################################
#
# Configuration
#
###########################################################################

# Sfera_CUSTOM_CONFIG
# C:/Users/David/Dati/OpenCL/Sfera-32bit/sfera/cmake/SpecializedConfig/Config_Dade-win32.cmake
# 
# Boost:
#  "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\vcvars64.bat"
#  bjam --toolset=msvc address-model=64 --stagedir=stage stage -j 8


MESSAGE(STATUS "Using Dade's Win32 Configuration settings")

set(CMAKE_BUILD_TYPE "Release")

set(FREEIMAGE_SEARCH_PATH "C:/Users/David/Dati/OpenCL/Sfera-32bit/FreeImage")
set(FreeImage_INC_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/Dist")
set(FreeImage_LIB_SEARCH_PATH "${FREEIMAGE_SEARCH_PATH}/Dist")

set(BOOST_SEARCH_PATH         "C:/Users/David/Dati/OpenCL/Sfera-32bit/boost_1_43_0")
set(BOOST_LIBRARYDIR          "${BOOST_SEARCH_PATH}/stage/boost/lib")

set(OPENCL_SEARCH_PATH        "C:/Program Files (x86)/AMD APP")
set(OPENCL_LIBRARYDIR         "${OPENCL_SEARCH_PATH}/lib/x86")

set(GLEW_SEARCH_PATH          "C:/Users/David/Dati/OpenCL/Sfera-32bit/glew-1.6.0")
ADD_DEFINITIONS(-DGLEW_BUILD)

# TODO: Incomplete
