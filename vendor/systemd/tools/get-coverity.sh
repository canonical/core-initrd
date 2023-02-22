#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later

# Download and extract coverity tool

set -e
set -o pipefail

# Environment check
if [ -z "$COVERITY_SCAN_TOKEN" ]; then
    echo >&2 'ERROR: COVERITY_SCAN_TOKEN must be set'
    exit 1
fi

# Use default values if not set
PLATFORM="$(uname)"
TOOL_BASE="${TOOL_BASE:-/tmp/coverity-scan-analysis}"
TOOL_ARCHIVE="${TOOL_ARCHIVE:-/tmp/cov-analysis-${PLATFORM}.tgz}"
TOOL_URL="https://scan.coverity.com/download/${PLATFORM}"

# Make sure wget is installed
sudo apt-get update && sudo apt-get -y install wget

# Get coverity tool
if [ ! -d "$TOOL_BASE" ]; then
    # Download Coverity Scan Analysis Tool
    if [ ! -e "$TOOL_ARCHIVE" ]; then
        echo -e "\033[33;1mDownloading Coverity Scan Analysis Tool...\033[0m"
        wget -nv -O "$TOOL_ARCHIVE" "$TOOL_URL" --post-data "project=$COVERITY_SCAN_PROJECT_NAME&token=$COVERITY_SCAN_TOKEN"
    fi

    # Extract Coverity Scan Analysis Tool
    echo -e "\033[33;1mExtracting Coverity Scan Analysis Tool...\033[0m"
    mkdir -p "$TOOL_BASE"
    pushd "$TOOL_BASE"
    tar xzf "$TOOL_ARCHIVE"
    popd
fi

echo -e "\033[33;1mCoverity Scan Analysis Tool can be found at $TOOL_BASE ...\033[0m"
