project(SrcVideoEntityProj)

set(FFMPEG_DIR ${DEPENDS_ROOT} CACHE PATH "Path to FFMpeg install dir")

message("FFMPEG_DIR = ${FFMPEG_DIR}")

if(DEPENDS_ROOT)
    include_directories(${DEPENDS_ROOT}/include)
    include_directories(${DEPENDS_ROOT}/glm)
    link_directories(${DEPENDS_ROOT}/lib)
endif()

if(NOT NO_V4L)
  set(src_l4v ${v4l2_wrap_files})
endif()

file(GLOB src_lib_ve *.cpp *.h delaunay/*.cpp delaunay/*.h )

include_directories(../external/libv4l2cpp/inc/)

set(FFMPEG_PKG_CONFIG_SUFFIX ${FFMPEG_DIR})
find_library(V4_LIB NAMES v4l2 PATHS ${DEPENDS_ROOT}/lib)
find_library(LOG4_LIB NAMES log4cpp PATHS ${DEPENDS_ROOT}/lib)


include_directories(../external/avcpp/src)
add_library(videoentity STATIC ${src_lib_ve} ${src_l4v})


target_link_libraries(videoentity zmbsrc
  ${FFMPEG_LIBRARIES} ${LOG4_LIB}
  ${V4_LIB} ${AV_TARGET} )
