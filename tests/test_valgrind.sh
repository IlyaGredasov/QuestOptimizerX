#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd -P)"
BUILD_DIR="${ROOT_DIR}/valgrind-build"

BIN_NAME="quest_optimizer_x"
BIN="${BUILD_DIR}/${BIN_NAME}"
INPUT_FILE="${INPUT_FILE:-${SCRIPT_DIR}/test_valgrind.yaml}"

run_args=(
  --file "${INPUT_FILE}"
  --num_threads 12
  --max_queue_size 1000
  --error_afford 1.2
  --depth_of_search 50
  --log_interval_seconds 1
)

echo "[1/2] Configure+build Valgrind"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build "${BUILD_DIR}" -j

echo "[2/2] Helgrind"
valgrind \
  --tool=helgrind \
  --suppressions="${ROOT_DIR}/helgrind.supp" \
  --fair-sched=yes \
  --history-level=full \
  --error-exitcode=1 \
  "${BIN}" "${run_args[@]}"

rm -rf -- "${BUILD_DIR}"
