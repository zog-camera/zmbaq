# CMakeLists.txt for the ZoneMinder binaries
cmake_minimum_required (VERSION 2.8.10)
project (zmbaq_proj)
set(zmbaq_VERSION "0.0.1")

option(WITH_PROFILER "Enable gperftools" OFF)
option(WITH_ADDRESS_SANITIZER "Enable memory access sanitizer." OFF)
option(WITH_DEBUG "Enable debug symbols." ON)
option(WITH_GUI "Build the GUI" ON)
option(NO_V4L "Disable Video4Linux support" OFF)

set(HOMIAK "$ENV{HOME}")

set(DEPENDS_ROOT "$ENV{BROOT}" CACHE PATH "Dependency installation directory.")
set(URHO3D_DIR "${DEPENDS_ROOT}" CACHE PATH "Path to Urho3D install dir")
set(ZMQ_DIR "${DEPENDS_ROOT}" CACHE PATH "Path to ZeroMq 4.x version")
set(OPENCV_DIR "${DEPENDS_ROOT}" CACHE PATH "OpenCV >= 3.0 installation path")


set(CMAKE_CXX_FLAGS "-std=c++1y -msse2")

add_definitions("-D__STDC_CONSTANT_MACROS -DBOOST_LOG_DYN_LINK")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)
include_directories(${DEPENDS_ROOT}/include)
include_directories(${DEPENDS_ROOT}/include/jsoncpp)
include_directories(${DEPENDS_ROOT}/include/glm)
include_directories(${DEPENDS_ROOT}/glm)
link_directories(${DEPENDS_ROOT}/lib)

if(NO_V4L)
       add_definitions("-DZMBAQ_NOV4L=1")
endif()

SET(CV_LIBS "-lopencv_core -lopencv_imgproc -lopencv_video")

SET(ASAN_LINK_FLAGS)

if(WITH_ADDRESS_SANITIZER)
        add_definitions("-fno-omit-frame-pointer")
        set(ASAN_LINK_FLAGS "-lasan -fsanitize=address")
endif()

if(WITH_DEBUG)
    set(CMAKE_BUILD_TYPE "debug")
endif()

SET(PROF_LINK_FLAGS)
if(WITH_PROFILER)
        add_definitions("-DGPERFTOOLS_ENABLED=1")
        set(PROF_LINK_FLAGS "-lprofiler")
endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)


##########################################3
# libzmbsrc: common things library
# common cpps
add_subdirectory(src)

# libvideoentity
add_subdirectory(src_videoentity)

##########################################

if (WITH_GUI)
    add_subdirectory(src_gui_urho3d)
endif()

### TESTS ##
## test applications:
#add_subdirectory(unit_tests/test_ThreadPool)
#add_subdirectory(unit_tests/test_videoentity)



