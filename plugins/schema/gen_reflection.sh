#!/usr/bin/env bash

BUILD_WIN32_PREFIX="../../build/windows-x86_64"
BUILD_LINUX_PREFIX="../../build/linux-x86_64"
BUILD_SUFFIX="/bin/he_schemac"

if [ -f "$BUILD_WIN32_PREFIX-debug$BUILD_SUFFIX" ]; then
    EXE="$BUILD_WIN32_PREFIX-debug$BUILD_SUFFIX"
elif [ -f "$BUILD_WIN32_PREFIX-release$BUILD_SUFFIX" ]; then
    EXE="$BUILD_WIN32_PREFIX-release$BUILD_SUFFIX"
elif [ -f "$BUILD_LINUX_PREFIX-debug$BUILD_SUFFIX" ]; then
    EXE="$BUILD_LINUX_PREFIX-debug$BUILD_SUFFIX"
elif [ -f "$BUILD_LINUX_PREFIX-release$BUILD_SUFFIX" ]; then
    EXE="$BUILD_LINUX_PREFIX-release$BUILD_SUFFIX"
else
    echo "Cannot find he_schemac executable, did you build it?"
    exit -1
fi

$EXE -t cpp -o ./schema/include/he/schema ./schema/include/he/schema/schema.hsc &&
mv ./schema/include/he/schema/schema.hsc.cpp ./schema/src/
