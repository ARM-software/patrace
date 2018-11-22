#!/bin/bash
set -ex

COMMITID=$(echo ${GIT_COMMIT} | cut -c1-7)
BRANCH=$(basename ${GIT_BRANCH})

if [ "${target}" == "ios" ]; then
    # Build for iOS
    security unlock-keychain -p jenkins ~/Library/Keychains/login.keychain
    xcodebuild \
        -workspace patrace/patrace/project/iOS/retrace.xcworkspace \
        -scheme RetracerApp \
        -sdk iphoneos CONFIGURATION_BUILD_DIR=${WORKSPACE}/build \
        clean
    xcodebuild \
        -workspace patrace/patrace/project/iOS/retrace.xcworkspace \
        -scheme RetracerApp \
        -sdk iphoneos CONFIGURATION_BUILD_DIR=${WORKSPACE}/build

    ARCHIVE_NAME=${WORKSPACE}/patrace-${target}-${type}${BRANCH}-r${COMMITID}${VERSION_TYPE}.tar.xz
    echo "Compressing ${ARCHIVE_NAME}..."
    cd ${WORKSPACE}/build
    tar cJf "${ARCHIVE_NAME}" RetracerApp.app
    exit
fi

TOOLSDIR=/work/jenkins/tools

if [ "${target}" == "10.04_x11_x32" ]; then
    CMAKEPATH=${TOOLSDIR}/cmake/10.04_x32
    ARCH="x11_x32"
elif [ "${target}" == "10.04_x11_x64" ]; then
    CMAKEPATH=${TOOLSDIR}/cmake/10.04_x32
    ARCH="x11_x64"
elif [ "${target}" == "12.04_x11_x32" ]; then
    CMAKEPATH=${TOOLSDIR}/cmake/10.04_x32
    ARCH="x11_x32"
elif [ "${target}" == "12.04_x11_x64" ]; then
    CMAKEPATH=${TOOLSDIR}/cmake/12.04_x64
    ARCH="x11_x64"
elif [ "${target}" == "14.04_x11_x64" ]; then
    CMAKEPATH=${TOOLSDIR}/cmake/14.04_x64
    ARCH="x11_x64"
elif [ "${target}" == "rhe6_x32" ]; then
    CMAKEPATH=${TOOLSDIR}/cmake/10.04_x32
    ARCH="rhe6_x32"
    source /arm/tools/setup/init/bash
    module load arm/cluster
    module load swdev git/git/1.7.3.2
    module load core
    module load syslib
    module load arm/cluster
    module load gnu/tar/1.23
    module load python/argparse_py2.7/1.2.1
    module load gnu/cmake/3.4.3
    module load python/python/2.7.8
    module load gnu/gcc/4.9.1_lto
    module list
elif [ "${target}" == "rhe6_x64" ]; then
    CMAKEPATH=${TOOLSDIR}/cmake/10.04_x32
    ARCH="rhe6_x64"
    source /arm/tools/setup/init/bash
    module load arm/cluster
    module load swdev git/git/1.7.3.2
    module load core
    module load syslib
    module load arm/cluster
    module load gnu/tar/1.23
    module load python/argparse_py2.7/1.2.1
    module load gnu/cmake/3.4.3
    module load python/python/2.7.8
    module load gnu/gcc/4.9.1_lto
    module list
else
    ## Android environment
    export ANDROID_SDK_HOME=${TOOLSDIR}/android-sdk
    export PATH=${PATH}:${TOOLSDIR}/android-sdk/platforms
    export PATH=${PATH}:${TOOLSDIR}/android-sdk/tools
    export PATH=${PATH}:${TOOLSDIR}/android-ndk/android-ndk-r10d/

    ## ARM compiler environment
    export PATH=${PATH}:${TOOLSDIR}/arm-compiler/codesourcery/linuxeabi/arm-2014.05/bin/
    export PATH=${PATH}:${TOOLSDIR}/arm-compiler/linaro/gnueabi/4.9_2014.07/bin/
    export PATH=${PATH}:${TOOLSDIR}/arm-compiler/arm/gcc_aarch64/4.8_2014.02/bin/

    CMAKEPATH=${TOOLSDIR}/cmake/12.04_x64
    ARCH=${target}
fi

cmake --version
# Use same version of cmake for all environments
export PATH=${CMAKEPATH}/bin:${PATH}

cmake --version

echo $PATH
echo $LD_LIBRARY_PATH
which gcc
which g++

# Exporting AOSP_PATH enables building it for Android. The plugin has a
# dependency to AOSP source unlike the retracer etc. because the DDK
# #includes some AOSP headers.
export AOSP_PATH=${TOOLSDIR}/headers_for_pateam_cinstr_plugin/aosp_headers

# Setting TEST_LIBRARY makes the test library to be built
# Ignore fbdev_aarch64 and fbdev_x64 for now
if [ "${target}" != "fbdev_aarch64" ] && [ "${target}" != "fbdev_x64" ] && [ "${target}" != "rhe6_x32" ] && [ "${target}" != "rhe6_x64" ]; then
    export TEST_LIBRARY=${TOOLSDIR}/test_libs/${target}
fi

# Set BUILD_PROJECT to patrace if not already set
if [ -z "${BUILD_PROJECT}" ]; then
    BUILD_PROJECT="patrace"
fi

if [ "${target}" == "rhe6_x32" ] || [ "${target}" == "rhe6_x64" ]; then
    CC=x86_64-unknown-linux-gnu-gcc NO_PYTHON_BUILD=y SVN_REVISION=${COMMITID} ${WORKSPACE}/patrace/scripts/build.py ${BUILD_PROJECT} ${ARCH} ${type}
else
    /usr/bin/env PATH=${PATH} VERSION_TYPE=${VERSION_TYPE} SVN_REVISION=${COMMITID} /bin/bash -c "\"${WORKSPACE}/patrace/scripts/build.py\" ${BUILD_PROJECT} ${ARCH} ${type}"
fi

cd patrace/install

if [ "${VERSION_TYPE}" == " " ]; then
    VERSION_TYPE=""
else
    VERSION_TYPE="-${VERSION_TYPE}"
fi

if [ -n "${BRANCH}" ]; then
    BRANCH="-${BRANCH}"
fi

ARCHIVE_NAME=${WORKSPACE}/${BUILD_PROJECT}-${target}-${type}${BRANCH}-${COMMITID}${VERSION_TYPE}.tar.xz
echo "Compressing ${ARCHIVE_NAME}..."
tar cJf "${ARCHIVE_NAME}" ${BUILD_PROJECT}
