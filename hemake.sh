#!/usr/bin/env bash
# Copyright Chad Engler

# Resolve the project path
if [[ $# -eq 0 ]]; then
    PROJECT_PATH=$(realpath "he_project.toml")
else
    PROJECT_PATH=$(realpath $1)
    shift
fi

if [[ ! -f "$PROJECT_PATH" ]]; then
    echo "Project file not found: $PROJECT_PATH"
    exit 1
fi

# Use the script's directory as the working directory
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
pushd "$SCRIPT_DIR" > /dev/null

PROJECT_DIR=$(dirname $PROJECT_PATH)
BUILD_DIR="$PROJECT_DIR/build"

HEMAKE_BUILD_CFG="Release"
HEMAKE_BUILD_DIR="$BUILD_DIR/hemake"
HEMAKE_BUILD_ASSEMBLY="$HEMAKE_BUILD_DIR/bin/$HEMAKE_BUILD_CFG/net8.0/Harvest.Make.App.dll"

HEMAKE_SRC_DIR="$SCRIPT_DIR/plugins/hemake"
HEMAKE_SRC_PROJ="$HEMAKE_SRC_DIR/Harvest.Make.App/Harvest.Make.App.csproj"

DOTNET_CHANNEL="8.0"
DOTNET_DIR="$BUILD_DIR/dotnet"
DOTNET_EXE="$DOTNET_DIR/dotnet"

# Check for a local dotnet installation, and if there isn't one download it
if [[ ! -d "$DOTNET_DIR" ]]; then
    echo "Downloading .NET SDK..."

    function download_file()
    {
        local url=$1
        local file=$2

        if command -v "curl" &> /dev/null; then
            curl -L -s -o $file $url
        elif command -v "wget" &> /dev/null; then
            wget -O $file $url
        else
            echo "Unable to download file because neither wget nor curl are available."
            popd >/dev/null
            exit 1
        fi
    }

    OS_NAME="$(uname -s)"
    case $OS_NAME in
        Linux*)
            download_file "https://dot.net/v1/dotnet-install.sh" "$BUILD_DIR/dotnet-install.sh"
            chmod +x $BUILD_DIR/dotnet-install.sh
            $BUILD_DIR/dotnet-install.sh --channel $DOTNET_CHANNEL --install-dir "$DOTNET_DIR"
            ;;

        MSYS_NT*)
            ;&
        CYGWIN*)
            ;&
        MINGW*)
            download_file "https://dot.net/v1/dotnet-install.ps1" "$BUILD_DIR/dotnet-install.ps1"
            powershell $BUILD_DIR/dotnet-install.ps1 -Channel $DOTNET_CHANNEL -InstallDir "$DOTNET_DIR"
            ;;

        *)
            echo "Unknown OS ($OS_NAME), can't automatically install .NET SDK into: $DOTNET_DIR"
            popd >/dev/null
            exit 1
    esac
fi

# Check if the hemake assembly needs to be rebuilt
NEEDS_REBUILD=0
if [[ ! -f $HEMAKE_BUILD_ASSEMBLY ]]; then
    NEEDS_REBUILD=1
    echo "Building hemake (assembly not found)..."
elif [[ $HEMAKE_SRC_PROJ -nt $HEMAKE_BUILD_ASSEMBLY ]]; then
    NEEDS_REBUILD=1
    echo "Building hemake (project file updated)..."
else
    for f in "$HE_SRC_DIR/**.cs"; do
        if [[ $f -nt $HEMAKE_BUILD_ASSEMBLY ]]; then
            NEEDS_REBUILD=1
            echo "Building hemake (source files updated)..."
            break
        fi
    done
fi

# If a rebuild is required, build the hemake assembly
if [[ $NEEDS_REBUILD -eq 1 ]]; then
    if [[ -z "$MSBUILD_VERBOSITY" ]]; then
        MSBUILD_VERBOSITY=quiet
    fi

    $DOTNET_EXE build "$HEMAKE_SRC_PROJ" -c Release -v $MSBUILD_VERBOSITY

    if [[ $? -ne 0 ]]; then
        echo "Build failed."
        popd >/dev/null
        exit 1
    fi
fi

# Run the hemake assembly
$DOTNET_EXE "$HEMAKE_BUILD_ASSEMBLY" --project "$PROJECT_PATH" "$@"
HEMAKE_EXIT_CODE=$?

popd >/dev/null
exit $HEMAKE_EXIT_CODE
