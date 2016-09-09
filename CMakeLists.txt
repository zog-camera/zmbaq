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
set(OF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src_foreign_repo/openFrameworks"
         CACHE PATH "Path to openFrameworks build")
set(FFMPEG_DIR "${HOMIAK}/coding/broot_debug" CACHE PATH "Path to FFMpeg install dir")
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

message("FFMPEG_DIR = ${FFMPEG_DIR}")
if(FFMPEG_DIR)
    message("including: ${FFMPEG_DIR}/include")
        include_directories(${FFMPEG_DIR}/include)
        link_directories(${FFMPEG_DIR}/lib)
endif()

if(ZMQ_DIR)
        include_directories(${ZMQ_DIR}/include)
        link_directories(${ZMQ_DIR}/lib)
endif()

find_library(JPEG_LIB NAMES jpeg libjpeg.a
        PATHS ${FFMPEG_DIR} ${FFMPEG_DIR}/lib
#        NO_SYSTEM_ENVIRONMENT_PATH
#        NO_CMAKE_SYSTEM_PATH
#        NO_DEFAULT_PATH
)

find_library(LEVELDB_LIB NAMES leveldb)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# json parser from https://github.com/nlohmann/json
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/src)

find_library(JSON_LIB
        NAMES jsoncpp
        PATH_SUFFIXES shared static
        PATHS ${FFMPEG_DIR} ${FFMPEG_DIR}/lib
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_DEFAULT_PATH)
message(STATUS "libjsoncpp: ${JSON_LIB}")
##########################################3
# libzmbaq_common : common things library
# common cpps
file(GLOB com_src src/zmbaq_common/*.cpp src/zmbaq_common/*.h )

add_library(zmbaq_common STATIC ${com_src})
target_link_libraries(zmbaq_common ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}
-lzip -pthread  ${JSON_LIB}
 -lzmq -lczmq
 -lPocoNet -lPocoNetSSL -lPocoFoundation
# -lboost_system
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/zmbaq_common)

##########################################3
# libzmbaq : core library
file(GLOB src_files
 src/*.cpp src/*.h src/*.hpp
 src/streaming/*.cpp  src/streaming/*.h
# src/detection/*.cpp  src/detection/*.h
src/avcpp/*.cpp src/avcpp/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/src/json.hpp)

add_library(zmbaq ${src_files})
target_link_libraries(zmbaq
 ${ASAN_LINK_FLAGS}
${PROF_LINK_FLAGS}
#${CV_LIBS} ${JPEG_LIB}
#-lv4l2
-lavcodec -lavformat -lavutil -lswscale -pthread -lzip  -lx264
# -lmp4v2
-lzmq -lczmq
zmbaq_common)
##########################################
##########################################
file(GLOB src_lib_ve
 src_videoentity/*.cpp
 src_videoentity/*.h
 src_videoentity/delaunay/*.cpp
 src_videoentity/delaunay/*.h
 )
add_library(videoentity STATIC ${src_lib_ve})
target_link_libraries(videoentity zmbaq -lPocoFoundation)
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

#################################################
## logreader gui:
#file(GLOB files_gui src_logview/*.cpp src_logview/*.h)
#add_executable(logviewer ${files_gui})
#target_link_libraries(logviewer ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}  -ltbb -ltbbmalloc ${JSON_LIB} Qt5::Gui Qt5::Widgets Qt5::Network zmbaq_common)
################################################
################################################

### TESTS ##
## test applications:
#set(TESTS testchan testlogservice test_zmq test_pokerobj test_leech_lair test_cds_map test_generic_read)

#foreach(test_obj ${TESTS})
#        add_executable(${test_obj} src_test/${test_obj}.cpp src_test/${test_obj}.h)
#        add_definitions("-fno-omit-frame-pointer")
#        set(ASAN_LINK_FLAGS "-lasan -fsanitize=address")
#        target_link_libraries(${test_obj} ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS} -ltbb -ltbbmalloc  zmbaq_common )
#endforeach()

##tests with JPEG
#file(GLOB jpeg_files src/jpegenc* src/jpegdec* src/sws* src/jimage*)
#add_library(test_jpeg_cpp STATIC ${jpeg_files})
#target_link_libraries(test_jpeg_cpp ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS} -ltbb -ltbbmalloc  zmbaq_common
# ${JPEG_LIB} Qt5::Gui -lavcodec -lavformat -lavutil -lswscale )

#set(JPEG_TESTS test_udpjpegencoding test_jpegctx )
#foreach(test_obj ${JPEG_TESTS})
#        add_executable(${test_obj} src_test/${test_obj}.cpp src_test/${test_obj}.h )
#        target_link_libraries(${test_obj} ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS} test_jpeg_cpp)
#endforeach()




