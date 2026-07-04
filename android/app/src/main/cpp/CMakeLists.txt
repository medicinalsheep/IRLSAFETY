cmake_minimum_required(VERSION 3.22.1)
project(irlsafety_jni)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../../../../cmake/android/libirlsafety_ndk.cmake")

add_library(irlsafety_jni SHARED irlsafety_jni.cpp yuv_convert.cpp)

target_include_directories(irlsafety_jni PRIVATE "${IRLSAFETY_REPO_ROOT}/src")

target_link_libraries(irlsafety_jni PRIVATE libirlsafety log)