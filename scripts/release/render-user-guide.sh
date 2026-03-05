#!/usr/bin/env bash

set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <source-markdown> <output-dir>" >&2
  exit 1
fi

SOURCE_MD="$1"
OUT_DIR="$2"

if [[ ! -f "${SOURCE_MD}" ]]; then
  echo "ERROR: user guide source not found: ${SOURCE_MD}" >&2
  exit 1
fi

mkdir -p "${OUT_DIR}"
cp "${SOURCE_MD}" "${OUT_DIR}/Kronos User Guide.md"

if command -v pandoc >/dev/null 2>&1; then
  pandoc "${SOURCE_MD}" -s -o "${OUT_DIR}/Kronos User Guide.html"
else
  # Fallback: ship readable plain-text and a minimal HTML wrapper.
  cp "${SOURCE_MD}" "${OUT_DIR}/Kronos User Guide.txt"
  {
    echo "<!doctype html>"
    echo "<html><head><meta charset=\"utf-8\"><title>Kronos User Guide</title></head><body><pre>"
    sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g' "${SOURCE_MD}"
    echo "</pre></body></html>"
  } > "${OUT_DIR}/Kronos User Guide.html"
fi

echo "Rendered user guide assets in: ${OUT_DIR}"
