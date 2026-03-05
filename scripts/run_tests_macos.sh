#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_PATH="${ROOT_DIR}/Builds/MacOSX/Kronos.xcodeproj"
TARGET_NAME="Kronos - Standalone Plugin"
CONFIGURATION="Debug"
SKIP_BUILD=0
APP_BINARY=""
BUILD_LOG_FILE="${BUILD_LOG_FILE:-/tmp/kronos_test_build.log}"

if [[ $# -gt 0 && "${1}" != --* ]]; then
  CONFIGURATION="${1}"
  shift
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    --app-binary)
      APP_BINARY="${2:-}"
      shift 2
      ;;
    *)
      echo "Unknown argument: $1" >&2
      echo "Usage: $0 [Debug|Release] [--skip-build] [--app-binary /path/to/Kronos]" >&2
      exit 1
      ;;
  esac
done

if [[ "${SKIP_BUILD}" != "1" ]]; then
  echo "Building ${TARGET_NAME} (${CONFIGURATION})..."
  xcodebuild \
    -project "${PROJECT_PATH}" \
    -target "${TARGET_NAME}" \
    -configuration "${CONFIGURATION}" \
    GCC_PREPROCESSOR_DEFINITIONS='$(inherited) JUCE_UNIT_TESTS=1' \
    CODE_SIGNING_ALLOWED=NO \
    CODE_SIGNING_REQUIRED=NO \
    build >"${BUILD_LOG_FILE}"
fi

if [[ -z "${APP_BINARY}" ]]; then
  APP_BINARY="${ROOT_DIR}/Builds/MacOSX/build/${CONFIGURATION}/Kronos.app/Contents/MacOS/Kronos"
fi

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
  if [[ "${SKIP_BUILD}" != "1" ]]; then
    echo "Build log: ${BUILD_LOG_FILE}"
  fi
  exit 1
fi

FAILURES="$(cat "${RESULT_FILE}")"
if [[ "${FAILURES}" != "0" ]]; then
  echo "Tests failed: ${FAILURES} failing assertions."
  exit 1
fi

echo "All tests passed."
