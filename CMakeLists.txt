cmake_minimum_required(VERSION 3.23)
project(sysfetch)

set(SOURCES)

if(WIN32)
    list(APPEND SOURCES fetch_win.c)
elseif(UNIX)
    list(APPEND SOURCES fetch_pos.c)
endif()

add_executable(sysfetch ${SOURCES})

target_link_libraries(sysfetch PRIVATE m)
