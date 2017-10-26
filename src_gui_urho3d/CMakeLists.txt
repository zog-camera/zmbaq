project(zmbaq_gui_proj)

if(NOT URHO3D_DIR)
    set(URHO3D_DIR ${DEPENDS_ROOT})
endif(NOT URHO3D_DIR)

include_directories(${URHO3D_DIR}/include/)
include_directories(${URHO3D_DIR}/include/Urho3D/ThirdParty)
INCLUDE_DIRECTORIES(${URHO3D_DIR}/include/Urho3D/ThirdParty/SDL)
link_directories(${DEPENDS_ROOT}/lib ${DEPENDS_ROOT}/lib64 ${DEPENDS_ROOT}/lib64/Urho3D)

find_library(u3dlib NAMES libUrho3D.a Urho3D 
		PATHS
 ${CMAKE_SOURCE_DIR}/deps/lib ${CMAKE_SOURCE_DIR}/deps/lib/Urho3D

 ${DEPENDS_ROOT}/lib64/Urho3D ${DEPENDS_ROOT}/lib ${DEPENDS_ROOT}/lib/Urho3D
    ${URHO3D_DIR}/lib/Urho3D       ${URHO3D_DIR}/lib64/Urho3D
    ${URHO3D_DIR}/lib64 ${URHO3D_DIR}/lib64/Urho3D/
    ${FFMPEG_DIR}/lib)
message("u3d lib: ${u3dlib} dir: ${URHO3D_DIR}")

file(GLOB files_gui Sample.cpp app.cpp app.h )
add_executable(zmbaq_gui ${files_gui} ${RC_OUT})

target_link_libraries(zmbaq_gui
    ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}
    ${u3dlib} -ldl -lGL -lGLEW -pthread

)
