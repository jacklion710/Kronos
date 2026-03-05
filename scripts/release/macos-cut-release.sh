#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

RELEASE_VERSION="${1:-${VERSION:-}}"
TEAM_ID="${TEAM_ID:-84KWJ3HCR4}"
NOTARY_PROFILE="${NOTARY_PROFILE:-kronos-plugin}"
NOTARIZE="${NOTARIZE:-1}"
PREPARE_ONLY="${PREPARE_ONLY:-0}"

if [[ -z "${RELEASE_VERSION}" ]]; then
  echo "Usage: $0 <version>" >&2
  echo "Example: $0 1.1.0-beta.1" >&2
  exit 1
fi

if [[ ! "${RELEASE_VERSION}" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)([.-][0-9A-Za-z.-]+)?$ ]]; then
  echo "ERROR: Release version must be semver (for example 1.0.3 or 1.1.0-beta.1)." >&2
  exit 1
fi

CORE_VERSION="${RELEASE_VERSION%%-*}"
IFS='.' read -r MAJOR MINOR PATCH <<< "${CORE_VERSION}"

# JUCE version code format: major<<16 | minor<<8 | patch
VERSION_CODE_DEC=$(( MAJOR * 65536 + MINOR * 256 + PATCH ))
printf -v VERSION_CODE_HEX "0x%05X" "${VERSION_CODE_DEC}"

APP_IDENTITY="${MACOS_APP_SIGNING_IDENTITY:-Developer ID Application: Jacob Leone (${TEAM_ID})}"
INSTALLER_IDENTITY="${MACOS_INSTALLER_SIGNING_IDENTITY:-Developer ID Installer: Jacob Leone (${TEAM_ID})}"

has_identity() {
  local identity="$1"
  security find-identity -v -p codesigning 2>/dev/null | grep -Fq "\"${identity}\""
}

has_installer_identity() {
  local identity="$1"
  security find-certificate -a -c "${identity}" -Z ~/Library/Keychains/login.keychain-db 2>/dev/null | grep -Fq "${identity}"
}

if ! has_identity "${APP_IDENTITY}"; then
  echo "ERROR: App signing identity not found: ${APP_IDENTITY}" >&2
  exit 1
fi

if ! has_installer_identity "${INSTALLER_IDENTITY}"; then
  echo "ERROR: Installer signing identity not found: ${INSTALLER_IDENTITY}" >&2
  exit 1
fi

if [[ "${NOTARIZE}" == "1" ]]; then
  if ! xcrun notarytool history --keychain-profile "${NOTARY_PROFILE}" >/dev/null 2>&1; then
    echo "ERROR: Notary profile '${NOTARY_PROFILE}' not found." >&2
    echo "Run: xcrun notarytool store-credentials ${NOTARY_PROFILE} --apple-id \"<APPLE_ID>\" --team-id \"${TEAM_ID}\" --password \"<APP_SPECIFIC_PASSWORD>\"" >&2
    exit 1
  fi
fi

JUCER_FILE="${REPO_ROOT}/Kronos.jucer"
PLUGIN_DEFINES_FILE="${REPO_ROOT}/JuceLibraryCode/JucePluginDefines.h"
PBXPROJ_FILE="${REPO_ROOT}/Builds/MacOSX/Kronos.xcodeproj/project.pbxproj"

# Keep project/plugin metadata aligned with the release core version.
perl -0pi -e "s/(<JUCERPROJECT\\b[^>]*\\bversion=\\\")([^\\\"]+)(\\\")/\$1${CORE_VERSION}\$3/" "${JUCER_FILE}"

perl -0pi -e "s/(#define\\s+JucePlugin_Version\\s+).*/\$1${CORE_VERSION}/; \
               s/(#define\\s+JucePlugin_VersionCode\\s+).*/\$1${VERSION_CODE_HEX}/; \
               s/(#define\\s+JucePlugin_VersionString\\s+).*/\$1\\\"${CORE_VERSION}\\\"/; \
               s/(#define\\s+JucePlugin_ARADocumentArchiveID\\s+).*/\$1\\\"com.Jack_Lion.Kronos.aradocumentarchive.${CORE_VERSION}\\\"/" "${PLUGIN_DEFINES_FILE}"

perl -0pi -e "s/JucePlugin_Version=[^\",]+/JucePlugin_Version=${CORE_VERSION}/g; \
               s/JucePlugin_VersionCode=0x[0-9A-Fa-f]+/JucePlugin_VersionCode=${VERSION_CODE_HEX}/g; \
               s/JucePlugin_VersionString=\\\\\"[^\\\\\"]+\\\\\"/JucePlugin_VersionString=\\\\\"${CORE_VERSION}\\\\\"/g; \
               s/JucePlugin_ARADocumentArchiveID=\\\\\"com\\.Jack_Lion\\.Kronos\\.aradocumentarchive\\.[^\\\\\"]+\\\\\"/JucePlugin_ARADocumentArchiveID=\\\\\"com.Jack_Lion.Kronos.aradocumentarchive.${CORE_VERSION}\\\\\"/g; \
               s/JUCE_APP_VERSION=[^\",]+/JUCE_APP_VERSION=${CORE_VERSION}/g; \
               s/JUCE_APP_VERSION_HEX=0x[0-9A-Fa-f]+/JUCE_APP_VERSION_HEX=${VERSION_CODE_HEX}/g; \
               s/-version \\\\\\\"[^\\\\\"]+\\\\\\\"/-version \\\\\\\"${CORE_VERSION}\\\\\\\"/g; \
               s/-version \\\"[^\\\"]+\\\"/-version \\\"${CORE_VERSION}\\\"/g" "${PBXPROJ_FILE}"

echo "Prepared release metadata:" 
echo "  release version: ${RELEASE_VERSION}" 
echo "  core version:    ${CORE_VERSION}" 
echo "  version code:    ${VERSION_CODE_HEX}" 
echo "  app identity:    ${APP_IDENTITY}" 
echo "  installer ident: ${INSTALLER_IDENTITY}" 

if [[ "${PREPARE_ONLY}" == "1" ]]; then
  echo "PREPARE_ONLY=1 set, skipping build/sign/notarize."
  exit 0
fi

VERSION="${RELEASE_VERSION}" \
MACOS_APP_SIGNING_IDENTITY="${APP_IDENTITY}" \
MACOS_INSTALLER_SIGNING_IDENTITY="${INSTALLER_IDENTITY}" \
NOTARIZE="${NOTARIZE}" \
NOTARY_PROFILE="${NOTARY_PROFILE}" \
"${SCRIPT_DIR}/macos-release.sh"
