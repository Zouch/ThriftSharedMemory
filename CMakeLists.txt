cmake_minimum_required(VERSION 3.8)
project(TestsThrift)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set(THRIFT_IDL ${CMAKE_SOURCE_DIR}/hello.thrift)
set(DST_THRIFT ${CMAKE_SOURCE_DIR}/gen-cpp)

if (UNIX)
	set(INCLUDES "/usr/local/include ${DST_THRIFT} ../trunk/Vertigo/External_Linux/boost_1_66_0/build/include")
	set(LIB_DIRS "/usr/local/lib")
	set(LIBS thrift pthread rt)
	set(THRIFT_COMPILER thrift)
else (UNIX)
	set(EXT_VERTIGO "G:\\External_Vertigo_trunk\\External_Win")
	set(EXT_VRGIS "G:\\External_VrGIS_trunk\\External_Win")
	set(THRIFT_ROOT "${EXT_VRGIS}\\thrift-0.11.0")
	set(INCLUDES "${THRIFT_ROOT}\\include;${EXT_VERTIGO}\\boost_1_66_0")
	set(LIB_DIRS "${THRIFT_ROOT}\\lib\\vc14.x64;${EXT_VERTIGO}\\boost_1_66_0\\stage\\lib;${EXT_VERTIGO}\\openssl-1.0.2n\\lib\\vc14.x64")
	set(LIBS optimized libthrift debug libthriftd general libeay32 ssleay32)
	set(THRIFT_COMPILER "${THRIFT_ROOT}\\bin\\thrift")
endif (UNIX)

link_directories(${LIB_DIRS})

add_custom_command(
	OUTPUT ${DST_THRIFT}/Hello.cpp ${DST_THRIFT}/hello_types.cpp ${DST_THRIFT}/hello_constants.cpp
	COMMAND ${THRIFT_COMPILER}
	ARGS --gen cpp:no_skeleton -out ${DST_THRIFT} -r ${THRIFT_IDL}
	MAIN_DEPENDENCY ${THRIFT_IDL}
	)

add_subdirectory(server)
add_subdirectory(client)
