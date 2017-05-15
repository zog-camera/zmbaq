project(ZMBCommonProj)

## (disabled)
#add_subdirectory(http)

file(GLOB src_zmb_common *.cpp *.h) 

add_library(zmbminor STATIC ${src_zmb_common})
target_compile_features(ZMBCommon PUBLIC cxx_constexpr)

include_directories(${DEPENDS_ROOT}/include ${DEPENDS_ROOT}/include/jsoncpp)
## for minor/http subdir(disabled)
#target_link_libraries(ZMBCommon -pthread webgrep ${JSON_LIB})

