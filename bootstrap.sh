#!/usr/bin/env bash

BUILD_DIR="build"
PREMAKE_VERSION="5.0.0-alpha16"

OS_NAME="$(uname -s)"
case $OS_NAME in
    Linux*)
        PREMAKE_OS="linux"
        PREMAKE_EXT=".tar.gz"
        PREMAKE_ACTION="gmake2 --cc=clang"
        PREMAKE_EXE="premake5"
        EXTRACT_CMD="tar -xzf"
        EXTRACT_FLAG="-C"
        ;;

    MSYS_NT*)
        ;&
    CYGWIN*)
        ;&
    MINGW*)
        PREMAKE_OS="windows"
        PREMAKE_EXT=".zip"
        PREMAKE_ACTION="vs2019"
        PREMAKE_EXE="premake5.exe"
        EXTRACT_CMD="unzip -qq"
        EXTRACT_FLAG="-d"
        ;;

    *)
        echo "Unknown OS, you'll have to get and run premake yourself."
        exit -1
esac

PREMAKE_DL_FILE="premake-${PREMAKE_VERSION}-${PREMAKE_OS}${PREMAKE_EXT}"
PREMAKE_DL_URL="https://github.com/premake/premake-core/releases/download/v${PREMAKE_VERSION}/${PREMAKE_DL_FILE}"

PREMAKE_DIR="$BUILD_DIR/premake/$PREMAKE_OS"
PREMAKE_PATH="$PREMAKE_DIR/$PREMAKE_DL_FILE"

mkdir -p $PREMAKE_DIR

if [[ ! -f "$PREMAKE_PATH" ]]; then
    echo "Downloading premake..."
    curl -L -s -o $PREMAKE_PATH $PREMAKE_DL_URL
fi

if [[ ! -f "$PREMAKE_DIR/$PREMAKE_EXE" ]]; then
    echo "Extracting premake..."
    $EXTRACT_CMD "$PREMAKE_PATH" $EXTRACT_FLAG "$PREMAKE_DIR"
    chmod +x "$PREMAKE_DIR/$PREMAKE_EXE"
fi

if [[ $# -eq 0 ]]; then
    "$PREMAKE_DIR/$PREMAKE_EXE" $PREMAKE_ACTION
else
    "$PREMAKE_DIR/$PREMAKE_EXE" $@
fi
