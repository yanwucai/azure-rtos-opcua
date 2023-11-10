cmake_minimum_required(VERSION 3.13..3.22 FATAL_ERROR)
set(CMAKE_C_STANDARD 99)

# Define paths
set(BASE_DIR ${CMAKE_SOURCE_DIR})
set(LIBS_DIR ${BASE_DIR}/libs)

# Define the CPU architecture for ThreadX
set(THREADX_ARCH "linux")
set(THREADX_TOOLCHAIN "gnu")

set(TX_THREAD_ENABLE_PERFORMANCE_INFO)

# Perform compiler test with the static library
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(LINUX_FLAGS "-g -pthread")

set(CMAKE_C_FLAGS   "${LINUX_FLAGS} -m32" CACHE INTERNAL "c compiler flags")
set(CMAKE_CXX_FLAGS "${LINUX_FLAGS} -m32 -fno-rtti -fno-exceptions" CACHE INTERNAL "cxx compiler flags")
set(CMAKE_ASM_FLAGS "${LINUX_FLAGS} -x assembler-with-cpp" CACHE INTERNAL "asm compiler flags")
set(CMAKE_EXE_LINKER_FLAGS "${LINUX_FLAGS} ${LD_FLAGS} -Wl,--gc-sections" CACHE INTERNAL "exe link flags")

SET(CMAKE_C_FLAGS_DEBUG "-g -ggdb3" CACHE INTERNAL "c debug compiler flags")
SET(CMAKE_CXX_FLAGS_DEBUG "-Og -g -ggdb3" CACHE INTERNAL "cxx debug compiler flags")
SET(CMAKE_ASM_FLAGS_DEBUG "-g -ggdb3" CACHE INTERNAL "asm debug compiler flags")

SET(CMAKE_C_FLAGS_RELEASE "-O3" CACHE INTERNAL "c release compiler flags")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3" CACHE INTERNAL "cxx release compiler flags")
SET(CMAKE_ASM_FLAGS_RELEASE "" CACHE INTERNAL "asm release compiler flags")


if(NOT DEFINED IF_NAME)
  set(IF_NAME "veth1")
endif()
message(STATUS "Bind to interface: ${IF_NAME}")


set(NX_USER_FILE ${CMAKE_SOURCE_DIR}/src/nx_user.h)

set(TX_USER_FILE ${CMAKE_SOURCE_DIR}/src/tx_user.h)

set(NXD_ENABLE_FILE_SERVERS
    OFF
    CACHE BOOL "Includes a dependency on FileX to support 'server' protocol handlers (default is ON)")

set(NXD_ENABLE_BSD ON)

set(UA_ARCHITECTURE "azurertos" CACHE STRING "Architecture to build open62541 on")
set(UA_ENABLE_PUBSUB ON CACHE BOOL "Enable the PubSub protocol")
set(UA_ENABLE_DISCOVERY ON CACHE BOOL "Enable Discovery Service (LDS)")
set(UA_FORCE_32BIT ON CACHE BOOL "Force compilation as 32-bit executable")
set(UA_MULTITHREADING 100 CACHE STRING "Level of multithreading (0-99: No Multithreading, >=100: Thread-Safe API)")
set(UA_ENABLE_PUBSUB_ETH_UADP ON CACHE BOOL "Enable publish/subscribe UADP over Ethernet")

# Project
set(PROJECT project_azurertos_opcua)
project(${PROJECT} VERSION 0.1.0 LANGUAGES C)

add_subdirectory(${LIBS_DIR}/threadx lib/threadx)
add_subdirectory(${LIBS_DIR}/netxduo lib/netxduo)
add_subdirectory(${LIBS_DIR}/open62541 lib/open62541)

macro(add_example EXAMPLE_NAME EXAMPLE_SOURCE)
  add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE} src/nx_linux_network_driver.c)
  target_link_libraries(${EXAMPLE_NAME} open62541::open62541 azrtos::netxduo)
  target_compile_definitions(${EXAMPLE_NAME} PUBLIC -DNX_LINUX_INTERFACE_NAME=\"${IF_NAME}\")
endmacro()


add_example(server src/server.c)
add_example(client src/client.c)
add_example(pubsub_publish src/pubsub_publish.c)
add_example(pubsub_subscribe src/pubsub_subscribe.c)
add_example(pubsub_publish_rt src/pubsub_publish_rt.c)
add_example(pubsub_subscribe_rt src/pubsub_subscribe_rt.c)
add_example(discovery_server_regist src/discovery_server_regist.c)
add_example(discovery_client_find_servers src/discovery_client_find_servers.c)
