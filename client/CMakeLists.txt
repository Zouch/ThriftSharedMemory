project(client)

add_executable(client ${CMAKE_SOURCE_DIR}/client/client.cpp ${CMAKE_SOURCE_DIR}/transport.h ${CMAKE_SOURCE_DIR}/transport.cpp ${DST_THRIFT}/Hello.cpp ${DST_THRIFT}/hello_types.cpp ${DST_THRIFT}/hello_constants.cpp)
target_include_directories(client PRIVATE ${INCLUDES} ${DST_THRIFT})
target_link_libraries(client ${LIBS})
