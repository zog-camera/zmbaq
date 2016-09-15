project(ZMBSrcProj)


add_subdirectory(zmbaq_common)


file(GLOB src_files *.cpp *.h *.hpp streaming/*.cpp  streaming/*.h avcpp/*.cpp avcpp/*.h)
include_directories(src src/zmbaq_common)

add_library(zmbsrc STATIC ${src_files})
target_compile_features(zmbsrc PUBLIC cxx_constexpr)

find_library(JPEG_LIB NAMES jpeg libjpeg.a PATHS ${DEPENDS_ROOT} ${DEPENDS_ROOT}/lib)


target_link_libraries(zmbsrc  ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}
    -pthread ZMBCommon ${JPEG_LIB})

