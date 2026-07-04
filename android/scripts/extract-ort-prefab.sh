#!/usr/bin/env bash
set -euo pipefail

ORT_VERSION="${ORT_VERSION:-1.20.0}"
OUT_DIR="${1:-ort-prefab}"
AAR_URL="https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/${ORT_VERSION}/onnxruntime-android-${ORT_VERSION}.aar"

echo "Downloading ONNX Runtime Android ${ORT_VERSION}..."
curl -fsSL "${AAR_URL}" -o /tmp/ort-android.aar
rm -rf "${OUT_DIR}"
mkdir -p "${OUT_DIR}"
unzip -q /tmp/ort-android.aar -d "${OUT_DIR}"

# AAR ships jni/ + headers/; CMake expects prefab/modules/onnxruntime layout.
ORT_MODULE="${OUT_DIR}/prefab/modules/onnxruntime"
mkdir -p "${ORT_MODULE}/include"
cp "${OUT_DIR}/headers/"*.h "${ORT_MODULE}/include/"

for abi_dir in "${OUT_DIR}/jni/"*; do
  abi="$(basename "${abi_dir}")"
  so="${abi_dir}/libonnxruntime.so"
  if [[ -f "${so}" ]]; then
    mkdir -p "${ORT_MODULE}/libs/android.${abi}"
    cp "${so}" "${ORT_MODULE}/libs/android.${abi}/libonnxruntime.so"
  fi
done

test -f "${ORT_MODULE}/libs/android.arm64-v8a/libonnxruntime.so"
test -f "${ORT_MODULE}/include/onnxruntime_c_api.h"
echo "Extracted prefab layout to ${OUT_DIR}"