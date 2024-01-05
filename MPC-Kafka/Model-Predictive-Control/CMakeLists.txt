project(MPC)

cmake_minimum_required (VERSION 3.5)

add_definitions(-std=c++11 -O3)
set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "${CXX_FLAGS}")

include_directories(/usr/local/include)
include_directories(src/Eigen-3.3)
include_directories(/usr/include/)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    include_directories(/usr/local/opt/openssl/include)
    link_directories(/usr/local/opt/openssl/lib)
    link_directories(/usr/local/Cellar/libuv/1.11.0/lib)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

set(sources src/MPC.cpp src/main.cpp)
add_executable(mpc ${sources})

# Explicitly set the link directories
link_directories(/lib/x86_64-linux-gnu)

# Link your executable with the libraries
target_link_libraries(mpc ipopt z ssl uv uWS rdkafka rdkafka++)
