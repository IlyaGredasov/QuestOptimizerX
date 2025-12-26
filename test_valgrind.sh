#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/valgrind-build"

BIN_NAME="QuestOptimizerX"
BIN="${BUILD_DIR}/bin/${BIN_NAME}"
INPUT_FILE="${ROOT_DIR}/example.txt"

run_args=(
  --file "${INPUT_FILE}"
  --num_threads 12
  --max_queue_size 1000
  --error_afford 1.2
  --depth_of_search 50
  --log_interval_seconds 1
  --disable_quest_line_names
  --disable_vertex_names
)

echo "[1/2] Configure+build Debug (valgrind)"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}" -j

echo "[2/2] Helgrind"
valgrind \
  --tool=helgrind \
  --fair-sched=yes \
  --history-level=full \
  --read-var-info=yes \
  --error-exitcode=1 \
  "${BIN}" "${run_args[@]}"
