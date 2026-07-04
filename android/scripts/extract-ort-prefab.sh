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

test -f "${OUT_DIR}/prefab/modules/onnxruntime/libs/android.arm64-v8a/libonnxruntime.so"
echo "Extracted prefab to ${OUT_DIR}"