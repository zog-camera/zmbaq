project(ZMBSrcProj)


add_subdirectory(minor)


file(GLOB src_files *.cpp *.h *.hpp)
include_directories(src src/zmbaq_common)


add_library(zmbsrc STATIC ${src_files})
target_compile_features(zmbsrc PUBLIC cxx_constexpr)

#find_library(JPEG_LIB NAMES jpeg libjpeg.a PATHS ${DEPENDS_ROOT} ${DEPENDS_ROOT}/lib)
set(ADDITIONAL_PATHS ${CMAKE_SOURCE_DIR}/deps/lib ${DEPENDS_ROOT}/lib)
find_library(BOOST_FS NAMES boost_filesystem PATHS ${DEPENDS_ROOT} ${ADDITIONAL_PATHS})

target_link_libraries(zmbsrc  ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}
    -pthread zmbminor ${BOOST_FS} ${FFMPEG_LIBRARIES})


add_executable(zmbdaemon main_stub.cpp)
target_link_libraries(zmbdaemon zmbsrc)
