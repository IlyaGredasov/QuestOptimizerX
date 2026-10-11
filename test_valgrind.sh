#!/usr/bin/env zsh
ROOT_DIR="$(pwd)"
BUILD_DIR="${ROOT_DIR}/valgrind-clang-build"

BIN_NAME="quest_optimizer_x"
BIN="${BUILD_DIR}/${BIN_NAME}"
INPUT_FILE="${INPUT_FILE:-${ROOT_DIR}/example.yaml}"

run_args=(
  --file "${INPUT_FILE}"
  --num_threads "${NUM_THREADS:-12}"
  --max_queue_size "${MAX_QUEUE_SIZE:-100000}"
  --error_afford "${ERROR_AFFORD:-1.2}"
  --depth_of_search "${DEPTH_OF_SEARCH:-50}"
  --log_interval_seconds "${LOG_INTERVAL_SECONDS:-1}"
)

echo "[1/2] Configure+build Debug (valgrind)"
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
