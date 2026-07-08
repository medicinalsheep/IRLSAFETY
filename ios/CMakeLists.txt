# IRLSAFETY+ — iOS static lib scaffold (phase I1)
# Builds libirlsafety for iOS / iOS Simulator without the OBS plugin shell.
#
# Example (macOS host):
#   cmake -S ios -B build-ios \
#     -G Xcode \
#     -DCMAKE_SYSTEM_NAME=iOS \
#     -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
#     -DIRLSAFETY_ENABLE_ONNX=OFF
#   cmake --build build-ios --config Release
#
# ONNX Runtime iOS (I4) will set IRLSAFETY_HAS_ONNX_RUNTIME and link ORT.

cmake_minimum_required(VERSION 3.28)

set(IRLSAFETY_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Repo root")

project(irlsafety_ios LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)

# I1: compile portable core with stub detector/OCR (no ORT AAR yet).
set(IRLSAFETY_ENABLE_ONNX OFF CACHE BOOL "Enable ONNX (I4)" FORCE)
set(IRLSAFETY_OCR_BACKEND "stub" CACHE STRING "OCR backend" FORCE)
set(IRLSAFETY_HAS_ONNX_RUNTIME FALSE)

include("${IRLSAFETY_REPO_ROOT}/cmake/ios/libirlsafety_ios.cmake")

# Convenience alias for Xcode consumers
add_library(IRLSafetyCore ALIAS libirlsafety)

message(STATUS "IRLSAFETY+ iOS I1: libirlsafety static target ready (stub YOLO/OCR)")
message(STATUS "  Next: I2 SwiftUI shell · I3 pixel bridge · I4 ORT iOS")
