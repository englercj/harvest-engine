#!/usr/bin/env bash
# Copyright Chad Engler

if [[ $# -eq 0 ]]; then
    echo "The path to the project file must be the first argument."
fi

BUILD_DIR="build"
#PREMAKE_VERSION="nightly"
PREMAKE_VERSION="5.0.0-beta2"
ENGINE_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECT_PATH=$(realpath $1)

shift # remove the project path argument

OS_NAME="$(uname -s)"
case $OS_NAME in
    Linux*)
        PREMAKE_OS="linux"
        PREMAKE_EXT=".tar.gz"
        PREMAKE_ACTION="gmake2"
        PREMAKE_EXE="premake5"
        ;;

    MSYS_NT*)
        ;&
    CYGWIN*)
        ;&
    MINGW*)
        PREMAKE_OS="windows"
        PREMAKE_EXT=".zip"
        PREMAKE_ACTION="vs2022"
        PREMAKE_EXE="premake5.exe"
        ;;

    *)
        echo "Unknown OS, you'll have to get and run premake yourself."
        exit -1
esac

if [ "$PREMAKE_VERSION" != "nightly" ]; then
    PREMAKE_DL_FILE="premake-${PREMAKE_VERSION}-${PREMAKE_OS}${PREMAKE_EXT}"
    PREMAKE_DL_URL="https://github.com/premake/premake-core/releases/download/v${PREMAKE_VERSION}/${PREMAKE_DL_FILE}"
else
    PREMAKE_EXT=".zip"
    PREMAKE_DL_FILE="premake-${PREMAKE_OS}-x64.zip"
    PREMAKE_DL_URL="https://nightly.link/premake/premake-core/workflows/ci-workflow/master/${PREMAKE_DL_FILE}"
fi

if [ "$PREMAKE_EXT" == ".tar.gz" ]; then
    EXTRACT_CMD="tar -xzf"
    EXTRACT_FLAG="-C"
else
    EXTRACT_CMD="unzip -qq"
    EXTRACT_FLAG="-d"
fi

PREMAKE_DIR="$BUILD_DIR/premake/$PREMAKE_OS"
PREMAKE_PATH="$PREMAKE_DIR/$PREMAKE_DL_FILE"

mkdir -p $PREMAKE_DIR

if [[ ! -f "$PREMAKE_PATH" ]]; then
    echo "Downloading premake..."
    if command -v "curl" &> /dev/null; then
        curl -L -s -o $PREMAKE_PATH $PREMAKE_DL_URL
    elif command -v "wget" &> /dev/null; then
        wget -O $PREMAKE_PATH $PREMAKE_DL_URL
    else
        echo "Unable to download premake because neither wget nor curl are available."
        exit 1
    fi
fi

if [[ ! -f "$PREMAKE_DIR/$PREMAKE_EXE" ]]; then
    echo "Extracting premake..."
    $EXTRACT_CMD "$PREMAKE_PATH" $EXTRACT_FLAG "$PREMAKE_DIR"
    chmod +x "$PREMAKE_DIR/$PREMAKE_EXE"
fi

if [[ $# -eq 0 ]]; then
    "$PREMAKE_DIR/$PREMAKE_EXE" $PREMAKE_ACTION --file="$ENGINE_DIR/premake5.lua" --he_project="$PROJECT_PATH"
else
    "$PREMAKE_DIR/$PREMAKE_EXE" $@ --file="$ENGINE_DIR/premake5.lua" --he_project="$PROJECT_PATH"
fi
