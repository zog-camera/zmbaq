project(rtsp_dumper)

file(GLOB_RECURSE files_dumper *.cpp *.h )
add_executable(rtsp_dumper ${files_dumper})

target_link_libraries(rtsp_dumper videoentity)
