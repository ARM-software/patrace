#!/bin/bash
# Usage:
set -xe

usage() {
    echo "Usage:"
    echo "$0 directory_name app_name [device_id]"
}

DEVICE=""

if [ "x"$1 == "x" ]; then
    usage
    echo "Missing dirname for golden repo!";
    exit
fi
if [ "x"$2 == "x" ]; then
    usage
    echo "Missing app name!";
    exit;
fi
if [ "x"$3 != "x" ]; then
    DEVICE="-s $3"
fi
echo "All ok";

ADB="adb"

echo `pwd`
mkdir -p $1
pushd $1
    echo `pwd`
    mkdir -p snap
    pushd snap
        echo `pwd`
        echo "$ADB $DEVICE pull /data/apitrace/snap"
        $ADB $DEVICE pull /data/apitrace/snap
    popd
    echo `pwd`

    echo "$ADB $DEVICE pull /data/apitrace/$2"
    $ADB $DEVICE pull /data/apitrace/$2
    $ADB $DEVICE shell rm -rf /data/apitrace/snap
    $ADB $DEVICE shell mkdir /data/apitrace/snap
    $ADB $DEVICE shell chmod 777 /data/apitrace/snap
    $ADB $DEVICE shell rm -rf /data/apitrace/$2
    echo `pwd`
    mv $2.1.pat `basename $1`.pat
    rm -f $2.meta
popd
