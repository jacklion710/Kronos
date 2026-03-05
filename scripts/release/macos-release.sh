#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

PROJECT_PATH="${PROJECT_PATH:-${REPO_ROOT}/Builds/MacOSX/Kronos.xcodeproj}"
SCHEME="${SCHEME:-Kronos - All}"
CONFIGURATION="${CONFIGURATION:-Release}"
VERSION="${VERSION:-}"
PRODUCT_NAME="${PRODUCT_NAME:-Kronos}"
TEAM_ID="${TEAM_ID:-84KWJ3HCR4}"
OUTPUT_ROOT="${OUTPUT_ROOT:-${REPO_ROOT}/dist/macos}"
PKG_IDENTIFIER="${PKG_IDENTIFIER:-com.Jack_Lion.Kronos.installer}"
NOTARIZE="${NOTARIZE:-1}"
NOTARY_PROFILE="${NOTARY_PROFILE:-kronos-plugin}"
SKIP_XCODEBUILD="${SKIP_XCODEBUILD:-0}"
USER_GUIDE_SOURCE="${USER_GUIDE_SOURCE:-${REPO_ROOT}/docs/user-guide.md}"
VST3_INSTALL_LOCATION="${VST3_INSTALL_LOCATION:-Library/Audio/Plug-Ins/VST3}"
AU_INSTALL_LOCATION="${AU_INSTALL_LOCATION:-Library/Audio/Plug-Ins/Components}"
GUIDE_INSTALL_LOCATION="${GUIDE_INSTALL_LOCATION:-Library/Application Support/${PRODUCT_NAME}/User Guide}"

MACOS_APP_SIGNING_IDENTITY="${MACOS_APP_SIGNING_IDENTITY:-Developer ID Application: Jacob Leone (${TEAM_ID})}"
MACOS_INSTALLER_SIGNING_IDENTITY="${MACOS_INSTALLER_SIGNING_IDENTITY:-Developer ID Installer: Jacob Leone (${TEAM_ID})}"

if [[ -z "${VERSION}" ]]; then
  VERSION="$(sed -n 's/^[[:space:]]*#define[[:space:]]*JucePlugin_VersionString[[:space:]]*"\(.*\)"/\1/p' "${REPO_ROOT}/JuceLibraryCode/JucePluginDefines.h" | head -n1 || true)"
fi
VERSION="${VERSION//$'\r'/}"

if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+([.-][0-9A-Za-z.-]+)?$ ]]; then
  echo "ERROR: VERSION must be semver (for example 1.0.3 or 1.1.0-beta.1). Got '${VERSION:-<empty>}'." >&2
  exit 1
fi

if [[ -z "${MACOS_APP_SIGNING_IDENTITY}" ]]; then
  echo "ERROR: MACOS_APP_SIGNING_IDENTITY is required." >&2
  exit 1
fi

if [[ -z "${MACOS_INSTALLER_SIGNING_IDENTITY}" ]]; then
  echo "ERROR: MACOS_INSTALLER_SIGNING_IDENTITY is required." >&2
  exit 1
fi

BUILD_DIR="${REPO_ROOT}/Builds/MacOSX/build/${CONFIGURATION}"
STAGE_DIR="${OUTPUT_ROOT}/staging"
COMPONENT_PKG_DIR="${STAGE_DIR}/components"
DIST_XML="${STAGE_DIR}/distribution.xml"
VST3_COMPONENT_PKG="${COMPONENT_PKG_DIR}/${PRODUCT_NAME}-vst3.pkg"
AU_COMPONENT_PKG="${COMPONENT_PKG_DIR}/${PRODUCT_NAME}-au.pkg"
GUIDE_PAYLOAD_DIR="${STAGE_DIR}/guide-payload"
GUIDE_COMPONENT_PKG="${COMPONENT_PKG_DIR}/${PRODUCT_NAME}-guide.pkg"
SIGNED_PKG="${OUTPUT_ROOT}/${PRODUCT_NAME}-${VERSION}.pkg"
SHA_FILE="${SIGNED_PKG}.sha256"

VST3_SOURCE="${BUILD_DIR}/${PRODUCT_NAME}.vst3"
AU_SOURCE="${BUILD_DIR}/${PRODUCT_NAME}.component"

rm -rf "${OUTPUT_ROOT}"
mkdir -p "${OUTPUT_ROOT}" "${COMPONENT_PKG_DIR}" "${GUIDE_PAYLOAD_DIR}"

pushd "${REPO_ROOT}" >/dev/null

if [[ "${SKIP_XCODEBUILD}" != "1" ]]; then
  xcodebuild \
    -project "${PROJECT_PATH}" \
    -scheme "${SCHEME}" \
    -configuration "${CONFIGURATION}" \
    -destination 'generic/platform=macOS' \
    clean build
fi

if [[ ! -d "${VST3_SOURCE}" ]]; then
  echo "ERROR: Expected VST3 artifact not found: ${VST3_SOURCE}" >&2
  exit 1
fi

if [[ ! -d "${AU_SOURCE}" ]]; then
  echo "ERROR: Expected AU artifact not found: ${AU_SOURCE}" >&2
  exit 1
fi

