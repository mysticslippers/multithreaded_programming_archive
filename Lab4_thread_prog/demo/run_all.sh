#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "[1/4] building demo programs"
make -C "${SCRIPT_DIR}" clean
make -C "${SCRIPT_DIR}"

echo
echo "[2/4] run no_deadlock"
LD_PRELOAD="${ROOT_DIR}/sanitizer.so" "${SCRIPT_DIR}/no_deadlock"

echo
echo "[3/4] run deadlock_abba"
LD_PRELOAD="${ROOT_DIR}/sanitizer.so" "${SCRIPT_DIR}/deadlock_abba"

echo
echo "[4/4] run deadlock_cycle3"
LD_PRELOAD="${ROOT_DIR}/sanitizer.so" "${SCRIPT_DIR}/deadlock_cycle3"