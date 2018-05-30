project(server)

add_executable(server ${CMAKE_SOURCE_DIR}/server/server.cpp ${DST_THRIFT}/Hello.cpp ${DST_THRIFT}/hello_types.cpp ${DST_THRIFT}/hello_constants.cpp)
target_include_directories(server PRIVATE ${INCLUDES} ${DST_THRIFT})
target_link_libraries(server ${LIBS})
