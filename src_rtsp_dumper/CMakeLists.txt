project(rtsp_dumper)

file(GLOB_RECURSE files_dumper *.cpp *.h )
add_executable(rtsp_dumper ${files_dumper})

if(DEPENDS_ROOT)
  include_directories(${DEPENDS_ROOT}/include)
  link_directories(${DEPENDS_ROOT}/lib)
endif()

find_library(POCO_FOUNDATION NAMES PocoFoundation PATHS ${DEPENDS_ROOT}/lib)
find_library(POCO_UTIL NAMES PocoUtil PATHS ${DEPENDS_ROOT}/lib)
find_library(POCO_XML NAMES PocoXML PATHS ${DEPENDS_ROOT}/lib)

target_link_libraries(rtsp_dumper videoentity ${POCO_FOUNDATION} -lPocoUtil -lPocoJSON -lPocoXML -ljsoncpp)
