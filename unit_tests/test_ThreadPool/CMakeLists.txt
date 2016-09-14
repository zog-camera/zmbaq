cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(TestThreadPool)

set(CDIR ../../src/zmb_common)
file(GLOB test_src *.cpp *.h *.hpp ${CDIR}/thread_pool* ${CDIR}/linked_task* ${CDIR}/zmb_str* )
include_directories(${CDIR})

add_executable(submission_test ${test_src})
if(DO_MEMADDR_SANITIZE)
    add_definitions("-fsanitize=address")
    target_link_libraries(submission_test -lasan)
endif()
target_compile_features(submission_test PUBLIC cxx_constexpr)
target_link_libraries(submission_test webgrep)


