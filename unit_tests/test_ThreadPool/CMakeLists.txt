cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(TestThreadPool)

file(GLOB test_src *.cpp *.h *.hpp)
include_directories(../.. ../../webgrep)
add_executable(submission_test ${test_src})
if(DO_MEMADDR_SANITIZE)
    add_definitions("-fsanitize=address")
    target_link_libraries(submission_test -lasan)
endif()
target_compile_features(submission_test PUBLIC cxx_constexpr)
target_link_libraries(submission_test webgrep)


