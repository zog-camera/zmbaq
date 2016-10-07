include_directories(./avcpp/src)

file(GLOB_RECURSE AV_SOURCES "avcpp/src/*.cpp")
file(GLOB_RECURSE AV_HEADERS "avcpp/src/*.h" "avcpp/src/*.hpp")

set(AV_TARGET avcpp)

link_directories(${FFMPEG_LIBRARY_DIRS})
message(STATUS "FFMPEG library dir: " ${FFMPEG_LIBRARY_DIRS})

add_library(${AV_TARGET}        SHARED ${AV_SOURCES})
add_library(${AV_TARGET}_static STATIC ${AV_SOURCES})

set_target_properties(${AV_TARGET}_static PROPERTIES OUTPUT_NAME ${AV_TARGET})
find_package(Threads)

target_link_libraries(${AV_TARGET}
    ${CMAKE_THREAD_LIBS_INIT}
    ${FFMPEG_LIBRARIES}
)

target_link_libraries(${AV_TARGET}_static
    ${CMAKE_THREAD_LIBS_INIT}
    ${FFMPEG_LIBRARIES}
)

if(WIN32)
  target_link_libraries(${AV_TARGET} ws2_32)
  target_link_libraries(${AV_TARGET}_static ws2_32)
endif()