# Enforce test gate for release packaging. Any failing test aborts the release build.
if [[ "${CONFIGURATION}" == "Release" ]]; then
  echo "[macos-release] Running embedded test suite gate (Release)..."
  "${REPO_ROOT}/scripts/run_tests_macos.sh" "Release"
fi

sign_bundle() {
  local path="$1"
  codesign --force --deep --options runtime --timestamp --sign "${MACOS_APP_SIGNING_IDENTITY}" "${path}"
  codesign --verify --deep --strict --verbose=2 "${path}"
}

sign_bundle "${VST3_SOURCE}"
sign_bundle "${AU_SOURCE}"

"${SCRIPT_DIR}/render-user-guide.sh" "${USER_GUIDE_SOURCE}" "${GUIDE_PAYLOAD_DIR}"

pkgbuild \
  --component "${VST3_SOURCE}" \
  --identifier "${PKG_IDENTIFIER}.vst3" \
  --version "${VERSION}" \
  --install-location "${VST3_INSTALL_LOCATION}" \
  "${VST3_COMPONENT_PKG}"

pkgbuild \
  --component "${AU_SOURCE}" \
  --identifier "${PKG_IDENTIFIER}.au" \
  --version "${VERSION}" \
  --install-location "${AU_INSTALL_LOCATION}" \
  "${AU_COMPONENT_PKG}"

pkgbuild \
  --root "${GUIDE_PAYLOAD_DIR}" \
  --identifier "${PKG_IDENTIFIER}.guide" \
  --version "${VERSION}" \
  --install-location "${GUIDE_INSTALL_LOCATION}" \
  "${GUIDE_COMPONENT_PKG}"

cat > "${DIST_XML}" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>${PRODUCT_NAME} ${VERSION}</title>
  <options customize="always" require-scripts="false"/>
  <domains enable_anywhere="false" enable_currentUserHome="true" enable_localSystem="true"/>
  <choices-outline>
    <line choice="choice.vst3"/>
    <line choice="choice.au"/>
    <line choice="choice.guide"/>
  </choices-outline>
  <choice id="choice.vst3" visible="true" selected="true" enabled="true" title="${PRODUCT_NAME} VST3" description="Install VST3 plugin format">
    <pkg-ref id="${PKG_IDENTIFIER}.vst3"/>
  </choice>
  <choice id="choice.au" visible="true" selected="true" enabled="true" title="${PRODUCT_NAME} AU" description="Install Audio Unit plugin format">
    <pkg-ref id="${PKG_IDENTIFIER}.au"/>
  </choice>
  <choice id="choice.guide" visible="true" selected="true" enabled="true" title="${PRODUCT_NAME} User Guide" description="Install local user guide files (HTML/Markdown)">
    <pkg-ref id="${PKG_IDENTIFIER}.guide"/>
  </choice>
  <pkg-ref id="${PKG_IDENTIFIER}.vst3" version="${VERSION}">$(basename "${VST3_COMPONENT_PKG}")</pkg-ref>
  <pkg-ref id="${PKG_IDENTIFIER}.au" version="${VERSION}">$(basename "${AU_COMPONENT_PKG}")</pkg-ref>
  <pkg-ref id="${PKG_IDENTIFIER}.guide" version="${VERSION}">$(basename "${GUIDE_COMPONENT_PKG}")</pkg-ref>
</installer-gui-script>
EOF

productbuild \
  --distribution "${DIST_XML}" \
  --package-path "${COMPONENT_PKG_DIR}" \
  --identifier "${PKG_IDENTIFIER}" \
  --version "${VERSION}" \
  --sign "${MACOS_INSTALLER_SIGNING_IDENTITY}" \
  "${SIGNED_PKG}"

if [[ "${NOTARIZE}" == "1" ]]; then
  submit_output="$(xcrun notarytool submit "${SIGNED_PKG}" --keychain-profile "${NOTARY_PROFILE}" 2>&1 || true)"
  echo "${submit_output}"

  submission_id="$(printf '%s\n' "${submit_output}" | sed -n 's/^ *id: *//p' | head -n1)"
  if [[ -z "${submission_id}" ]]; then
    echo "ERROR: Failed to parse notary submission ID." >&2
    exit 1
  fi

  wait_ok=0
  for attempt in 1 2 3; do
    if xcrun notarytool wait "${submission_id}" --keychain-profile "${NOTARY_PROFILE}" --timeout 20m; then
      wait_ok=1
      break
    fi
    echo "Warning: notarytool wait failed on attempt ${attempt}/3, retrying..." >&2
    sleep 5
  done

  if [[ "${wait_ok}" != "1" ]]; then
    echo "ERROR: Notarization wait failed for submission ${submission_id}." >&2
    xcrun notarytool log "${submission_id}" --keychain-profile "${NOTARY_PROFILE}" || true
    exit 1
  fi

  xcrun stapler staple "${SIGNED_PKG}"
  spctl -a -vv --type install "${SIGNED_PKG}"
else
  if ! spctl -a -vv --type install "${SIGNED_PKG}"; then
    echo "Warning: installer is signed but not notarized (expected with NOTARIZE=0)." >&2
  fi
fi
shasum -a 256 "${SIGNED_PKG}" > "${SHA_FILE}"

popd >/dev/null

echo "Release package ready: ${SIGNED_PKG}"
echo "SHA256 file ready: ${SHA_FILE}"
