cmake_minimum_required(VERSION 3.10)
project(Demo_GIT C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE "Debug")

include_directories(include)
link_directories(lib)

add_executable(kcpcli1 src/pthread/kcpcli.c)
target_link_libraries(kcpcli1 kcp)
set_target_properties(kcpcli1 PROPERTIES COMPILE_FLAGS "-pthread" LINK_FLAGS "-pthread")
add_executable(kcpsrv1 src/pthread/kcpsrv.c)
target_link_libraries(kcpsrv1 kcp)
set_target_properties(kcpsrv1 PROPERTIES COMPILE_FLAGS "-pthread" LINK_FLAGS "-pthread")

