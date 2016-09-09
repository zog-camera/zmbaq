project(WebGrep)

option(USE_LIBNEON "Compile using NEON https/webdav client library" OFF)
option(USE_LIBCURL "Compile using cURL library" OFF)
option(USE_QTNETWORK "Experimental: Compile using QtNetwork async i/o" OFF)
option(DO_MEMADDR_SANITIZE "Option for GCC/Clang to sanitize memory access" OFF)
# dependencies:
# -pthread -lssl -lcrypto {-lneon OR -lcurl}

if(NOT USE_QTNETWORK)
	if(WIN32)
		set(USE_LIBCURL ON)
		add_definitions(-DWITH_LIBCURL)
	else()
		# on Linux/Max both of NEON and cURL are available:
		if(NOT USE_LIBCURL)
			set(USE_LIBNEON ON)
			message("Using NEON for HTTPS")
			add_definitions(-DWITH_LIBNEON)
		else()
			set(USE_LIBCURL ON)
			message("Using cURL for HTTPS")
			add_definitions(-DWITH_LIBCURL)
		endif()
	endif()
else()
	message("Using QtNetwork for HTTPS")
	add_definitions(-DWITH_QTNETWORK)
endif()

link_directories(${DEPENDS_ROOT}/lib ${DEPENDS_ROOT}/bin)
include_directories(${DEPENDS_ROOT}/include)

if(NOT USE_QTNETWORK)
if(USE_LIBNEON)
#neon
  set(NETW_LIB_NAME "neon")
  set(NETW_SRC http_impl/ch_ctx_neon.cpp http_impl/ch_ctx_neon.h )
  find_library(NETW_LIB NAMES ${NETW_LIB_NAME})
else()
  set(NETW_LIB_NAME "curl")
  set(NETW_SRC http_impl/ch_ctx_curl.cpp http_impl/ch_ctx_curl.h )
  find_library(NETW_LIB NAMES ${NETW_LIB_NAME})
endif()
endif(NOT USE_QTNETWORK)

file(GLOB src_wgrep *.cpp *.h) 
message("Linker will use ${NETW_LIB} for http.")

add_library(webgrep SHARED ${src_wgrep}  ${NETW_SRC})
target_compile_features(webgrep PUBLIC cxx_constexpr)
if(DO_MEMADDR_SANITIZE)
        add_definitions("-fsanitize=address")
        target_link_libraries(webgrep -lasan)
endif()

target_link_libraries(webgrep -pthread -lssl -lcrypto ${NETW_LIB})

