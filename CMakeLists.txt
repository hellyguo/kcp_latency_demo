cmake_minimum_required(VERSION 3.10)
project(Demo_GIT C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE "Debug")

include_directories(include)
link_directories(lib)

add_executable(kcpcli src/kcpcli.c)
target_link_libraries(kcpcli kcp)
set_target_properties(kcpcli PROPERTIES COMPILE_FLAGS "-pthread" LINK_FLAGS "-pthread")
add_executable(kcpsrv src/kcpsrv.c)
target_link_libraries(kcpsrv kcp)
set_target_properties(kcpsrv PROPERTIES COMPILE_FLAGS "-pthread" LINK_FLAGS "-pthread")

