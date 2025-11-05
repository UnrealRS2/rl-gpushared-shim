cmake_minimum_required(VERSION 3.15)
project(rl_gpushared_shim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JAVA_HOME "C:/Program Files/Java/jdk-17")

include_directories(
        "include"
        "${JAVA_HOME}/include"
        "${JAVA_HOME}/include/win32"
)

find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS} include)

if(WIN32)
    set(LIB_NAME "rl_gpushared_shim")
elseif(APPLE)
    set(LIB_NAME "rl_gpushared_shim")
else()
    set(LIB_NAME "rl_gpushared_shim")
endif()

set(SRC src/rl_gpushared_shim.cpp)
add_library(${LIB_NAME} SHARED ${SRC})

if(UNIX)
    target_link_libraries(${LIB_NAME} pthread)
endif()
