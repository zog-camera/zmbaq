project(SrcVideoEntityProj)

set(FFMPEG_DIR "${DEPENDS_ROOT}"
    CACHE PATH "Path to FFMpeg install dir")

message("FFMPEG_DIR = ${FFMPEG_DIR}")
if(DEPENDS_ROOT)
  include_directories(${DEPENDS_ROOT}/include ${DEPENDS_ROOT}/glm)
  link_directories(${DEPENDS_ROOT}/lib)
endif()

file(GLOB src_lib_ve *.cpp *.h delaunay/*.cpp delaunay/*.h )
file(GLOB src_l4v  ../external/libv4l2cpp/src/*.cpp ../external/libv4l2cpp/inc/*.h )
include_directories(../external/libv4l2cpp/inc/)

set(FFMPEG_PKG_CONFIG_SUFFIX ${FFMPEG_DIR})
find_library(V4_LIB NAMES v4l2 PATHS ${DEPENDS_ROOT}/lib)
find_library(LOG4_LIB NAMES log4cpp PATHS ${DEPENDS_ROOT}/lib)
find_library(AVFILTER_LIB NAMES avfilter PATHS ${DEPENDS_ROOT}/lib)
find_library(SWSCALE_LIB NAMES swscale PATHS ${DEPENDS_ROOT}/lib)
find_library(AVDEVICE_LIB NAMES avdevice PATHS ${DEPENDS_ROOT}/lib)
find_library(AVCODEC_LIB NAMES avcodec PATHS ${DEPENDS_ROOT}/lib)
find_library(AVFORMAT_LIB NAMES avformat PATHS ${DEPENDS_ROOT}/lib)
find_library(AVUTIL_LIB NAMES avutil PATHS ${DEPENDS_ROOT}/lib)
find_library(PocoFoundation_LIB NAMES PocoFoundation PATHS ${DEPENDS_ROOT}/lib)

set(FFMPEG_LIBRARY_DIRS ${FFMPEG_DIR}/lib)
set(FFMPEG_LIBRARIES ${AVCODEC_LIB} ${AVFORMAT_LIB} ${AVUTIL_LIB} ${AVFILTER_LIB} ${AVDEVICE_LIB} ${SWSCALE_LIB})

add_subdirectory(../external ${CMAKE_BINARY_DIR}/avcpp)
add_library(videoentity STATIC ${src_lib_ve} ${src_l4v})


target_link_libraries(videoentity zmbsrc ${PocoFoundation_LIB}
  ${FFMPEG_LIBRARIES} #${LOG4_LIB}
  ${V4_LIB} avcpp_static)
