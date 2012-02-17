
###########################################################################
#
# Configuration
#
###########################################################################

#cmake -DSfera_CUSTOM_CONFIG=cmake/SpecializedConfig/Config_Dade.cmake .
#cmake -DSfera_CUSTOM_CONFIG=cmake/SpecializedConfig/Config_Dade.cmake -DSFERA_DISABLE_OPENCL=1 .

MESSAGE(STATUS "Using Dade's Linux Configuration settings")

#set(BOOST_SEARCH_PATH         "/home/david/projects/luxrender-dev/boost_1_43_0")

set(OPENCL_SEARCH_PATH        "$ENV{ATISTREAMSDKROOT}")
set(OPENCL_INCLUDEPATH         "${OPENCL_SEARCH_PATH}/include")
#set(OPENCL_LIBRARYDIR         "${OPENCL_SEARCH_PATH}/lib/x86_64")

set(BULLET_SEARCH_PATH         "/home/david/src/bullet-2.79-bin")

#set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_BUILD_TYPE "Release")
