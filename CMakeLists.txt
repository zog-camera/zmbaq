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

set(URHO3D_DIR "${FFMPEG_DIR}" CACHE PATH "Path to Urho3D install dir")
set(ZMQ_DIR "${FFMPEG_DIR}" CACHE PATH "Path to ZeroMq 4.x version")
set(OPENCV_DIR "${FFMPEG_DIR}" CACHE PATH "OpenCV >= 3.0 installation path")
set(CMAKE_CXX_FLAGS "-std=c++11 -msse2")

add_definitions("-D__STDC_CONSTANT_MACROS -DBOOST_LOG_DYN_LINK")

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

if(URHO3D_DIR)
    include_directories(${URHO3D_DIR}/include/Urho3D/ThirdParty ${URHO3D_DIR}/include/Urho3D/ThirdParty/SDL)

endif()

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)


##########################################3
# libzmbsrc: common things library
# common cpps
add_subdirectory(src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/zmbaq_common)

##########################################
# libvideoentity
file(GLOB src_lib_ve
 src_videoentity/*.cpp
 src_videoentity/*.h
 src_videoentity/delaunay/*.cpp
 src_videoentity/delaunay/*.h
 )
add_library(videoentity STATIC ${src_lib_ve})
target_link_libraries(videoentity zmbsrc -lPocoFoundation)
##########################################
##########################################

# RTSP dumper:
file(GLOB_RECURSE files_rtsp_dumper src_rtsp_dumper/*.cpp src_rtsp_dumper/*.h)
add_executable(rtsp_dumper ${files_rtsp_dumper})
target_link_libraries(rtsp_dumper videoentity -lPocoFoundation)
##########################################
##########################################

if (WITH_GUI)
  find_library(u3dlib NAMES Urho3D
      PATHS
      ${URHO3D_DIR}/lib/Urho3D       ${URHO3D_DIR}/lib64/Urho3D
      ${URHO3D_DIR}/lib64 ${URHO3D_DIR}/lib64/Urho3D/
      ${FFMPEG_DIR}/lib)
  message("u3d lib: ${u3dlib} dir: ${URHO3D_DIR}")

  file(GLOB_RECURSE files_gui src_gui_urho3d/*.cpp src_gui_urho3d/*.h)
  add_executable(zmbaq_gui ${files_gui} ${RC_OUT})
  target_link_libraries(zmbaq_gui ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}
   ${u3dlib} -ldl -lGL -lGLEW
   videoentity
  )

endif()

### TESTS ##
## test applications:
add_subdirectory(unit_tests/test_ThreadPool)



