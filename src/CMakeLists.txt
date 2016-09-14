project(ZMBSrcProj)

set(FFMPEG_DIR "${HOMIAK}/coding/broot_debug" CACHE PATH "Path to FFMpeg install dir")

add_subdirectory(zmbaq_common)


file(GLOB src_files *.cpp *.h *.hpp streaming/*.cpp  streaming/*.h vcpp/*.cpp avcpp/*.h)

add_library(zmbsrc STATIC ${src_files})
target_compile_features(zmbsrc PUBLIC cxx_constexpr)

message("FFMPEG_DIR = ${FFMPEG_DIR}")
if(FFMPEG_DIR)
  include_directories(${FFMPEG_DIR}/include ${FFMPEG_DIR}/glm)
  link_directories(${FFMPEG_DIR}/lib)
endif()
find_library(JPEG_LIB NAMES jpeg libjpeg.a PATHS ${FFMPEG_DIR} ${FFMPEG_DIR}/lib)




target_link_libraries(zmbsrc  ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS} -pthread ZMBCommon ${JPEG_LIB})

