cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(TestVideoEntity)

set(CDIR ../../src_videoentity)
include_directories(${CDIR})
file(GLOB src_test *.cpp *.h)


add_executable(test_videoentity ${src_test})
if(DO_MEMADDR_SANITIZE)
    add_definitions("-fsanitize=address")
    target_link_libraries(submission_test -lasan)
endif()
target_compile_features(test_videoentity PUBLIC cxx_constexpr)
target_link_libraries(test_videoentity videoentity)



