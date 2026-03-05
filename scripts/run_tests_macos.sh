#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_PATH="${ROOT_DIR}/Builds/MacOSX/Kronos.xcodeproj"
TARGET_NAME="Kronos - Standalone Plugin"
CONFIGURATION="${1:-Debug}"

echo "Building ${TARGET_NAME} (${CONFIGURATION})..."
xcodebuild \
  -project "${PROJECT_PATH}" \
  -target "${TARGET_NAME}" \
  -configuration "${CONFIGURATION}" \
  GCC_PREPROCESSOR_DEFINITIONS='$(inherited) JUCE_UNIT_TESTS=1' \
  CODE_SIGNING_ALLOWED=NO \
  CODE_SIGNING_REQUIRED=NO \
  build >/tmp/kronos_test_build.log

APP_BINARY="${ROOT_DIR}/Builds/MacOSX/build/${CONFIGURATION}/Kronos.app/Contents/MacOS/Kronos"

if [[ ! -x "${APP_BINARY}" ]]; then
  echo "Test runner app not found: ${APP_BINARY}"
  exit 1
fi

RESULT_FILE="$(mktemp)"
LOG_FILE="$(mktemp)"
trap 'rm -f "${RESULT_FILE}" "${LOG_FILE}"' EXIT

echo "Running embedded JUCE tests..."
touch "${LOG_FILE}"
tail -n +1 -f "${LOG_FILE}" &
TAIL_PID=$!

KRONOS_TEST_MODE=1 \
KRONOS_RUN_TESTS=1 \
KRONOS_TEST_RESULTS_FILE="${RESULT_FILE}" \
KRONOS_TEST_LOG_FILE="${LOG_FILE}" \
"${APP_BINARY}" &
APP_PID=$!

for _ in {1..120}; do
  if [[ -s "${RESULT_FILE}" ]]; then
    break
  fi
  sleep 1
done

if kill -0 "${APP_PID}" >/dev/null 2>&1; then
  kill "${APP_PID}" >/dev/null 2>&1 || true
  wait "${APP_PID}" || true
fi

if kill -0 "${TAIL_PID}" >/dev/null 2>&1; then
  kill "${TAIL_PID}" >/dev/null 2>&1 || true
  wait "${TAIL_PID}" || true
fi

if [[ ! -s "${RESULT_FILE}" ]]; then
  echo "No test results produced."
  echo "Build log: /tmp/kronos_test_build.log"
  exit 1
fi

FAILURES="$(cat "${RESULT_FILE}")"
if [[ "${FAILURES}" != "0" ]]; then
  echo "Tests failed: ${FAILURES} failing assertions."
  exit 1
fi

echo "All tests passed."
