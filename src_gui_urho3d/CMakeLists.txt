project(zmbaq_gui_proj)

if(URHO3D_DIR)
    include_directories(${URHO3D_DIR}/include/Urho3D/ThirdParty ${URHO3D_DIR}/include/Urho3D/ThirdParty/SDL)
else(URHO3D_DIR)
    set(URHO3D_DIR ${DEPENDS_ROOT})
endif(URHO3D_DIR)

link_directories(${DEPENDS_ROOT}/lib ${DEPENDS_ROOT}/lib64 ${DEPENDS_ROOT}/lib64/Urho3D)

find_library(u3dlib NAMES libUrho3D.a Urho3D 
		PATHS
 ${CMAKE_SOURCE_DIR}/deps/lib ${CMAKE_SOURCE_DIR}/deps/lib/Urho3D

 ${DEPENDS_ROOT}/lib64/Urho3D ${DEPENDS_ROOT}/lib ${DEPENDS_ROOT}/lib/Urho3D
    ${URHO3D_DIR}/lib/Urho3D       ${URHO3D_DIR}/lib64/Urho3D
    ${URHO3D_DIR}/lib64 ${URHO3D_DIR}/lib64/Urho3D/
    ${FFMPEG_DIR}/lib)
message("u3d lib: ${u3dlib} dir: ${URHO3D_DIR}")

file(GLOB_RECURSE files_gui L10n.cpp L10n.h Sample.cpp)
add_executable(zmbaq_gui ${files_gui} ${RC_OUT})

target_link_libraries(zmbaq_gui
    ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}
    ${u3dlib} -ldl -lGL -lGLEW

)
