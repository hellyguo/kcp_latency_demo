cmake_minimum_required(VERSION 3.10)
project(Demo_GIT C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE "Debug")

include_directories(include)
link_directories(lib)

add_executable(kcpcli0 src/00kcp/kcpcli.c)
target_link_libraries(kcpcli0 kcp)
add_executable(kcpsrv0 src/00kcp/kcpsrv.c)
target_link_libraries(kcpsrv0 kcp)

add_executable(kcpcli1 src/01pthread/kcpcli.c)
target_link_libraries(kcpcli1 kcp)
set_target_properties(kcpcli1 PROPERTIES COMPILE_FLAGS "-pthread" LINK_FLAGS "-pthread")
add_executable(kcpsrv1 src/01pthread/kcpsrv.c)
target_link_libraries(kcpsrv1 kcp)
set_target_properties(kcpsrv1 PROPERTIES COMPILE_FLAGS "-pthread" LINK_FLAGS "-pthread")

add_executable(kcpcli2 src/02libev/kcpcli.c)
target_link_libraries(kcpcli2 kcp ev)
add_executable(kcpsrv2 src/02libev/kcpsrv.c)
target_link_libraries(kcpsrv2 kcp ev)

add_executable(kcpcli3 src/03libev_payload/kcpcli.c)
target_link_libraries(kcpcli3 kcp ev)
add_executable(kcpsrv3 src/03libev_payload/kcpsrv.c)
target_link_libraries(kcpsrv3 kcp ev)

add_executable(kcpcli4 src/04libev_remote/kcpcli.c)
target_link_libraries(kcpcli4 kcp ev)
add_executable(kcpsrv4 src/04libev_remote/kcpsrv.c)
target_link_libraries(kcpsrv4 kcp ev)

add_executable(kcpcli5 src/05libev_remote2/kcpcli.c)
target_link_libraries(kcpcli5 kcp ev)
add_executable(kcpsrv5 src/05libev_remote2/kcpsrv.c)
target_link_libraries(kcpsrv5 kcp ev)
