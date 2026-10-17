cmake_minimum_required(VERSION 3.16)

project(chatter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(chatter-server server/main.cpp)
add_executable(chatter-client client/main.cpp)