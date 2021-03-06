project(ZMBSrcProj)


add_subdirectory(minor)


file(GLOB src_files *.cpp *.h *.hpp)
include_directories(src src/zmbaq_common)
include_directories(${DEPENDS_ROOT}/include)
include_directories(${DEPENDS_ROOT}/include/glm)
include_directories(${DEPENDS_ROOT}/glm )

add_library(zmbsrc STATIC ${src_files})
target_compile_features(zmbsrc PUBLIC cxx_constexpr)

#find_library(JPEG_LIB NAMES jpeg libjpeg.a PATHS ${DEPENDS_ROOT} ${DEPENDS_ROOT}/lib)
set(ADDITIONAL_PATHS ${CMAKE_SOURCE_DIR}/deps/lib ${DEPENDS_ROOT}/lib)
find_library(BOOST_FS NAMES boost_filesystem PATHS ${DEPENDS_ROOT} ${ADDITIONAL_PATHS})
find_library(FF_AVUTIL NAMES avutil PATHS  ${DEPENDS_ROOT} ${ADDITIONAL_PATHS})
find_library(FF_SWSCALE NAMES swscale PATHS  ${DEPENDS_ROOT} ${ADDITIONAL_PATHS})
find_library(FF_AVFORMAT NAMES avformat PATHS  ${DEPENDS_ROOT} ${ADDITIONAL_PATHS})
find_library(FF_AVCODEC NAMES avcodec PATHS  ${DEPENDS_ROOT} ${ADDITIONAL_PATHS})
find_library(FF_AVDEVICE NAMES avdevice PATHS  ${DEPENDS_ROOT} ${ADDITIONAL_PATHS})

set(FF_LIBS ${FF_AVUTIL} ${FF_SWSCALE} ${FF_AVFORMAT} ${FF_AVCODEC} ${FF_AVDEVICE})

target_link_libraries(zmbsrc  ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}
    -pthread zmbminor ${BOOST_FS} ${FF_LIBS})

