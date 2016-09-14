project(ZMBCommonProj)

set(JSON_PATH "" CACHE PATH "libjsoncpp search path (it should have lib/ include/ subdirs)")

add_subdirectory(http)


file(GLOB src_zmb_common *.cpp *.h) 

add_library(ZMBCommon SHARED ${src_zmb_common})
target_compile_features(ZMBCommon PUBLIC cxx_constexpr)



find_library(JSON_LIB
        NAMES jsoncpp
        PATH_SUFFIXES shared static
        PATHS ${JSON_PATH} ${JSON_PATH}/lib
        NO_SYSTEM_ENVIRONMENT_PATH
        NO_CMAKE_SYSTEM_PATH
        NO_DEFAULT_PATH)
include_directories(${FFMPEG_DIR}/include/jsoncpp)
message(STATUS "libjsoncpp: ${JSON_LIB}")


target_link_libraries(ZMBCommon -pthread webgrep ${JSON_LIB})

