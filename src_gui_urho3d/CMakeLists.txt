project(zmbaq_gui_proj)

if(URHO3D_DIR)
    include_directories(${URHO3D_DIR}/include/Urho3D/ThirdParty ${URHO3D_DIR}/include/Urho3D/ThirdParty/SDL)
endif()


find_library(u3dlib NAMES Urho3D libUrho3D.a
	PATHS ${DEPENDS_ROOT}/lib/Urho3D ${DEPENDS_ROOT}/lib
    ${URHO3D_DIR}/lib/Urho3D       ${URHO3D_DIR}/lib64/Urho3D
    ${URHO3D_DIR}/lib64 ${URHO3D_DIR}/lib64/Urho3D/
    ${FFMPEG_DIR}/lib)
message("u3d lib: ${u3dlib} dir: ${URHO3D_DIR}")

file(GLOB_RECURSE files_gui *.cpp *.h)
add_executable(zmbaq_gui ${files_gui} ${RC_OUT})

target_link_libraries(zmbaq_gui
    ${ASAN_LINK_FLAGS} ${PROF_LINK_FLAGS}
    ${u3dlib} -ldl -lGL -lGLEW
 videoentity
)
